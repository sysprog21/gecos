/*
 * Copyright (c) 2014 Mohamed L. Karaoui.
 * Copyright (c) 2014 Sorbonne Universités, UPMC Univ Paris 06.
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA 02111-1307, USA.
 */

#include <assert.h>
#include <pthread.h>
#include <stdio.h>
#include "allocator_malloc.h"
#include "lock.h"
#include "test.h"

#ifdef GECOS_B
int search(int key, node_t **head, lock_t *lock)
{
    node_t *cur;
    node_t *next;
    node_t *prev;
    gc_t cur_gc;
    gc_t next_gc;
    gc_t prev_gc;
    int cur_key;

retry:
    cur = ACCESS_ONCE(*head);
    if (cur == NULL)
        return 0;
    cur_gc = ACCESS_ONCE(cur->gc);
    compiler_barrier();
    if (cur != ACCESS_ONCE(*head))
        goto retry;

    while (cur) {
        cur_key = cur->key;
    relink:
        next = ACCESS_ONCE(cur->next);
        if (next != NULL)
            next_gc = ACCESS_ONCE(next->gc);
        compiler_barrier();
        if (next != ACCESS_ONCE(cur->next)) {
            nbrelink++;
            goto relink;
        }
        if (cur_gc != ACCESS_ONCE(cur->gc)) {
            if (prev_gc == ACCESS_ONCE(prev->gc)) {
                cur = prev;
                cur_gc = prev_gc;
                nbreprev++;
                continue;
            }
            nbretry++;
            goto retry;
        }

        read_compute();
        if (cur_key >= key)
            return (cur_key == key);
        prev = cur;
        cur = next;
        prev_gc = cur_gc;
        cur_gc = next_gc;
    }
    return 0;
}
#elif GECOS_R
int search(int key, node_t **head, lock_t *lock)
{
    node_t *cur;
    node_t *next;
    gc_t cur_gc;
    gc_t next_gc;
    int cur_key;

retry:
    cur = ACCESS_ONCE(*head);
    if (cur == NULL)
        return 0;
    cur_gc = ACCESS_ONCE(cur->gc);
    compiler_barrier();
    if (cur != ACCESS_ONCE(*head))
        goto retry;

    while (cur) {
        cur_key = cur->key;
    relink:
        next = ACCESS_ONCE(cur->next);
        if (next != NULL)
            next_gc = ACCESS_ONCE(next->gc);
        compiler_barrier();
        if (next != ACCESS_ONCE(cur->next)) {
            nbrelink++;
            goto relink;
        }
        if (cur_gc != ACCESS_ONCE(cur->gc)) {
            nbretry++;
            goto retry;
        }

        read_compute();
        if (cur_key >= key)
            return (cur_key == key);
        cur = next;
        cur_gc = next_gc;
    }
    return 0;
}
#else
int search(int key, node_t **head, lock_t *lock)
{
    node_t *cur;
    node_t *next;
    gc_t cur_gc;
    gc_t next_gc;
    int cur_key;
#ifdef CHECK_CCRT
    int l, t;
    int prev_key;
    t = 0;
#endif
retry:
#ifdef CHECK_CCRT
    l = 0;
    prev_key = 0;
#endif
    cur = ACCESS_ONCE(*head);
    if (cur == NULL)
        return 0;
    cur_gc = ACCESS_ONCE(cur->gc);
    compiler_barrier();
    if (cur != ACCESS_ONCE(*head))
        goto retry;

    while (cur) {
        cur_key = cur->key;
        next = ACCESS_ONCE(cur->next);
        if (next != NULL)
            next_gc = ACCESS_ONCE(next->gc);
        compiler_barrier();
        if ((next != ACCESS_ONCE(cur->next)) ||
            (cur_gc != ACCESS_ONCE(cur->gc))) {
            nbretry++;
            goto retry;
        }

        read_compute();
        if (cur_key >= key)
            return (cur_key == key);
        cur_gc = next_gc;
        cur = next;
#ifdef CHECK_CCRT
        if (prev_key > cur_key)
            printf("loop %d, try %d, cur_key %d, prev_key %d", l, t, cur_key,
                   prev_key);
        assert(prev_key <= cur_key);
        prev_key = cur_key;
        l++;
        t++;
#endif
    }
    return 0;
}
#endif

#ifdef OSA_BEST
__thread node_t *free_later;
void free_node_later(int thread, node_t *n)
{
    if (free_later) {
        free_node(free_later);
        n->gc++;  // last moment
        // write_barrier();
    }
    free_later = n;
}


#else

void free_node_later(int thread, node_t *n)
{
    n->gc++;  // last moment
    free_node(n);
}


#endif
inline node_t *osa_new_node()
{
    return new_node();
}

node_t *init_new_node()
{
    return osa_new_node();
}
