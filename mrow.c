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

#define _GNU_SOURCE /* See feature_test_macros(7) */
#include <assert.h>
#include <pthread.h>
#include <sched.h>
#include <signal.h>
#include <stdio.h>
#include <string.h>
#include "allocator_malloc.h"
#include "lock.h"
#include "test.h"
#include "util.h"

struct hash hash;
atomic_t go = 0;
atomic_t __attribute__((__aligned__(CACHESIZE))) done = 0;
atomic_t ready = 0;

struct sigaction sa;

// RD WR times
double avg_time[MAX_THREAD];

double run_avg_rd[ITERATION];
double run_avg_wr[ITERATION];
double run_max_rd[ITERATION];
double run_max_wr[ITERATION];
double run_min_rd[ITERATION];
double run_min_wr[ITERATION];

double thd_time[MAX_THREAD];
double run_cput[MAX_THREAD];

// OPs
double suc_ins[MAX_THREAD];
double suc_del[MAX_THREAD];
double suc_sea[MAX_THREAD];

double run_ins[ITERATION];
double run_del[ITERATION];
double run_sea[ITERATION];

double thd_ops[MAX_THREAD];
double twr_ops[MAX_THREAD];  // total write
double trd_ops[MAX_THREAD];  // total read

double thd_mallocs[MAX_THREAD];
double thd_retry[MAX_THREAD];
double thd_relink[MAX_THREAD];
double thd_reprev[MAX_THREAD];
double run_mallocs[MAX_THREAD];
double run_retry[MAX_THREAD];
double run_relink[MAX_THREAD];
double run_reprev[MAX_THREAD];

double thd_blockeds[MAX_THREAD];
double run_blockeds[MAX_THREAD];

unsigned long nbthreads, nbupdaters,
    nbreaders __attribute__((__aligned__(CACHESIZE)));
unsigned long pbnodes, nbbuckets, nbkeys;

__thread long thread;
__thread long nbretry = 0;
__thread long nbrelink = 0;
__thread long nbreprev = 0;
__thread double nbmallocs = 0;
__thread int nbfl = 0;

__thread double nbblockeds = 0;  // for RCU
long qsblockeds = 0;             // for RCU
long rcublockeds = 0;            // for RCU

__thread unsigned int thd_seed;
__thread unsigned int thd_nbupdaters;

__thread double thd_ins;
__thread double thd_del;
__thread double thd_sea;

/* Install our signal handler for debug */
void setsignals()
{
    /* If we segfault, print out an error message and spin forever. */
    sa.sa_handler = (void *) bt_sighandler;
    sigemptyset(&sa.sa_mask);
    sa.sa_flags = SA_RESTART;
    sigaction(SIGSEGV, &sa, NULL);
}

static inline void get_buckets(int key, node_t ***hd, lock_t **lck)
{
    int i = key % nbbuckets;
    *hd = &hash.heads[i];
    *lck = &hash.locks[i];
}

void *run(void *arg)
{
    unsigned long i;
    char iamreader;
    int key;
    long t = (long) arg;
    long nbn;  // same cache line for the best rand!
    double start, end;
    node_t **hd;
    lock_t *lck;

    set_affinity(t);

    thd_ins = 0;
    thd_del = 0;
    thd_sea = 0;
    nbmallocs = 0;
    nbretry = 0;
    nbrelink = 0;
    nbreprev = 0;
    nbfl = 0;
    thread = t;
    thd_nbupdaters = nbupdaters;
    setsignals();
    iamreader = t >= nbupdaters ? 1 : 0;
    if (!iamreader)
        prealloc();
    mysrand(t ^ time(NULL));

    /************** barrier ****************/
    atomic_xadd4(&ready, 1);
    while (!go)
        memory_barrier();

    /******************* START ****************/
    start = d_gettimeofday();
#ifdef RCU
    rcu_register_thread();
#endif

    i = 0;
    do {
        key = myrand() % nbkeys;
        // key = rand_r(&thd_seed)%nbthreads;
        // key = (t+key)%nbkeys;
        // key = random() % nbkeys;
        // if(i%100000) printf("%d %d\n", thread, key);

        get_buckets(key, &hd, &lck);
        if (iamreader) {
            thd_sea += search(key, hd, lck);
            if (i >= NB_TEST)
                break;
        } else {
            if (i % 2)
                thd_ins += insert(key, hd, lck);
            else
                thd_del += delete (key, hd, lck);
            if (done) {
                // printf("writer stopped\n");
                break;
            }
        }
        // if(!(i%10000))
        // printf("%d loop %d\n", t, i);
#ifdef RCU_QSBR
#if ((defined RCU_QSBR_ACCELERATE) || (defined RCU_Q10))
#ifdef RCU_Q10
        if (!(i % 10))
#else
        if (!(i % 100))
#endif
#endif
            rcu_quiescent_state();
#endif
    } while (++i);

#ifdef RCU
    // if(!iamreader) rcu_barrier();
    // printf("iamreader %d, ops %d\n", iamreader, i);
    rcu_unregister_thread();
#endif

    end = d_gettimeofday();
    /******************* END ****************/

    // number of ops done
    thd_ops[t] = i;

    thd_mallocs[t] = nbmallocs;
    thd_retry[t] = nbretry;
    thd_relink[t] = nbrelink;
    thd_reprev[t] = nbreprev;

    // if(t==0) printf("%lu | nbblocked %g\n", t, nbblockeds);
#ifdef RCU
    thd_blockeds[t] = atomic_read_ptr(&rcublockeds);
#else
    thd_blockeds[t] = nbblockeds;
#endif

    // average time per ops
    avg_time[t] = (((end - start)) / i);

    // total time
    thd_time[t] = (end - start);

    suc_ins[t] = thd_ins;
    suc_sea[t] = thd_sea;
    suc_del[t] = thd_del;

    return NULL;
}

