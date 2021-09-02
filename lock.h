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

/*
 * Spinlock operations.  These were hand-coded specially for this
 * effort without reference to GPL code.
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
 * Copyright (c) 2003 IBM Corporation.
 */

#ifndef __LOCK_H
#define __LOCK_H

#include <stdio.h>
#include <stdlib.h>
#include "atomic.h"

#define CACHE_LINE_SIZE 64

typedef pthread_spinlock_t spinlock_t;
#define spin_init(a) pthread_spin_init(a, PTHREAD_PROCESS_PRIVATE)
#define spin_lock pthread_spin_lock
#define spin_unlock pthread_spin_unlock

#define lock_validate(sl) /**/

/*****************************************************************/
#ifdef RWLOCK
/*****************************************************************/

typedef pthread_rwlock_t lock_t;
#define lock_init(a) pthread_rwlock_init(a, PTHREAD_PROCESS_PRIVATE)
#define lock_rlock(l) pthread_rwlock_rdlock(l)
#define lock_wlock(l) pthread_rwlock_wrlock(l)
#define lock_runlock(l) pthread_rwlock_unlock(l)
#define lock_wunlock(l) pthread_rwlock_unlock(l)
//#define lock_trylock pthread_spin_trylock

/*****************************************************************/
#elif defined(LOCK_CLASSIC)
/*****************************************************************/

typedef volatile long lock_t;

//#define SPIN_LOCK_UNLOCKED 0
static inline void lock_init(lock_t *exclusion)
{
    *exclusion = 0;
    __sync_synchronize();  // Memory barrier.
}

#define lock_rlock(l) lock_lock(l)
#define lock_wlock(l) lock_lock(l)
static inline void lock_lock(lock_t *exclusion)
{
    while (__sync_lock_test_and_set(exclusion, 1))
        ;
}


#define lock_wunlock(l) lock_unlock(l)
static inline void lock_unlock(lock_t *exclusion)
{
    if (*exclusion != 1) {
        printf("abort!!!\n");
        abort();
    }
    __sync_lock_release(exclusion, 0);
}

/*****************************************************************/
#elif defined(SEQLOCK)
/*****************************************************************/

typedef volatile unsigned long lock_t;

#define lock_init(sl)         \
    ({                        \
        *sl = 0;              \
        __sync_synchronize(); \
    })

#define lock_rlock(sl)                    \
    ({                                    \
        do {                              \
            __loc_val = ACCESS_ONCE(*sl); \
            read_read_barrier();          \
        } while ((__loc_val & 0x1));      \
        read_read_barrier();              \
    })

#define lock_runlock(sl)                 \
    ({                                   \
        read_read_barrier();             \
        long __sqtmp = ACCESS_ONCE(*sl); \
        if ((__loc_val != __sqtmp))      \
            goto search_retry;           \
    })

#undef lock_validate
#define lock_validate(sl) lock_runlock(sl)

//#define lock_runlock(sl) ({if(__loc_val != ACCESS_ONCE(*sl)) goto
// search_retry;})

#define lock_wlock(sl)         \
    ({                         \
        read_read_barrier();   \
        *sl += 1;              \
        write_write_barrier(); \
    })
#ifdef ONE_WRITER
#ifdef NO_WR_BR
#define lock_wunlock(sl)      \
    ({                        \
        read_read_barrier();  \
        *sl += 1;             \
        write_read_barrier(); \
    })
#else
#define lock_wunlock(sl)       \
    ({                         \
        read_read_barrier();   \
        *sl += 1;              \
        write_write_barrier(); \
    })
#endif


#else
#define lock_wlock(sl)                                                       \
    int __sqsucc = 0;                                                        \
    long __sqold;                                                            \
    do {                                                                     \
        __sqold = ACCESS_ONCE(*sl);                                          \
        if (__sqold & 0x1) {                                                 \
            continue;                                                        \
        }                                                                    \
        __sqsucc = __sync_bool_compare_and_swap(sl, __sqold, (__sqold + 1)); \
    } while (!__sqsucc);                                                     \
    assert(ACCESS_ONCE(*sl) & 0x1)

#define lock_wunlock(sl) ({ __sync_fetch_and_add(sl, 1); })


#endif

/*****************************************************************/
#elif defined(GCLOCK)
/*****************************************************************/

typedef volatile unsigned long lock_t
    __attribute__((__aligned__(CACHE_LINE_SIZE)));
