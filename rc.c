#include "rc.h"
#include <assert.h>
#include "allocator_malloc.h"

int increment_if_not_zero(unsigned long *p)
{
    unsigned long old, new;
    do {
        old = *p;
        if (old == 0)
            return -1;  // inc failed
        new = old + 1;
        if (__sync_val_compare_and_swap(p, old, new) == old)
            break;
    } while (1);

    return 0;
}

void *lfrc_list_dec(node_t *nd)
{
    lfrc_refcnt_dec(nd);
}

void *lfrc_list2_dec(node_t *nd)
{
    lfrc_refcnt_dec(nd);
}

node_t *rc_new_node()
{
    return new_node();
}

void rc_free_node(node_t *n)
{
    free_node(n);
}

/* Reads a pointer to a node and increments that node's reference count. */
node_t *safe_read(node_t **p)
{
    node_t *q;
    node_t *safe;

    while (1) {
        q = *p;
        safe = (node_t *) ((long) q & -2);
        if (safe == NULL)
            return q;

        if (increment_if_not_zero(&safe->rc))
            continue;

        memory_barrier();
        /* If the pointer hasn't changed its value.... */
        if (q == *p) {
            return q;
        } else
            lfrc_refcnt_dec(safe);
    }
}

/* Decrement reference count and see if the node can be safelfreereed. */
void lfrc_refcnt_dec(node_t *p)
{
    /* It's possible to acquire a reference to a marked node, and then
     * want to release it. This stops us from getting a bus error.
     */
    p = (node_t *) ((long) p & (-2));
    if (p == NULL)
        return;

    /* 0 => no more references to this node, but someone else is already
     * reclaiming it. */
    memory_barrier();

    assert(p->rc > 0);

    // xadd return ol value
    if (atomic_xadd4(&p->rc, -1) == 1) {
        p->gc++;  // for osr
        lfrc_refcnt_dec(p->next);
        rc_free_node(p);
    }
}

void lfrc_list2_inc(node_t *nd)
{
    lfrc_refcnt_inc(nd);
}

void lfrc_list_inc(node_t *nd)
{
    lfrc_refcnt_inc(nd);
}

/* Increment reference count. */
void lfrc_refcnt_inc(node_t *p)
{
    if (p == NULL)
        return;
    atomic_xadd4(&p->rc, 1);
    memory_barrier();
}
