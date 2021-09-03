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
#include "lock.h"
#include "rc.h"
#include "test.h"

int search(int key, node_t **head, lock_t *lock)
{
    node_t **prev, *cur, *next;
    int retval;

try_again:
    prev = head;
    cur = safe_read(prev);
    for (; cur != NULL;) {
        if (cur->key >= key) {
            retval = (cur->key == key);
            lfrc_refcnt_dec(cur);
            return retval;
        }

        next = safe_read(&cur->next);
        lfrc_refcnt_dec(cur);
        cur = next;
    }
    lfrc_refcnt_dec(cur);
    return (0);
}

void free_node_later(int thread, node_t *n)
{
    /* FIXME: is it safe do it here (it was before the delete) */
    lfrc_refcnt_inc(n->next);
    lfrc_refcnt_dec(n);
}

node_t *init_new_node()
{
    node_t *new = new_node();
    atomic_xadd4(&new->rc, 1);  // also act as write barrier
    return new;
}
