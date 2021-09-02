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

int search(int key, node_t **head, lock_t *lock)
{
    node_t *cur;
    node_t *next;
    gc_t lgc;
    int cur_key;
retry:
    // if(!*head) return 0;
    // compiler_barrier();
    lgc = *lock; /* RLGC */
    compiler_barrier();
    cur = *head; /* RL */
    while (cur) {
        cur_key = cur->key; /* RD */
        next = cur->next;   /* RL */
        compiler_barrier();
        if (lgc != *lock) {
            nbretry++;
            goto retry;
        } /* CLGC */
        compiler_barrier();
        if (cur_key >= key)
            return (cur_key == key);
        cur = next;
    }
    return 0;
}

int delete (int key, node_t **head, lock_t *lock)
{
    node_t *cur, **prev;
    prev = head;
    cur = *head;
    while (cur) {
        if (cur->key >= key) {
            if (cur->key != key)
                break;
            *prev = cur->next;
            write_write_barrier();
            (*lock)++;
            write_read_barrier();
            free_node(cur);
            return 1;
        }
        prev = &cur->next;
        cur = cur->next;
    }
    return 0;
}

int insert(int key, node_t **head, lock_t *lock)
{
    int sucess = 0;
    node_t *new;
    node_t *cur, **prev;

    prev = head;
    cur = *head;

    while (cur) {
        write_compute();
        if (cur->key >= key) {
            if (cur->key == key)
                goto out;
            break;
        }
        prev = &cur->next;
        cur = cur->next;
    }

    new = new_node();
    new->key = key;
    new->next = cur;
    write_write_barrier();
    *prev = new;
    sucess = 1;
out:
    return sucess;
}
