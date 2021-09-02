/*
 * Parts copied from:
 *   T. E. Hart. "Making Lockless Synchronization Fast: Performance Implications
 * of Memory Reclamation".
 *
 * Copyright (c) 2014 Mohamed L. Karaoui.
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

#ifndef __SMR_H
#define __SMR_H

#include "lock.h"
#include "test.h"

/* Parameters to the algorithm:
 *   K: Number of hazard pointers per CPU.
 *   H: Number of hazard pointers required.
 *   R: Chosen such that R = H + Omega(H).
 */

#define K 3
#define H (K * MAX_THREAD)
//#define R (100 + 2*H)
#ifndef DIRECT_SCAN
#define R (100 + (2 * (nbthreads * K)))
#else
#define R 0
#endif

typedef struct hazard_pointer_s {
    struct node *p;
} __attribute__((__aligned__(CACHESIZE))) hazard_pointer;

typedef struct zombie_list_s {
    struct node *p;
    int number;
} __attribute__((__aligned__(CACHESIZE))) zombie_list;

/* Must be dynamically initialized to be an array of size H. */
// hazard_pointer HP;
extern hazard_pointer HP[H];

void scan(int);

void free_node_later(int, node_t *);

#endif
