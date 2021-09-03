/*
 * Atomic operations for x86 CPUs.
 *
 * Partially copied from:
 *   T. E. Hart. "Making Lockless Synchronization Fast: Performance Implications
 * of Memory Reclamation".
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
 * Copyright (c) 2014 Mohamed L. Karaoui.
 */

#ifndef _ATOMIC_H
#define _ATOMIC_H

#if defined(__x86_64__) || defined(__i686__)
#define __TSO__
#endif

/*
 * Wrapper for atomic_cmpxchg_ptr that provides more suitable semantics for
 * use in most published CAS-based lock-free algorithms.
 */
// success retrun 1 else 0
#define CAS(ptr, oldval, newval) \
    (atomic_cmpxchg_ptr((ptr), (oldval), (newval)) == (oldval))
#define atomic_cmpxchg_ptr(p, o, n) \
    (typeof(*(p))) atomic_cmpxchg4((atomic_t *) p, (atomic_t) o, (atomic_t) n)
#define atomic_read_ptr(p) (typeof(*(p))) atomic_read4((atomic_t *) p)
#define LPTR(p) (typeof(p)) atomic_read4((atomic_t *) &p)

typedef unsigned long atomic_t;

static inline unsigned long atomic_xchg4(volatile atomic_t *ptr,
                                         unsigned long value);

/*
 * Atomically compare and exchange the value pointed to by ptr
 * with newval, but only if the pointed-to value is equal to
 * oldval.  In either case, return the value originally pointed
 * to by ptr.
 */

#define atomic_cmpxchg4(ptr, oldval, newval) \
    __sync_val_compare_and_swap(ptr, oldval, newval)

/*
 * Atomically read the value pointed to by ptr.
 */

static inline unsigned long atomic_read4(volatile atomic_t *ptr)
{
    unsigned long retval;

    asm("# atomic_read4\n"
        "mov (%1),%0\n"
        "# end atomic_read4"
        : "=r"(retval)
        : "r"(ptr), "m"(*ptr));
    return (retval);
}

/*
 * Atomically set the value pointed to by ptr to newvalue.
 */

static inline void atomic_set4(volatile atomic_t *ptr, unsigned long newvalue)
{
    (void) atomic_xchg4(ptr, newvalue);
}

/*
 * Atomically add the addend to the value pointed to by ptr,
 * returning the old value.
 */

// static inline unsigned long
#define atomic_xadd4(ctr, addend) __sync_fetch_and_add(ctr, addend)

/*
 * Atomically exchange the new value to the value pointed to by ptr,
 * returning the old value.
 */

static inline unsigned long atomic_xchg4(volatile atomic_t *ptr,
                                         unsigned long value)
{
    return __sync_lock_test_and_set(ptr, value);
}

/*
 * Prevent the compiler from moving memory references across
 * a call to this function.  This does absolutely nothing
 * to prevent the CPU from reordering references -- use
 * memory_barrier() to keep both the compiler and CPU in
 * order.
 *
 * The compiler_barrier() function can be used to force
 * the compiler not refetch variable, thereby enforcing
 * a snapshot operation.
 */
static inline void compiler_barrier(void)
{
    asm volatile("" : : : "memory");
}

static inline void read_read_barrier(void)
{
#if defined(__TSO__)
    compiler_barrier();
#else
#error "you to define a read barrier"
#endif
}

/** full memory barrier **/
static inline void memory_barrier(void)
{
#if defined(__TSO__)
    // asm volatile("mfence" : : : "memory");
    __sync_synchronize();
#else
#error "you to define a read barrier"
#endif
}

/*
 * Prevent both the compiler and the CPU from reads before write
 * acroos this function.
 */
static inline void write_read_barrier(void)
{
#if defined(__TSO__)
    asm volatile("sfence" : : : "memory");
#else
#error "you to define a read barrier"
#endif
}

/*
 * Prevent both the compiler and the CPU from moving memory
 * writes across this function.  The x86 does this by default,
 * so only need to rein in the compiler.
 */
static inline void write_write_barrier(void)
{
#if defined(__TSO__)
    compiler_barrier();
#else
#error "you to define a oredered write bariier"
#endif
}

/*
 * Prevent both the compiler and the CPU from moving anything
 * across the acquisition of a spinlock that should not be so
 * moved.
 */
static inline void spin_lock_barrier(void)
{
    compiler_barrier();
}

/*
 * Prevent both the compiler and the CPU from moving anything
 * across the acquisition of a spinlock that should not be so
 * moved.
 */
static inline void spin_unlock_barrier(void)
{
    compiler_barrier();
}

#endif
