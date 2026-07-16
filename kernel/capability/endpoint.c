#include <kernel/capability/capability.h>
#include <kernel/capability/endpoint.h>
#include <kernel/core/initcall.h>
#include <kernel/core/sched.h>
#include <kernel/core/thread.h>
#include <kernel/sync/spinlock.h>
#include <kernel/uaccess.h>
#include <libk/list.h>
#include <stdbool.h>
#include <stddef.h>
#include <uapi/errno.h>
#include <uapi/ipc.h>

static list_node_s reply_queue;
static spinlock_s  reply_lock;

static i32 transfer(thread_ctrl_blk_s* dst, thread_ctrl_blk_s* src, u32 badge, msg_info_t mi) {
    i32 ret = copy_between_user(dst->cr3, dst->rx_msg, src->cr3, src->tx_msg, IPC_MSG_SZ);
    dst->rx_capabilities = 0;
    if (unlikely(ret != E_OK))
        return ret;

    dst->ipc_badge       = badge;
    dst->ipc_msg_info    = mi;
    if (src->tx_depth && dst->rx_depth) {
        i32 xfer = capability_transfer(
            &src->root, src->tx_cptr, src->tx_depth,
            &dst->root, dst->rx_cptr, dst->rx_depth
        );
        dst->rx_capabilities = (xfer == E_OK) ? 1 : 0;
    }

    return E_OK;
}

static inline msg_info_t mk_recv_tag(const thread_ctrl_blk_s* cur) {
    return mk_msg_info(
        get_msg_label(cur->ipc_msg_info),
        (u32)cur->rx_capabilities,
        get_msg_len(cur->ipc_msg_info)
    );
}

static i32 __hot send(endpoint_s* ep, bool is_call, bool nonblock) {
    thread_ctrl_blk_s* cur = thread_self();
    u32 flags = spin_lock_irqsave(&ep->lock);
    if (ep->state == ENDPOINT_STATE_RECV) {
        /* Copy message into first receiver */
        thread_ctrl_blk_s* recv = list_first_entry(&ep->queue, thread_ctrl_blk_s, wait_link);
        list_del(&recv->wait_link);
        if (list_is_empty(&ep->queue))
            ep->state = ENDPOINT_STATE_IDLE;

        i32 ret = transfer(recv, cur, cur->ipc_badge, cur->ipc_msg_info);
        /* Send simply puts sender back into the ready queue */
        if (!is_call || unlikely(ret != E_OK)) {
            spin_unlock_irqrestore(&ep->lock, flags);
            sched_ready(recv);
            return ret;
        }

        /* Maintain pointer to blocked caller */
        if (recv->reply)
            recv->reply->caller = cur;

        sched_enqueue(recv);
        sched_block_and_release(&reply_queue, THREAD_STATE_BLOCKED, &ep->lock, flags);
        return (i32)cur->ipc_msg_info;
    }

    if (nonblock) {
        spin_unlock_irqrestore(&ep->lock, flags);
        return -(i32)E_AGAIN;
    }

    /* No receivers available on endpoint queue */
    cur->ipc_is_call = is_call;
    ep->state = ENDPOINT_STATE_SEND;
    sched_block_and_release(&ep->queue, THREAD_STATE_BLOCKED, &ep->lock, flags);
    return is_call ? (i32)cur->ipc_msg_info : E_OK;
}

static i32 __hot recv(endpoint_s* ep, reply_s* reply, u32* badge, bool nonblock) {
    thread_ctrl_blk_s* cur = thread_self();
    u32 flags = spin_lock_irqsave(&ep->lock);
    if (ep->state == ENDPOINT_STATE_SEND) {
        /* Copy message from first sender */
        thread_ctrl_blk_s* snd = list_first_entry(&ep->queue, thread_ctrl_blk_s, wait_link);
        list_del(&snd->wait_link);
        if (list_is_empty(&ep->queue))
            ep->state = ENDPOINT_STATE_IDLE;

        i32 ret = transfer(cur, snd, snd->ipc_badge, snd->ipc_msg_info);
        if (badge)
            *badge = snd->ipc_badge;

        if (snd->ipc_is_call && likely(ret == E_OK)) {
            if (reply)
                reply->caller = snd;

            spin_unlock_irqrestore(&ep->lock, flags);
        } else {
            spin_unlock_irqrestore(&ep->lock, flags);
            sched_ready(snd);
        }

        return (ret == E_OK) ? (i32)mk_recv_tag(cur) : ret;
    }

    if (nonblock) {
        spin_unlock_irqrestore(&ep->lock, flags);
        return -(i32)E_AGAIN;
    }

    cur->reply = reply;
    ep->state = ENDPOINT_STATE_RECV;
    sched_block_and_release(&ep->queue, THREAD_STATE_BLOCKED, &ep->lock, flags);
    if (badge)
        *badge = cur->ipc_badge;

    return (i32)mk_recv_tag(cur);
}

