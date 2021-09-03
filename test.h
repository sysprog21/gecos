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

#ifndef TOTEST_H
#define TOTEST_H

#define _LGPL_SOURCE
#include <pthread.h>
#include "lock.h"

#ifdef RCU_QSBR
#include <urcu-qsbr.h>
#endif

#if (defined(RCU_MB) || defined(RCU_SIGNAL))
#ifdef RCU_QSBR
#error ! cannot mix rcu flavors !
#endif
//#define RCU_MB
#include <urcu.h>
#endif

#define MAX_CPU
#define NB_TEST 2000000
#define ITERATION 5
#define NBNODE_DEFAULT 4
#define MAX_THREAD 256

#define ACCESS_ONCE(x) (*(volatile typeof(x) *) &(x))
#define clean_pointer(p) ((node_t *) (((unsigned long) (p)) & (-2)))
#define mark_pointer(p) ((node_t *) (((unsigned long) (p)) + 1))
#define CACHESIZE 64 /* Should cover most machines. */

typedef unsigned long gc_t;

typedef struct node {
    int key;
    struct node *next;
    struct node *mr_next;
    unsigned long rc;
    gc_t gc;
#ifdef RCU
    struct rcu_head head;
#endif
} __attribute__((__aligned__(CACHESIZE))) node_t;

struct hash {
    lock_t *locks;
    node_t **heads;
};
//};__attribute__ ((__aligned__ (CACHESIZE)));

extern __thread long thread;
extern __thread long nbretry;
extern __thread long nbrelink;
extern __thread long nbreprev;
extern __thread int backoff_amount;
extern __thread double nbmallocs;
extern __thread int nbfl;
extern  //__thread
    __thread double nbblockeds;
extern __thread unsigned int thd_seed;
extern __thread unsigned int thd_nbupdaters;
extern unsigned long nbnodes, nbbuckets, nbkeys;
extern unsigned long nbthreads, nbreaders, nbupdaters;
node_t *new_node();
static void *run(void *);
static void read_compute();

int insert(int key, node_t **head, lock_t *lock);
int search(int key, node_t **head, lock_t *lock);
int delete (int key, node_t **head, lock_t *lock);

static void read_compute() {}
static void write_compute() {}

#endif
