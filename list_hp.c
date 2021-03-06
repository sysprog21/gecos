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
#include "hp.h"
#include "lock.h"
#include "test.h"

hazard_pointer HP[H];

int search(int key, node_t **head, lock_t *lock)
{
    node_t **prev, *cur, *next;
    int base = thread * K;
    int off = 0;

try_again:
    prev = head;

    for (cur = ACCESS_ONCE(*prev); cur != NULL; cur = clean_pointer(next)) {
        /* Protect cur with a hazard pointer. */
        HP[base + off].p = cur;
        write_read_barrier();
        if (*prev != cur) {
            nbretry++;
            goto try_again;
        }

        if ((long) cur & 1)
            printf("%p\n", cur);
        assert(!((long) cur & 1));
        next = ACCESS_ONCE(cur->next);

        read_compute();
        if (cur->key >= key) {
            return (cur->key == key);
        }

        prev = &cur->next;
        off = !off;
    }
    return (0);
}