static i32 reply(reply_s* reply, msg_info_t mi) {
    thread_ctrl_blk_s* cur = thread_self();
    u32 flags = spin_lock_irqsave(&reply_lock);
    thread_ctrl_blk_s* caller = reply ? reply->caller : NULL;
    if (unlikely(!caller)) {
        spin_unlock_irqrestore(&reply_lock, flags);
        return -(i32)E_INVAL;
    }

    reply->caller = NULL;
    i32 ret = transfer(caller, cur, 0, mi);
    spin_unlock_irqrestore(&reply_lock, flags);
    sched_unblock(caller);
    return ret;
}

static i32 ntfn_wait(ntfn_s* nt, u32* signals) {
    thread_ctrl_blk_s* cur = thread_self();
    u32 flags = spin_lock_irqsave(&nt->lock);
    if (nt->signals) {
        if (signals)
            *signals = nt->signals;

        nt->signals = 0;
        spin_unlock_irqrestore(&nt->lock, flags);
        return E_OK;
    }

    sched_block_and_release(&nt->waiters, THREAD_STATE_BLOCKED, &nt->lock, flags);
    if (signals)
        *signals = cur->ipc_badge;

    return E_OK;
}

static cte_s* resolve(const capability_s* root, u32 cptr, capability_type_e type, u32 rights) {
    cte_s* slot = capability_lookup(root, cptr, capability_depth(root));
    if (unlikely(!slot || slot->capability.type != type || (slot->capability.rights & rights) != rights))
        return NULL;

    return slot;
}

void endpoint_init(endpoint_s* ep) {
    spinlock_init(&ep->lock);
    ep->state = ENDPOINT_STATE_IDLE;
    list_init(&ep->queue);
}

void ntfn_init(ntfn_s* nt) {
    spinlock_init(&nt->lock);
    nt->signals = 0;
    list_init(&nt->waiters);
}

void reply_init(reply_s* reply) {
    reply->caller = NULL;
}

void ntfn_signal(ntfn_s* nt, u32 bits) {
    u32 flags = spin_lock_irqsave(&nt->lock);
    nt->signals |= bits;
    if (!list_is_empty(&nt->waiters)) {
        thread_ctrl_blk_s* waiter = list_first_entry(&nt->waiters, thread_ctrl_blk_s, wait_link);
        list_del(&waiter->wait_link);
        waiter->ipc_badge = nt->signals;
        nt->signals = 0;
        spin_unlock_irqrestore(&nt->lock, flags);
        sched_ready(waiter);
        return;
    }

    spin_unlock_irqrestore(&nt->lock, flags);
}

i32 ipc_send(const capability_s* root,
             u32                 ep_cptr,
             msg_info_t          mi, 
             ipc_msg_s*          msg,
             u32                 grant_cptr, 
             bool                nonblock) {
    cte_s* slot = resolve(root, ep_cptr, CAPABILITY_TYPE_ENDPOINT, CAPABILITY_RIGHT_WRITE);
    if (unlikely(!slot))
        return -(i32)E_PERM;
    if (grant_cptr && unlikely(!(slot->capability.rights & CAPABILITY_RIGHT_GRANT)))
        return -(i32)E_PERM;

    thread_ctrl_blk_s* cur = thread_self();
    cur->tx_msg            = msg;
    cur->ipc_badge         = slot->capability.badge;
    cur->ipc_msg_info      = mi;
    cur->root              = *root;
    cur->tx_cptr           = grant_cptr;
    cur->tx_depth          = grant_cptr ? capability_depth(root) : 0;

    i32 ret = send((endpoint_s*)slot->capability.obj, false, nonblock);
    cur->tx_msg            = NULL;
    return ret;
}

i32 ipc_call(const capability_s* root, 
             u32                 ep_cptr,
             msg_info_t          mi,
             ipc_msg_s*          msg,
             u32                 grant_cptr) {
    cte_s* slot = resolve(root, ep_cptr, CAPABILITY_TYPE_ENDPOINT, CAPABILITY_RIGHT_WRITE);
    if (unlikely(!slot))
        return -(i32)E_PERM;
    if (grant_cptr && unlikely(!(slot->capability.rights & CAPABILITY_RIGHT_GRANT)))
        return -(i32)E_PERM;

    thread_ctrl_blk_s* cur = thread_self();
    cur->tx_msg            = msg;
    cur->rx_msg            = msg;
    cur->ipc_badge         = slot->capability.badge;
    cur->ipc_msg_info      = mi;
    cur->root              = *root;
    cur->tx_cptr           = grant_cptr;
    cur->tx_depth          = grant_cptr ? capability_depth(root) : 0;
    cur->rx_cptr           = 0;
    cur->rx_depth          = 0;

    i32 ret = send((endpoint_s*)slot->capability.obj, true, false);
    cur->tx_msg            = NULL;
    cur->rx_msg            = NULL;
    return ret;
}