// typedef volatile unsigned long lock_t ;
#define lock_init(sl)         \
    ({                        \
        *sl = 0;              \
        __sync_synchronize(); \
    })

#ifdef ONE_WRITER

#define lock_rlock(sl)                    \
    ({                                    \
        do {                              \
            __loc_val = ACCESS_ONCE(*sl); \
            read_read_barrier();          \
        } while (0);                      \
        read_read_barrier();              \
    })

#define lock_runlock(sl)                 \
    ({                                   \
        read_read_barrier();             \
        long __sqtmp = ACCESS_ONCE(*sl); \
        if ((__loc_val != __sqtmp))      \
            goto search_retry;           \
    })
//#define lock_runlock(sl) ({ memory_barrier(); if(__loc_val !=
// ACCESS_ONCE(*sl)) goto search_retry;})

#undef lock_validate
#define lock_validate(sl) lock_runlock(sl)

#define lock_wlock(sl) /**/
#ifdef NO_WR_BR
#define lock_wunlock(sl)       \
    ({                         \
        *sl += 1;              \
        write_write_barrier(); \
    })
#else
#define lock_wunlock(sl)      \
    ({                        \
        *sl += 1;             \
        write_read_barrier(); \
    })
#endif

#else /* ONE_WRITER */

#define lock_rlock(sl) __loc_val = ACCESS_ONCE(*sl) >> 1;

#define lock_runlock(sl)                        \
    ({                                          \
        read_read_barrier();                    \
        long __sqtmp = (ACCESS_ONCE(*sl) >> 1); \
        if ((__loc_val != __sqtmp))             \
            goto search_retry;                  \
    })
//#define lock_runlock(sl) ({ memory_barrier(); if(__loc_val !=
// ACCESS_ONCE(*sl)) goto search_retry;})

#define lock_wlock(sl)                                                       \
    int __sqsucc = 0;                                                        \
    long __oldv;                                                             \
    do {                                                                     \
        __oldv = ACCESS_ONCE(*sl);                                           \
        if (__oldv & 0x1) {                                                  \
            continue;                                                        \
        }                                                                    \
        __sqsucc = __sync_bool_compare_and_swap(sl, __oldv, (__oldv | 0x1)); \
    } while (!__sqsucc);                                                     \
    assert(ACCESS_ONCE(*sl) & 0x1)

#define lock_wunlock(sl)             \
    ({                               \
        __sync_fetch_and_add(sl, 1); \
    })  // adding a 2(1 and 1 when we locked) will increment the GC(bits 31 to
        // 1) and put to zero the lock(the bit 0)

#endif /* ONE_WRITER */

/*****************************************************************/
#elif defined(LSPIN)
/*****************************************************************/
typedef pthread_spinlock_t lock_t;


#define lock_init(a) pthread_spin_init(a, PTHREAD_PROCESS_PRIVATE)
#define lock_rlock(l) pthread_spin_lock(l)
#define lock_wlock(l) pthread_spin_lock(l)
#define lock_runlock(l) pthread_spin_unlock(l)
#define lock_wunlock(l) pthread_spin_unlock(l)
#define lock_trylock(l) pthread_spin_trylock(l)

/*****************************************************************/
#elif defined(SGECOS)
/*****************************************************************/
typedef long lock_t;


#define lock_init(l) ({ *l = 0; })
//#define lock_rlock(l) /**/
//#define lock_wlock(l)
//#define lock_runlock(l)
//#define lock_wunlock(l)
//#define lock_trylock(l)

/*****************************************************************/
#else
/*****************************************************************/
typedef pthread_spinlock_t lock_t;

#ifdef ONE_WRITER

#define lock_init(a) pthread_spin_init(a, PTHREAD_PROCESS_PRIVATE)
#define lock_wlock(a) /**/
#ifdef NO_WR_BR
#define lock_wunlock(a) write_write_barrier();
#else
#define lock_wunlock(a) \
    write_read_barrier();  // must flush the fifo, so the effect is visible
                           // within the function
#endif

#else /* ONE_WRITER*/

#define lock_init(a) pthread_spin_init(a, PTHREAD_PROCESS_PRIVATE)
//#define lock_rlock(l) pthread_spin_lock(l)
#define lock_wlock(l) pthread_spin_lock(l)
//#define lock_runlock(l) pthread_spin_unlock(l)
#define lock_wunlock(l) pthread_spin_unlock(l)
//#define lock_trylock pthread_spin_trylock
#endif /* ONE_WRITER*/

#endif

#include "test.h"

#endif /* #ifndef __LOCK_H */
