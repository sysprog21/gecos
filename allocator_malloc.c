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
#include <stdio.h>
#include <stdlib.h>
#include "lock.h"  //TODO use directly pthread locks
#include "test.h"

node_t *freelist;
int frct;

__thread node_t *lfl = NULL;
__thread node_t *lfl_tail = NULL;
__thread int lcnt = 0;

void free_node(node_t *n);

#define NBPRE 100000
#if (defined(PREALLOCAT))
void prealloc()
{
    for (int i = 0; i < NBPRE; i++)
        free_node((node_t *) calloc(sizeof(node_t), 1));
}
#else
void prealloc() {}
#endif

void init_allocator()
{
    freelist = NULL;
    frct = 0;
    prealloc();
}

void free_nodes(node_t *nodes, void f(void *))
{
    node_t *temp;

    while (nodes) {
        temp = nodes->next;
        compiler_barrier();
        f(nodes);
        compiler_barrier();
        nodes = temp;
    }
}

void fnd(void *n)
{
    free_node((node_t *) n);
}

void allocator_thread_free()
{
    free_nodes(lfl, fnd);
    compiler_barrier();
    lfl = NULL;
    lfl_tail = NULL;
}

void delete_allocator()
{
    free_nodes(freelist, free);
    compiler_barrier();
    freelist = NULL;
}

#ifndef SIMPLE_ALLOC
node_t *get_free()
{
    if (!lfl) {
        while (lfl = ACCESS_ONCE(freelist)) {
            if (__sync_bool_compare_and_swap(&freelist, lfl, NULL))
                break;
        }
        if (!lfl)
            return NULL;
    }
    node_t *node = lfl;
    lfl = node->mr_next;
    return node;
}

#define AMORT_FREE_ON 100
#ifdef SIMPLE_AMORT_FREE
void add_free(node_t *p)
{
    if (!p)
        preintf("no p");
    assert(p);
    p->mr_next = lfl;
    lfl = p;
    lcnt++;

    if ((lcnt >= AMORT_FREE_ON) && (ACCESS_ONCE(freelist) == NULL)) {
        // printf("%s %d, ", __FUNCTION__, thread);
        if (__sync_bool_compare_and_swap(&freelist, NULL,
                                         lfl))  // only one freer
        {
            write_write_barrier();  //?
            lfl = NULL;
            lcnt = 0;
        }
    }
}
#else
void add_free(node_t *p)
{
    if (!lfl_tail) {
        lfl_tail = p;
        // printf("%d | tail %p\n", thread, p);
    }
    p->mr_next = lfl;
    lfl = p;
    lcnt++;
    // printf("%d | lfl %p\n", thread, lfl);

    if ((lcnt >= AMORT_FREE_ON)) {
        lfl_tail->mr_next = ACCESS_ONCE(freelist);
        compiler_barrier();
        // printf("%s %d| tofreelist %p, tail %p , mr_next %p\n", __FUNCTION__,
        // thread, freelist, lfl_tail, lfl_tail->mr_next );
        compiler_barrier();
        while (!__sync_bool_compare_and_swap(&freelist, lfl_tail->mr_next,
                                             lfl))  // only one freer
        {
            lfl_tail->mr_next = freelist;
        }

        compiler_barrier();
        lfl_tail = NULL;
        // printf("%d | ALL NULL\n", thread);
        lfl = NULL;
        lcnt = 0;
    }
}
#endif

#else
node_t *get_free()
{
    node_t *node = lfl;
    if (node)
        lfl = node->mr_next;
    return node;
}

void add_free(node_t *p)
{
    p->mr_next = lfl;
    lfl = p;
}
#endif

node_t *new_node()
{
    node_t *node = get_free();
    if (!node) {
        node = (node_t *) calloc(sizeof(node_t), 1);
        nbmallocs++;
        // node->gc = 0; calloc do the job
        // node->rc = 0;
    }

    return node;
}

void free_node(node_t *n)
{
    add_free(n);
    nbfl++;
}
