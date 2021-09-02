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

#define DEBUG 1

#if 0
void show() {
	node_t *cur;
	
	for (cur = *head; cur != NULL; cur=cur->next)
	{
		printf("{%d} ", cur->key);
		if(cur->next) assert(cur->next->key > cur->key);
	}
	printf("\n");
}
#else
void show() {}
#endif

#ifndef SEQLOCK_VLD
#define validate_cur(a, l) /**/
#else
#define validate_cur(a, l) lock_runlock(l)
#endif

int search(int key, node_t **head, lock_t *lock)
{
    node_t *cur;
    int cur_key;
#if (defined(SEQLOCK) || defined(GCLOCK))
    long __loc_val;
search_retry:
#endif

    lock_rlock(lock);
    for (cur = *head; cur != NULL;) {
        lock_validate(lock);
        cur_key = cur->key;
        read_compute();
        if (cur_key >= key) {
            lock_runlock(lock);
            return (cur_key == key);
        }
        cur = cur->next;
        validate_cur(cur, lock);
    }
    lock_runlock(lock);
    return 0;
}

#ifdef GCLOCK_NONEED
__thread node_t *free_later;
// we need to free the node after unlock!
void free_node_later(int thread, node_t *n)
{
    if (free_later) {
        free_node(
            free_later);  // this should be done by the wunlock in the case of
                          // the delete func, but for now we are functionnal
        // write_barrier();
    }
    free_later = n;
}
#else
void free_node_later(int thread, node_t *n)
{
    free_node(n);
}
#endif

node_t *init_new_node()
{
    return new_node();
}
