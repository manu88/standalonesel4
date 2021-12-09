// from https://github.com/seL4/seL4_libs/blob/master/libsel4sync/include/sync/bin_sem_bare.h
/*
 * Copyright 2017, Data61, CSIRO (ABN 41 687 119 230)
 *
 * SPDX-License-Identifier: BSD-2-Clause
 */

#pragma once

/* An unmanaged binary semaphore; i.e. the caller stores the state related to
 * the semaphore itself. This can be useful in scenarios such as CAmkES, where
 * immediate concurrency means we have a race on initialising a managed
 * semaphore.
 */

#include <sel4/sel4.h>
#include <stddef.h>
//#include <platsupport/sync/atomic.h>


static inline int sync_atomic_decrement_safe(volatile int *x, int *oldval, int success_memorder) {
    do {
        *oldval = *x;
        if (*oldval == INT_MIN) {
            /* We would overflow */
            return -1;
        }
    } while (!__atomic_compare_exchange_n(x, oldval, *oldval - 1, 1, success_memorder, __ATOMIC_RELAXED));
     return 0;
}

/* Atomically increment an integer and return its new value. */
static inline int sync_atomic_increment(volatile int *x, int memorder) {
    return __atomic_add_fetch(x, 1, memorder);
}

static inline int sync_bin_sem_bare_wait(seL4_CPtr notification, volatile int *value) {
    int oldval;
    int result = sync_atomic_decrement_safe(value, &oldval, __ATOMIC_ACQUIRE);
    if (result != 0) {
        /* Failed decrement; too many outstanding lock holders. */
        return -1;
    }
    if (oldval <= 0) {
        seL4_Wait(notification, NULL);
        /* Even though we performed an acquire barrier during the atomic
         * decrement we did not actually have the lock yet, so we have
         * to do another one now */
        __atomic_thread_fence(__ATOMIC_ACQUIRE);
    }
    return 0;
}

static inline int sync_bin_sem_bare_post(seL4_CPtr notification, volatile int *value) {
    /* We can do an "unsafe" increment here because we know we are the only
     * lock holder.
     */
    int val = sync_atomic_increment(value, __ATOMIC_RELEASE);
    if (val <= 0) {
        seL4_Signal(notification);
    }
    return 0;
}