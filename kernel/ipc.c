#include <stddef.h>

#include "kernel/ipc.h"
#include "kernel/sched.h"
#include "kernel/syscall.h"
#include "kernel/task.h"
#include "kernel/thread.h"
#include "lib/list.h"
#include "lib/string.h"
#include "mm/kheap.h"

static i32 port_send(port_t* port, const ipc_msg_t* msg, u32 sender) {
    ENTER_CRIT_SEC(flags);
    if (!list_is_empty(&port->receivers)) {
        thread_ctrl_blk_t* recv = list_first_entry(
            &port->receivers, thread_ctrl_blk_t, wait_link
        );

        memcpy(recv->rx_msg, msg, sizeof(ipc_msg_t));
        recv->rx_msg->sender = sender;
        recv->rx_msg         = NULL;

        sched_unblock(recv);
        EXIT_CRIT_SEC(flags);
        return E_OK;
    }

    if (port->cnt < port->cap) {
        ipc_msg_t* slot = (ipc_msg_t*)kmalloc(sizeof(ipc_msg_t));
        if (unlikely(!slot)) {
            EXIT_CRIT_SEC(flags);
            return -(i32)E_NOMEM;
        }
        memcpy(slot, msg, sizeof(ipc_msg_t));
        slot->sender = sender;
        list_init(&slot->link);
        list_add_to_tail(&slot->link, &port->messages);
        port->cnt++;

        EXIT_CRIT_SEC(flags);
        return E_OK;
    }

    thread_ctrl_blk_t* cur = thread_self();
    cur->tx_msg = msg;
    sched_block(&port->senders, THREAD_STATE_BLOCKED);
    cur->tx_msg = NULL;

    EXIT_CRIT_SEC(flags);
    return E_OK;
}

static i32 port_recv(port_t* port, ipc_msg_t* out) {
    ENTER_CRIT_SEC(flags);
    if (!list_is_empty(&port->messages)) {
        ipc_msg_t* slot = list_first_entry(&port->messages, ipc_msg_t, link);

        list_del(&slot->link);
        port->cnt--;

        memcpy(out, slot, sizeof(ipc_msg_t));
        if (!list_is_empty(&port->senders)) {
            thread_ctrl_blk_t* snd = list_first_entry(
                &port->senders, thread_ctrl_blk_t, wait_link
            );

            memcpy(slot, snd->tx_msg, sizeof(ipc_msg_t));
            slot->sender = snd->tid;
            list_init(&slot->link);
            list_add_to_tail(&slot->link, &port->messages);
            port->cnt++;

            snd->tx_msg = NULL;
            sched_unblock(snd);
        } else {
            kfree(slot);
        }

        EXIT_CRIT_SEC(flags);
        return E_OK;
    }

    if (!list_is_empty(&port->senders)) {
        thread_ctrl_blk_t* snd = list_first_entry(
            &port->senders, thread_ctrl_blk_t, wait_link
        );

        memcpy(out, snd->tx_msg, sizeof(ipc_msg_t));
        out->sender = snd->tid;
        snd->tx_msg = NULL;

        sched_unblock(snd);
        EXIT_CRIT_SEC(flags);
        return E_OK;
    }

    thread_ctrl_blk_t* cur = thread_self();
    cur->rx_msg = out;
    sched_block(&port->receivers, THREAD_STATE_BLOCKED);
    cur->rx_msg = NULL;

    EXIT_CRIT_SEC(flags);
    return E_OK;
}

port_t* port_create(u32 capacity) {
    port_t* port = (port_t*)kmalloc(sizeof(port_t));
    if (unlikely(!port))
        return NULL;

    list_init(&port->messages);
    list_init(&port->receivers);
    list_init(&port->senders);
    port->cnt = 0;
    port->cap = capacity;
    return port;
}

void port_destroy(port_t* port) {
    if (unlikely(!port))
        return;

    while (!list_is_empty(&port->messages)) {
        ipc_msg_t* msg = list_first_entry(&port->messages, ipc_msg_t, link);
        
        list_del(&msg->link);
        kfree(msg);
    }
    kfree(port);
}

i32 ipc_send(u32 dst, const ipc_msg_t* msg) {
    task_ctrl_blk_t* task = task_lookup(dst);
    if (unlikely(!task))
        return -(i32)E_INVAL;

    return port_send(task->port, msg, thread_self()->tid);
}

i32 ipc_recv(ipc_msg_t* out) {
    return port_recv(thread_self()->task->port, out);
}

i32 ipc_call(u32 dst, ipc_msg_t* msg) {
    task_ctrl_blk_t* task = task_lookup(dst);
    if (unlikely(!task))
        return -(i32)E_INVAL;

    thread_ctrl_blk_t* cur = thread_self();
    if (unlikely(!cur->reply_port))
        return -(i32)E_PERM;

    i32 ret = port_send(task->port, msg, cur->tid);
    if (unlikely(ret != E_OK))
        return ret;

    return port_recv(cur->reply_port, msg);
}

i32 ipc_reply(u32 client, const ipc_msg_t* msg) {
    thread_ctrl_blk_t* thread = thread_lookup(client);
    if (unlikely(!thread || !thread->reply_port))
        return -(i32)E_INVAL;
    
    return port_send(thread->reply_port, msg, thread_self()->tid);
}

void __init ipc_init(void) {
    ;
}