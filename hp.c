/*
 * Copyright (c) 2014 Mohamed L. Karaoui.
 * Copyright (c) 2014 Sorbonne Universités, UPMC Univ Paris 06.
 *
 * Parts copied from:
 *   T. E. Hart. "Making Lockless Synchronization Fast: Performance Implications
 * of Memory Reclamation".
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
 *
 */

#include "hp.h"
#include <stdlib.h>
#include "allocator_malloc.h"


zombie_list zlist[MAX_THREAD];

/*
 * Comparison function for qsort.
 *
 * We just need any total order, so we'll use the arithmetic order
 * of pointers on the machine.
 *
 * Output (see "man qsort"):
 *  < 0 : a < b
 *    0 : a == b
 *  > 0 : a > b
 */
int compare(const void *a, const void *b)
{
    return (*(node_t **) a - *(node_t **) b);
}

void scan(int thread)
{
    /* Iteratation variables. */
    node_t *cur;
    int i;

    // printf("R: %d, number of blocked node %d\n", R, zlist[thread].number);
    nbblockeds = R;

    /* List of SMR callbacks. */
    node_t *tmplist;

    /* List of hazard pointers, and its size. */
    // FIXME improve write side avoid this alloc do it once...
    node_t **plist = malloc(sizeof(node_t *) * K * nbthreads);
    unsigned long psize;

    zombie_list *zl = &zlist[thread];
    /*
     * Make sure that the most recent node to be deleted has been unlinked
     * in all processors' views.
     *
     * Good:
     *   A -> B -> C ---> A -> C ---> A -> C
     *                    B -> C      B -> POISON
     *
     * Illegal:
     *   A -> B -> C ---> A -> B      ---> A -> C
     *                    B -> POISON      B -> POISON
     */
    write_write_barrier();

    /* Stage 1: Scan HP list and insert non-null values in plist. */
    psize = 0;
    for (i = 0; i < H; i++) {
        if (HP[i].p != NULL)
            plist[psize++] = HP[i].p;
    }

    /* Stage 2: Sort the plist. */
    qsort(plist, psize, sizeof(node_t *), compare);

    /* Stage 3: Free non-harzardous nodes. */
    tmplist = zl->p;
    zl->p = NULL;
    zl->number = 0;

    while (tmplist != NULL) {
        /* Pop cur off top of tmplist. */
        cur = tmplist;
        tmplist = tmplist->mr_next;

        if (bsearch(&cur, plist, psize, sizeof(node_t *), compare)) {
            cur->mr_next = zl->p;
            zl->p = cur;
            zl->number++;
        } else {
            free_node(cur);
        }
    }
}

void free_node_later(int thread, node_t *node)
{
    zombie_list *zl = &zlist[thread];
    node->mr_next = zl->p;
    zl->p = node;
    if (++zl->number > R)
        scan(thread);
}

node_t *init_new_node()
{
    return new_node();
}
