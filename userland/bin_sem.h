// from https://github.com/seL4/seL4_libs/blob/master/libsel4sync/include/sync/bin_sem.h
// atomic ops from https://github.com/seL4/util_libs/blob/b0a3a4b292e3eaedfcd5770fc647768c49b3e653/libplatsupport/include/platsupport/sync/atomic.h
/*
 * Copyright 2017, Data61, CSIRO (ABN 41 687 119 230)
 *
 * SPDX-License-Identifier: BSD-2-Clause
 */

#pragma once

#include <autoconf.h>
#include "sel4.hpp"
#include <stddef.h>
#include "bin_sem_bare.h"

typedef struct {
    seL4_CPtr notification;
    volatile int value;
} sync_bin_sem_t;

/* Initialise an unmanaged binary semaphore with a notification object
 * @param sem           A semaphore object to be initialised.
 * @param notification  A notification object to use for the lock.
 * @param value         The initial value for the semaphore. Must be 0 or 1.
 * @return              0 on success, an error code on failure. */
static inline int sync_bin_sem_init(sync_bin_sem_t *sem, seL4_CPtr notification, int value)
{
    if (sem == NULL) {
        return -1;
    }

    if (value != 0 && value != 1) {
        return -1;
    }
    sem->notification = notification;
    sem->value = value;
    return 0;
}

/* Wait on a binary semaphore
 * @param sem           An initialised semaphore to acquire.
 * @return              0 on success, an error code on failure. */
static inline int sync_bin_sem_wait(sync_bin_sem_t *sem)
{
    if (sem == NULL) {
        return -1;
    }
    return sync_bin_sem_bare_wait(sem->notification, &sem->value);
}

/* Signal a binary semaphore
 * @param sem           An initialised semaphore to release.
 * @return              0 on success, an error code on failure. */
static inline int sync_bin_sem_post(sync_bin_sem_t *sem)
{
    if (sem == NULL) {
        return -1;
    }
    return sync_bin_sem_bare_post(sem->notification, &sem->value);
}

#if 0
/* Allocate and initialise a managed binary semaphore
 * @param vka           A VKA instance used to allocate a notification object.
 * @param sem           A semaphore object to initialise.
 * @param value         The initial value for the semaphore. Must be 0 or 1.
 * @return              0 on success, an error code on failure. */
static inline int sync_bin_sem_new(vka_t *vka, sync_bin_sem_t *sem, int value)
{
    if (sem == NULL) {
        return -1;
    }
    if (value != 0 && value != 1) {
        return -1;
    }
    int error = vka_alloc_notification(vka, &(sem->notification));

    if (error != 0) {
        return error;
    } else {
        return sync_bin_sem_init(sem, sem->notification.cptr, value);
    }
}

/* Deallocate a managed binary semaphore (do not use with sync_bin_sem_init)
 * @param vka           A VKA instance used to deallocate the notification object.
 * @param sem           A semaphore object initialised by sync_bin_sem_new.
 * @return              0 on success, an error code on failure. */
static inline int sync_bin_sem_destroy(vka_t *vka, sync_bin_sem_t *sem)
{
    if (sem == NULL) {
        ZF_LOGE("Semaphore passed to sync_bin_sem_destroy was NULL");
        return -1;
    }
    vka_free_object(vka, &(sem->notification));
    return 0;
}
#endif