i32 ipc_recv(const capability_s* root, 
             u32                 ep_cptr,
             u32                 reply_cptr,
             ipc_msg_s*          msg,
             u32*                badge,
             u32                 recv_slot,
             bool                nonblock) {
    cte_s* slot = resolve(root, ep_cptr, CAPABILITY_TYPE_ENDPOINT, CAPABILITY_RIGHT_READ);
    if (unlikely(!slot))
        return -(i32)E_PERM;

    reply_s* reply = NULL;
    if (reply_cptr) {
        cte_s* rslot = resolve(root, reply_cptr, CAPABILITY_TYPE_REPLY, 0);
        if (unlikely(!rslot))
            return -(i32)E_INVAL;

        reply = (reply_s*)rslot->capability.obj;
    }

    thread_ctrl_blk_s* cur = thread_self();
    cur->rx_msg            = msg;
    cur->root              = *root;
    cur->rx_cptr           = recv_slot;
    cur->rx_depth          = recv_slot ? capability_depth(root) : 0;

    i32 ret = recv((endpoint_s*)slot->capability.obj, reply, badge, nonblock);
    cur->rx_msg            = NULL;
    return ret;
}

i32 ipc_reply(const capability_s* root, u32 reply_cptr, msg_info_t mi, ipc_msg_s* msg) {
    cte_s* rslot = resolve(root, reply_cptr, CAPABILITY_TYPE_REPLY, 0);
    if (unlikely(!rslot))
        return -(i32)E_INVAL;

    thread_ctrl_blk_s* cur = thread_self();
    cur->tx_msg            = msg;
    cur->root              = *root;
    cur->tx_depth          = 0;

    i32 ret = reply((reply_s*)rslot->capability.obj, mi);
    cur->tx_msg            = NULL;
    return ret;
}

i32 ipc_replyrecv(const capability_s* root, 
                  u32                 ep_cptr,
                  u32                 reply_cptr,
                  msg_info_t          mi,
                  ipc_msg_s*          msg,
                  u32*                badge) {
    cte_s* eslot = resolve(root, ep_cptr, CAPABILITY_TYPE_ENDPOINT, CAPABILITY_RIGHT_READ);
    cte_s* rslot = resolve(root, reply_cptr, CAPABILITY_TYPE_REPLY, 0);
    if (unlikely(!eslot || !rslot))
        return -(i32)(eslot ? E_INVAL : E_PERM);

    reply_s* rep = (reply_s*)rslot->capability.obj;
    thread_ctrl_blk_s* cur = thread_self();
    cur->tx_msg            = msg;
    cur->root              = *root;
    cur->tx_depth          = 0;
    reply(rep, mi);

    cur->tx_msg            = NULL;
    cur->rx_msg            = msg;
    cur->rx_cptr           = 0;
    cur->rx_depth          = 0;

    i32 ret = recv((endpoint_s*)eslot->capability.obj, rep, badge, false);
    cur->rx_msg            = NULL;
    return ret;
}

i32 ipc_signal(const capability_s* root, u32 ntfn_cptr) {
    cte_s* slot = resolve(root, ntfn_cptr, CAPABILITY_TYPE_NTFN, CAPABILITY_RIGHT_WRITE);
    if (unlikely(!slot))
        return -(i32)E_PERM;

    u32 badge = slot->capability.badge;
    ntfn_signal((ntfn_s*)slot->capability.obj, badge ? badge : 1u);
    return E_OK;
}

i32 ipc_wait(const capability_s* root, u32 ntfn_cptr, u32* badge) {
    cte_s* slot = resolve(root, ntfn_cptr, CAPABILITY_TYPE_NTFN, CAPABILITY_RIGHT_READ);
    if (unlikely(!slot))
        return -(i32)E_PERM;

    u32 signals = 0;
    i32 ret = ntfn_wait((ntfn_s*)slot->capability.obj, &signals);
    if (badge)
        *badge = signals;

    return ret;
}

void __init ipc_init(void) {
    list_init(&reply_queue);
    spinlock_init(&reply_lock);
}

kernel_initcall(ipc_init);