void hash_init()
{
    int i;

    hash.heads = calloc(sizeof(struct node_t *), nbbuckets);
    hash.locks = calloc(sizeof(lock_t), nbbuckets);

    for (i = 0; i < nbbuckets; i++) {
        lock_init(&hash.locks[i]);
    }

    // populate
    node_t **hd;
    lock_t *lck;

#ifdef POPULATE
    unsigned nbcores = get_nbcores();
    unsigned percorekeys = (nbkeys / nbcores) + 1;
    unsigned nextcore = 1;
    // printf("npn %d %d %d\n", nbcores, percorekeys, nextcore);
    for (i = 0; i < nbkeys; i++) {
        if (!(i % percorekeys)) {
            // printf("switching to core %d at %d keys\n", nextcore, i);
            set_affinity(nextcore % nbcores);
            nextcore++;
        }
        get_buckets(i, &hd, &lck);
        insert(i, hd, lck);
    }
#endif

#ifdef SET_TAIL
    printf("SET TAIL\n");
    // insert max key
    get_buckets(i, &hd, &lck);
    insert(nbkeys + 1, hd, lck);
#endif
}

void hash_delete()
{
    // populate
    int i;
    node_t **hd;
    lock_t *lck;

#ifdef COUNT_HASH_EMPTY
    int empty = 0;
    for (i = 0; i < nbkeys; i++) {
        get_buckets(i, &hd, &lck);
        if (*hd == NULL)
            empty++;
    }
    printf("empty = %d, total = %d, ratio: %d\n", empty, nbkeys,
           empty / nbkeys);
#endif

    for (i = 0; i < nbkeys; i++) {
        get_buckets(i, &hd, &lck);
        delete (i, hd, lck);
    }

#ifdef SET_TAIL
    // printf("SET TAIL\n");
    // insert max key
    get_buckets(i, &hd, &lck);
    delete (nbkeys + 1, hd, lck);
#endif
    free((void *) hash.heads);
    free((void *) hash.locks);
}

int test(int iteration)
{
    long i;
    pthread_attr_t attr;
    pthread_t thread[nbthreads];

    hash_init();
    init_allocator();
    pthread_attr_init(&attr);
    done = 0;
    go = 0;
    ready = 0;
    memory_barrier();

    for (i = 0; i < nbthreads; i++) {
        set_affinity(i);
        if (pthread_create(&thread[i], &attr, run, (void *) i))
            perror("creat");
    }

    while (ready != nbthreads)
        memory_barrier();
    go = 1;
    memory_barrier();

    void *res;
    for (i = (nbthreads - 1); i >= 0; i--) {
        pthread_join(thread[i], &res);
        if (i == nbupdaters) {
            done = 1;  // Stop the writers!
            memory_barrier();
        }
    }

    // printf("Done!\n", i);

    unsigned nbcores = get_nbcores();
    unsigned mtc = nbcores < nbreaders ? nbcores : nbreaders;

    run_avg_rd[iteration] = get_avg(&avg_time[nbupdaters], nbreaders);
    run_avg_wr[iteration] = get_avg(&avg_time[0], nbupdaters);

    run_max_rd[iteration] = get_max(&avg_time[nbupdaters], nbreaders);
    run_max_wr[iteration] = get_max(&avg_time[0], nbupdaters);

    run_min_rd[iteration] = get_min(&avg_time[nbupdaters], nbreaders);
    run_min_wr[iteration] = get_min(&avg_time[0], nbupdaters);

    run_retry[iteration] = get_sum(&thd_retry[nbupdaters], nbreaders);
    run_relink[iteration] = get_sum(&thd_relink[nbupdaters], nbreaders);
    run_reprev[iteration] = get_sum(&thd_reprev[nbupdaters], nbreaders);

    run_cput[iteration] =
        get_cpu_work(&thd_time[nbupdaters], nbreaders, mtc) / NB_TEST;

    run_mallocs[iteration] = get_sum(&thd_mallocs[0], nbupdaters);
    run_blockeds[iteration] = get_sum(&thd_blockeds[0], nbupdaters);

    run_sea[iteration] = get_avg(&suc_sea[nbupdaters], nbreaders);
    run_del[iteration] = get_avg(&suc_del[0], nbupdaters);
    run_ins[iteration] = get_avg(&suc_ins[0], nbupdaters);

    // printf("avg = %gus\n", run_avg_rd[iteration]);
    // printf("first reader %gus\n", thd_time[nbupdaters]);
    double time = get_avg(&thd_time[nbupdaters], nbreaders);
    trd_ops[iteration] = get_avg(&thd_ops[nbupdaters], nbreaders) / time;
    twr_ops[iteration] = get_avg(&thd_ops[0], nbupdaters) / time;

    // hash_delete();
    // delete_allocator();
}

