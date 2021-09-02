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

#ifndef __UTIL_H
#define __UTIL_H

#ifndef _GNU_SOURCE
#define _GNU_SOURCE /* See feature_test_macros(7) */
#endif

#include <execinfo.h>
#include <float.h>
#include <sched.h>
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/time.h>
#include <unistd.h>

static inline double d_gettimeofday()
{
    struct timeval tv;

    if (gettimeofday(&tv, NULL)) {
        perror("gettimeofday");
        exit(-1);
    }
    // time in us
    return (tv.tv_sec * 1000000 + ((double) tv.tv_usec));
}

static inline unsigned get_nbcores()
{
    // return nb_core
    return sysconf(_SC_NPROCESSORS_ONLN);
}

static inline void set_affinity(long tid)
{
    unsigned cpu = (tid % get_nbcores());
    // printf("%d, i am placed in %d on %d\n", tid, cpu, get_nbcores());
    cpu_set_t mask;
    CPU_ZERO(&mask);
    CPU_SET(cpu, &mask);
#if SCHED_SETAFFINITY_ARGS == 2
    sched_setaffinity(0, &mask);
#else
    sched_setaffinity(0, sizeof(mask), &mask);
#endif
}

/* RAND_MAX assumed to be nbnodes */
static inline inline int myrand(void)
{
    thd_seed = thd_seed * 1103515245 + 12345;
    return ((unsigned) (thd_seed / 65536) % 32768);
}

static inline void mysrand(unsigned seed)
{
    thd_seed = seed;
}


static inline double get_sum(double *data, int samples)
{
    int i;
    double sum = 0;

    for (i = 0; i < samples; i++)
        sum += data[i];

    return sum;
}

static inline double get_avg(double *data, int samples)
{
    int i;
    double sum = 0;

    for (i = 0; i < samples; i++)
        sum += data[i];

    return sum / (double) samples;
}

static inline double get_max(double *data, int samples)
{
    int i;
    double max = 0;

    for (i = 0; i < samples; i++)
        max = max < data[i] ? data[i] : max;

    return max;
}

static inline double get_min(double *data, int samples)
{
    int i;
    double min = DBL_MAX;

    for (i = 0; i < samples; i++)
        min = min > data[i] ? data[i] : min;

    return min;
}

static inline double get_cpw(double *data, int samples, int div)
{
    int i;
    double sum = 0;

    for (i = 0; i < samples; i++)
        sum += data[i];

    sum /= (double) samples;

    return sum / (double) div;
}

static inline double get_cpu_work(double *data, int samples, int mtc)
{
    int i;
    double sum = 0;

    for (i = 0; i < samples; i++)
        sum += data[i];

    return sum / (double) (samples + (samples - mtc));
}

static inline void bt_sighandler(int sig, struct sigcontext ctx)
{
    void *trace[16];
    char **messages;
    long i, trace_size = 0;
    void *inst;

#if __WORDSIZE == 32
    inst = (void *) ctx.eip;
#else
    inst = (void *) ctx.rip;
#endif

    if (sig == SIGSEGV)
        printf(
            "Got signal %d, faulty address is %lx, "
            "from %p\n",
            sig, ctx.cr2, inst);
    else
        printf("Got signal %d\n", sig);

    trace_size = backtrace(trace, 16);
    /* overwrite sigaction with caller's address */
    trace[1] = (void *) inst;
    messages = backtrace_symbols(trace, trace_size);
    if (messages == NULL) {
        exit(-1);
    }


    printf("[%ld] Execution path:\n", thread);
    /* skip first stack frame (points here) */
    for (i = 1; i < trace_size; ++i)
        printf("[%ld] %s\n", thread, messages[i]);

    exit(-1);
}

#endif
