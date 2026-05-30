#include <stddef.h>

#include "config.h"
#include "kernel/ipc.h"
#include "kernel/sched.h"
#include "kernel/syscall.h"
#include "kernel/task.h"
#include "kernel/thread.h"
#include "libk/list.h"
#include "libk/string.h"
#include "mm/kheap.h"
#include "mm/page.h"
#include "mm/vmm.h"
#include "uapi/uapi.h"

static void copy_from_user(const phys_addr_t user_pg_dir, void* dst, const void* src, size_t nbytes) {
    phys_addr_t old_pg_dir, kcr3 = task_lookup(0)->cr3;
    __asm__ volatile("mov %%cr3, %0" : "=r"(old_pg_dir) : : "memory");

    __asm__ volatile("mov %0, %%cr3" : : "r"(user_pg_dir) : "memory");
    memcpy((void*)VMM_IPC_SCRATCH, src, nbytes);

    __asm__ volatile("mov %0, %%cr3" : : "r"(kcr3) : "memory");
    memcpy(dst, (void*)VMM_IPC_SCRATCH, nbytes);

    __asm__ volatile("mov %0, %%cr3" : : "r"(old_pg_dir) : "memory");
}

static void copy_to_user(const phys_addr_t user_pg_dir, void* dst, const void* src, size_t nbytes) {
    phys_addr_t old_pg_dir, kcr3 = task_lookup(0)->cr3;
    __asm__ volatile("mov %%cr3, %0" : "=r"(old_pg_dir) : : "memory");

    __asm__ volatile("mov %0, %%cr3" : : "r"(kcr3) : "memory");
    memcpy((void*)VMM_IPC_SCRATCH, src, nbytes);

    __asm__ volatile("mov %0, %%cr3" : : "r"(user_pg_dir) : "memory");
    memcpy(dst, (void*)VMM_IPC_SCRATCH, nbytes);

    __asm__ volatile("mov %0, %%cr3" : : "r"(old_pg_dir) : "memory");
}

static void deliver(thread_ctrl_blk_s* recv, const ipc_msg_s* msg, u32 sender) {
    u8 kbuf[IPC_MSG_SZ];
    const thread_ctrl_blk_s* cur = thread_self();

    /* This function call might be unnecessary */
    copy_from_user(cur->cr3, kbuf, msg, IPC_MSG_SZ);
    ((ipc_msg_s*)kbuf)->sender = sender;

    copy_to_user(recv->cr3, recv->rx_msg, kbuf, IPC_MSG_SZ);
    recv->rx_msg = NULL;
}

static i32 __hot port_send(port_s* port, const ipc_msg_s* msg, u32 sender) {
    ENTER_CRIT_SEC(flags1);
    /* If there are blocked receivers, deliver the message directly to them */
    if (!list_is_empty(&port->receivers)) {
        thread_ctrl_blk_s* recv = list_first_entry(
            &port->receivers, thread_ctrl_blk_s, wait_link
        );

        deliver(recv, msg, sender);
        EXIT_CRIT_SEC(flags1);
        sched_unblock(recv);
        return E_OK;
    }

    /* If the port has space, enqueue the message in the port's message queue */
    if (port->cnt < port->cap) {
        ipc_msg_ext_s* slot = (ipc_msg_ext_s*)kmalloc(sizeof(ipc_msg_ext_s));
        if (unlikely(!slot)) {
            EXIT_CRIT_SEC(flags1);
            return -(i32)E_NOMEM;
        }

        copy_from_user(thread_self()->cr3, &slot->msg, msg, IPC_MSG_SZ);
        slot->msg.sender = sender;

        list_init(&slot->link);
        list_add_to_tail(&slot->link, &port->msgs);
        port->cnt++;

        EXIT_CRIT_SEC(flags1);
        return E_OK;
    }

    /* If the port is full, block the sender */
    thread_ctrl_blk_s* cur = thread_self();
    cur->tx_msg            = msg;

    EXIT_CRIT_SEC(flags1);
    sched_block(&port->senders, THREAD_STATE_BLOCKED);

    ENTER_CRIT_SEC(flags2);
    cur->tx_msg = NULL;
    EXIT_CRIT_SEC(flags2);
    return E_OK;
}