void setup_test(int argc, char **argv)
{
    if (argc < 5) {
        printf("usage: %s nbthreads nbupdaters nbbuckets perbucket-pbnodes\n",
               argv[0]);
        exit(1);
    }

    nbthreads = strtoul(argv[1], NULL, 0);
    nbupdaters = strtoul(argv[2], NULL, 0);
    if (nbthreads <= nbupdaters) {
        printf("nbupdaters must be smaller than nbthreads\n");
        exit(1);
    }
    nbreaders = nbthreads - nbupdaters;
    nbbuckets = strtoul(argv[3], NULL, 0);
    if (!nbbuckets)
        nbbuckets = 1;
    pbnodes = strtoul(argv[4], NULL, 0);
    if (!pbnodes)
        pbnodes = 1;
    nbkeys = pbnodes * nbbuckets;
}

int main(int argc, char **argv)
{
    int i = 0;
    setsignals();
#ifdef RCU_SIGNAL
    rcu_init();
#endif
    setup_test(argc, argv);
    printf(
        "nbthreads %lu nbupdaters %lu nbbuckets %lu perbucket-pbnodes %lu "
        "nbtest %d nbreaders %ld nbcores %d"
#ifdef POPULATE
        " POPULATE"
#endif
        "\n",
        nbthreads, nbupdaters, nbbuckets, pbnodes, NB_TEST, nbreaders,
        get_nbcores());

    for (; i < ITERATION; i++)
        test(i);

    double rops = get_avg(&trd_ops[0], ITERATION);
    double wops = get_avg(&twr_ops[0], ITERATION);
    printf("read ops per micsec = %g\n", rops);
    printf("write ops per micsec = %g\n", wops);
    printf("write/read = %g\n", wops / rops);

    printf("nbmallocs = %g\n", get_avg(&run_mallocs[0], ITERATION));
    printf("nbretry = %g\n", get_avg(&run_retry[0], ITERATION));
    printf("nbrelink = %g\n", get_avg(&run_relink[0], ITERATION));
    printf("nbreprev = %g\n", get_avg(&run_reprev[0], ITERATION));
    printf("nbblockeds = %g\n", get_avg(&run_blockeds[0], ITERATION));

    printf("sucess_search = %g on %g\n", get_avg(&run_sea[0], ITERATION),
           get_avg(&thd_ops[nbupdaters], nbreaders));
    printf("sucess_insert = %g on %g\n", get_avg(&run_ins[0], ITERATION),
           get_avg(&thd_ops[0], nbupdaters) / 2);
    printf("sucess_delete = %g on %g\n", get_avg(&run_del[0], ITERATION),
           get_avg(&thd_ops[0], nbupdaters) / 2);

    printf("\n");

    printf("avg_max_read = %gus\n", get_max(&run_avg_rd[0], ITERATION));
    printf("avg_avg_read = %gus\n", get_avg(&run_avg_rd[0], ITERATION));
    printf("avg_min_read = %gus\n", get_min(&run_avg_rd[0], ITERATION));

    printf("\n");

    printf("avg_max_write = %gus\n", get_max(&run_avg_wr[0], ITERATION));
    printf("avg_avg_write = %gus\n", get_avg(&run_avg_wr[0], ITERATION));
    printf("avg_min_write = %gus\n", get_min(&run_avg_wr[0], ITERATION));

    printf("\n");

    printf("max_read = %gus\n", get_max(&run_max_rd[0], ITERATION));
    printf("min_read = %gus\n", get_min(&run_min_rd[0], ITERATION));

    printf("\n");

    printf("max_write = %gus\n", get_max(&run_max_wr[0], ITERATION));
    printf("min_write = %gus\n", get_min(&run_min_wr[0], ITERATION));

    // printf("cpu time = %gus\n", get_avg(&run_cput[0],ITERATION));//FIXME

    return 0;
}
