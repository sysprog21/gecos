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

#include "rcu.h"

extern long qsblockeds;
extern long rcublockeds;

__thread long amort;
#define AMORT_COUNT 16

void free_node_cb(struct rcu_head *head)
{
    node_t *node = caa_container_of(head, node_t, head);

#ifdef RCU_COUNT_BLC
    unsigned long tmp = atomic_read_ptr(&qsblockeds);

    if (rcublockeds < tmp) {
        (rcublockeds) = tmp;  // one threads? if not -> race cond
        write_write_barrier();
    }

    // printf("%p free node %p %ld\n", pthread_self(), node, tmp-1);

    // expérimentale evaluation indicate that the CAS operation
    // incure a small négligeable penalité on the performance
#ifdef RCU_COUNT_ATM
    amort++;
    if (amort >= AMORT_COUNT) {
        assert(tmp >= AMORT_COUNT);
        atomic_xadd4(&qsblockeds,
                     (-AMORT_COUNT));  // could steel get negatif? do a cas
        amort = 0;
    }
#else
    qsblockeds--;
#endif
    assert(qsblockeds >= 0);
#endif
    free_node(node);
    if (thread != 0)
        printf("not 0 %d\n", thread);
}

void free_node_later(int thread, node_t *node)
{
    // printf("before call rcu\n");
    // printf("%d before call rcu %g\n", thread, qsblockeds);
    call_rcu(&node->head, free_node_cb);
#ifdef RCU_COUNT_BLC
#ifdef RCU_COUNT_ATM
    // atomic_xadd4(&qsblockeds, 1);
    amort++;
    if (amort >= AMORT_COUNT)
        amort = 0;
    else if (amort == 1)
        atomic_xadd4(&qsblockeds, (AMORT_COUNT));
#else
    qsblockeds++;
#endif
        // printf("%p after call rcu %p %ld\n", pthread_self(), node,
        // qsblockeds);
#endif
}

node_t *init_new_node()
{
    return new_node();
}
