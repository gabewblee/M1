#include <kernel/core/initcall.h>
#include <kernel/core/sched.h>
#include <kernel/core/task.h>
#include <kernel/core/thread.h>
#include <kernel/ipc/ipc.h>
#include <kernel/syscall.h>
#include <kernel/uaccess.h>
#include <libk/list.h>
#include <libk/string.h>
#include <mm/kheap.h>
#include <mm/page.h>
#include <mm/vmm.h>
#include <stddef.h>
#include <uapi/uapi.h>

static i32 deliver(thread_ctrl_blk_s* recv, ipc_packet_s* packet, u32 sender) {
    u8 kbuf[IPC_PACKET_SZ];
    thread_ctrl_blk_s* cur = thread_self();

    i32 ret = copy_from_user(cur->cr3, kbuf, packet, IPC_PACKET_SZ);
    if (unlikely(ret != E_OK))
        return ret;

    ((ipc_packet_s*)kbuf)->hdr.sender = sender;

    ret = copy_to_user(recv->cr3, recv->rx_packet, kbuf, IPC_PACKET_SZ);
    if (unlikely(ret != E_OK))
        return ret;

    recv->rx_packet = NULL;
    return E_OK;
}

static i32 __hot port_send(task_port_s* port, ipc_packet_s* packet, u32 sender) {
    thread_ctrl_blk_s* unblock = NULL;
    bool block = false;
    i32 ret = E_OK;

    {
        spin_guard(&port->lock);

        /* Case 1: Deliver packet to blocked receiver */
        if (!list_is_empty(&port->recvs)) {
            unblock = list_first_entry(&port->recvs, thread_ctrl_blk_s, wait_link);
            ret = deliver(unblock, packet, sender);
            if (unlikely(ret != E_OK))
                unblock = NULL;

        /* Case 2: Enqueue packet in port's packet queue */
        } else if (port->cnt < port->cap) {
            kipc_packet_s* slot = (kipc_packet_s*)kmalloc(sizeof(kipc_packet_s));
            if (unlikely(!slot)) {
                ret = -(i32)E_NOMEM;
            } else {
                ret = copy_from_user(thread_self()->cr3, &slot->packet, packet, IPC_PACKET_SZ);
                if (unlikely(ret != E_OK)) {
                    kfree(slot);
                } else {
                    slot->packet.hdr.sender = sender;
                    list_init(&slot->link);
                    list_add_to_tail(&slot->link, &port->packets);
                    port->cnt++;
                }
            }

        /* Case 3: Block the sender */
        } else {
            thread_self()->tx_packet = packet;
            block = true;
        }
    }

    if (unblock)
        sched_unblock(unblock);

    if (block) {
        sched_block(&port->sndrs, THREAD_STATE_BLOCKED);
        spin_guard(&port->lock);
        thread_self()->tx_packet = NULL;
    }

    return ret;
}

static i32 __hot port_recv(task_port_s* port, ipc_packet_s* packet) {
    thread_ctrl_blk_s* unblock = NULL;
    kipc_packet_s* free = NULL;
    bool block = false;
    i32 ret = E_OK;

    {
        spin_guard(&port->lock);

        /* Case 1: Deliver the packet to caller */
        if (!list_is_empty(&port->packets)) {
            kipc_packet_s* slot = list_first_entry(&port->packets, kipc_packet_s, link);
            list_del(&slot->link);
            port->cnt--;

            ret = copy_to_user(thread_self()->cr3, packet, &slot->packet, IPC_PACKET_SZ);
            if (unlikely(ret != E_OK)) {
                free = slot;
            } else if (!list_is_empty(&port->sndrs)) {
                /* Reuse previously allocated packet slot */
                thread_ctrl_blk_s* sndr = list_first_entry(
                    &port->sndrs, thread_ctrl_blk_s, wait_link
                );
                
                i32 sret = copy_from_user(sndr->cr3, &slot->packet, sndr->tx_packet, IPC_PACKET_SZ);
                sndr->tx_packet = NULL;
                unblock   = sndr;
                if (unlikely(sret != E_OK)) {
                    free = slot;
                } else {
                    slot->packet.hdr.sender = sndr->id;
                    list_init(&slot->link);
                    list_add_to_tail(&slot->link, &port->packets);
                    port->cnt++;
                }
            } else {
                free = slot;
            }

        /* Case 2: Retrieve packet from sender */
        } else if (!list_is_empty(&port->sndrs)) {
            thread_ctrl_blk_s* sndr = list_first_entry(
                &port->sndrs, thread_ctrl_blk_s, wait_link
            );
            u8 buf[IPC_PACKET_SZ];
            ret = copy_from_user(sndr->cr3, buf, sndr->tx_packet, IPC_PACKET_SZ);
            if (likely(ret == E_OK)) {
                ((ipc_packet_s*)buf)->hdr.sender = sndr->id;
                ret = copy_to_user(thread_self()->cr3, packet, buf, IPC_PACKET_SZ);
            }
            sndr->tx_packet = NULL;
            unblock   = sndr;

        /* Case 3: Block the receiver */
        } else {
            thread_self()->rx_packet = packet;
            block = true;
        }
    }

    if (free)
        kfree(free);

    if (unblock)
        sched_unblock(unblock);

    if (block) {
        sched_block(&port->recvs, THREAD_STATE_BLOCKED);
        spin_guard(&port->lock);
        thread_self()->rx_packet = NULL;
    }

    return ret;
}

task_port_s* port_create(u32 cap) {
    task_port_s* port = (task_port_s*)kmalloc(sizeof(task_port_s));
    if (unlikely(!port))
        return NULL;

    spinlock_init(&port->lock);
    list_init(&port->packets);
    list_init(&port->recvs);
    list_init(&port->sndrs);

    port->cnt = 0;
    port->cap = cap;
    return port;
}

void port_destroy(task_port_s* port) {
    while (!list_is_empty(&port->packets)) {
        kipc_packet_s* slot = list_first_entry(&port->packets, kipc_packet_s, link);
        list_del(&slot->link);
        kfree(slot);
    }
    kfree(port);
}

i32 ipc_send(u32 dst, ipc_packet_s* packet) {
    task_ctrl_blk_s* task = task_lookup(dst);
    if (unlikely(!task))
        return -(i32)E_INVAL;

    return port_send(task->port, packet, thread_self()->id);
}

i32 ipc_recv(ipc_packet_s* packet) {
    return port_recv(thread_self()->task->port, packet);
}

i32 ipc_call(u32 dst, ipc_packet_s* packet) {
    task_ctrl_blk_s* task = task_lookup(dst);
    if (unlikely(!task))
        return -(i32)E_INVAL;

    thread_ctrl_blk_s* cur = thread_self();
    if (unlikely(!cur->reply_port))
        return -(i32)E_PERM;

    i32 ret = port_send(task->port, packet, cur->id);
    if (unlikely(ret != E_OK))
        return ret;

    return port_recv(cur->reply_port, packet);
}

i32 ipc_reply(u32 dst, ipc_packet_s* packet) {
    thread_ctrl_blk_s* thread = thread_lookup(dst);
    if (unlikely(!thread || !thread->reply_port))
        return -(i32)E_INVAL;

    return port_send(thread->reply_port, packet, thread_self()->id);
}

void __init ipc_init(void) {
    ;
}

subsys_initcall(ipc_init);
