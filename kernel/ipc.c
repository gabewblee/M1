#include <stddef.h>

#include "idt.h"
#include "ipc.h"
#include "sched.h"
#include "syscall.h"
#include "task.h"
#include "thread.h"

#include "../lib/list.h"
#include "../lib/string.h"
#include "../mm/kheap.h"

extern thread_ctrl_blk_t* cur_running_thread;

static i32 __port_send(port_t* port, const ipc_msg_t* msg, u32 sender) {
    enter_crit_sec(flags);
    if (!list_is_empty(&port->receivers)) {
        thread_ctrl_blk_t* recv = list_first_entry(
            &port->receivers, thread_ctrl_blk_t, run_link
        );

        memcpy(recv->rx_msg, msg, sizeof(*msg));
        recv->rx_msg->sender = sender;
        recv->rx_msg         = NULL;

        sched_unblock(recv);
        exit_crit_sec(flags);
        return E_OK;
    }

    if (port->queue_len < port->queue_cap) {
        ipc_msg_t* slot = (ipc_msg_t*)kmalloc(sizeof(*slot));
        if (unlikely(!slot)) {
            exit_crit_sec(flags);
            return -(i32)E_NOMEM;
        }
        memcpy(slot, msg, sizeof(*slot));
        slot->sender = sender;
        list_init(&slot->link);
        list_add_to_tail(&slot->link, &port->messages);
        port->queue_len++;

        exit_crit_sec(flags);
        return E_OK;
    }

    cur_running_thread->tx_msg = msg;
    sched_block(&port->senders, THREAD_STATE_BLOCKED);
    cur_running_thread->tx_msg = NULL;

    exit_crit_sec(flags);
    return E_OK;
}

static i32 __port_recv(port_t* port, ipc_msg_t* out) {
    enter_crit_sec(flags);
    if (!list_is_empty(&port->messages)) {
        ipc_msg_t* slot = list_first_entry(&port->messages, ipc_msg_t, link);

        list_del(&slot->link);
        port->queue_len--;

        memcpy(out, slot, sizeof(*out));
        if (!list_is_empty(&port->senders)) {
            thread_ctrl_blk_t* snd = list_first_entry(
                &port->senders, thread_ctrl_blk_t, run_link
            );

            memcpy(slot, snd->tx_msg, sizeof(*slot));
            slot->sender = snd->tid;
            list_init(&slot->link);
            list_add_to_tail(&slot->link, &port->messages);
            port->queue_len++;

            snd->tx_msg = NULL;
            sched_unblock(snd);
        } else {
            kfree(slot);
        }

        exit_crit_sec(flags);
        return E_OK;
    }

    if (!list_is_empty(&port->senders)) {
        thread_ctrl_blk_t* snd = list_first_entry(
            &port->senders, thread_ctrl_blk_t, run_link
        );

        memcpy(out, snd->tx_msg, sizeof(*out));
        out->sender = snd->tid;
        snd->tx_msg = NULL;

        sched_unblock(snd);
        exit_crit_sec(flags);
        return E_OK;
    }

    cur_running_thread->rx_msg = out;
    sched_block(&port->receivers, THREAD_STATE_BLOCKED);
    cur_running_thread->rx_msg = NULL;

    exit_crit_sec(flags);
    return E_OK;
}

port_t* port_create(u32 capacity) {
    port_t* port = (port_t*)kmalloc(sizeof(*port));
    if (unlikely(!port))
        return NULL;

    list_init(&port->messages);
    list_init(&port->receivers);
    list_init(&port->senders);
    port->queue_len = 0;
    port->queue_cap = capacity;
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
    return __port_send(task->port, msg, cur_running_thread->tid);
}

i32 ipc_recv(ipc_msg_t* out) {
    return __port_recv(cur_running_thread->task->port, out);
}

i32 ipc_call(u32 dst, ipc_msg_t* msg) {
    task_ctrl_blk_t* task = task_lookup(dst);
    if (unlikely(!task))
        return -(i32)E_INVAL;

    if (unlikely(!cur_running_thread->reply_port))
        return -(i32)E_PERM;

    i32 ret = __port_send(task->port, msg, cur_running_thread->tid);
    if (unlikely(ret != E_OK))
        return ret;

    return __port_recv(cur_running_thread->reply_port, msg);
}

i32 ipc_reply(u32 client, const ipc_msg_t* msg) {
    thread_ctrl_blk_t* task = thread_lookup(client);
    if (unlikely(!task || !task->reply_port))
        return -(i32)E_INVAL;
    
    return __port_send(task->reply_port, msg, cur_running_thread->tid);
}

void __init ipc_init(void) {
    ;
}