static i32 __hot port_recv(port_s* port, ipc_msg_s* out) {
    ENTER_CRIT_SEC(flags1);
    /* If there are enqueued msgs, deliver the message directly to the caller */
    if (!list_is_empty(&port->msgs)) {
        ipc_msg_ext_s* slot = list_first_entry(&port->msgs, ipc_msg_ext_s, link);

        list_del(&slot->link);
        port->cnt--;

        copy_to_user(thread_self()->cr3, out, &slot->msg, IPC_MSG_SZ);
        if (!list_is_empty(&port->senders)) {
            /* Reuse previously allocated message slot */
            thread_ctrl_blk_s* sender = list_first_entry(
                &port->senders, thread_ctrl_blk_s, wait_link
            );

            copy_from_user(sender->cr3, &slot->msg, sender->tx_msg, IPC_MSG_SZ);
            slot->msg.sender = sender->tid;

            list_init(&slot->link);
            list_add_to_tail(&slot->link, &port->msgs);
            port->cnt++;

            sender->tx_msg = NULL;
            EXIT_CRIT_SEC(flags1);
            sched_unblock(sender);
            return E_OK;
        }

        kfree(slot);
        EXIT_CRIT_SEC(flags1);
        return E_OK;
    }

    /* If there are blocked senders, retrieve message directly from sender */
    if (!list_is_empty(&port->senders)) {
        thread_ctrl_blk_s* sender = list_first_entry(
            &port->senders, thread_ctrl_blk_s, wait_link
        );

        {
            u8 buf[IPC_MSG_SZ];
            copy_from_user(sender->cr3, buf, sender->tx_msg, IPC_MSG_SZ);
            ((ipc_msg_s*)buf)->sender = sender->tid;
            copy_to_user(thread_self()->cr3, out, buf, IPC_MSG_SZ);
        }
        sender->tx_msg = NULL;

        EXIT_CRIT_SEC(flags1);
        sched_unblock(sender);
        return E_OK;
    }

    /* Block the receiver until a message is available */
    thread_ctrl_blk_s* cur = thread_self();
    cur->rx_msg            = out;

    EXIT_CRIT_SEC(flags1);
    sched_block(&port->receivers, THREAD_STATE_BLOCKED);

    ENTER_CRIT_SEC(flags2);
    cur->rx_msg = NULL;
    EXIT_CRIT_SEC(flags2);
    return E_OK;
}

port_s* port_create(u32 capacity) {
    port_s* port = (port_s*)kmalloc(sizeof(port_s));
    if (unlikely(!port))
        return NULL;

    list_init(&port->msgs);
    list_init(&port->receivers);
    list_init(&port->senders);

    port->cnt = 0; port->cap = capacity;
    return port;
}

void port_destroy(port_s* port) {
    while (!list_is_empty(&port->msgs)) {
        ipc_msg_ext_s* slot = list_first_entry(&port->msgs, ipc_msg_ext_s, link);
        list_del(&slot->link);
        kfree(slot);
    }
    kfree(port);
}

i32 ipc_send(u32 dst, const ipc_msg_s* msg) {
    task_ctrl_blk_s* task = task_lookup(dst);
    if (unlikely(!task))
        return -(i32)E_INVAL;

    return port_send(task->port, msg, thread_self()->tid);
}

i32 ipc_recv(ipc_msg_s* out) {
    return port_recv(thread_self()->task->port, out);
}

i32 ipc_call(u32 dst, ipc_msg_s* msg) {
    task_ctrl_blk_s* task = task_lookup(dst);
    if (unlikely(!task))
        return -(i32)E_INVAL;

    thread_ctrl_blk_s* cur = thread_self();
    if (unlikely(!cur->reply_port))
        return -(i32)E_PERM;

    i32 ret = port_send(task->port, msg, cur->tid);
    if (unlikely(ret != E_OK))
        return ret;

    return port_recv(cur->reply_port, msg);
}

i32 ipc_reply(u32 client, const ipc_msg_s* msg) {
    thread_ctrl_blk_s* thread = thread_lookup(client);
    if (unlikely(!thread || !thread->reply_port))
        return -(i32)E_INVAL;

    return port_send(thread->reply_port, msg, thread_self()->tid);
}

void __init ipc_init(void) {
    ;
}