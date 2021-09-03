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
#include "test.h"

int search(int key, node_t **head, lock_t *lock)
{
    node_t *cur;
    int cur_key;
    rcu_read_lock();

    for (cur = rcu_dereference(*head); cur != NULL;
         cur = rcu_dereference(cur->next)) {
        read_compute();
        cur_key = cur->key;
        if (cur_key >= key) {
            rcu_read_unlock();
            return (cur_key == key);
        }
    }
    rcu_read_unlock();
    return 0;
}
