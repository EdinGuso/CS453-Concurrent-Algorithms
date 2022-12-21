#pragma once

#include <stdint.h>
#include <stdbool.h>
#include <pthread.h>
#include <stdatomic.h>

#include "versioned-spinlock.h"

#define NUM_LOCKS 4999

/**
 * @brief Global lock object that controls access to shared memory.
 */
struct shared_lock_t {
    _Atomic int clock;
    struct versioned_spinlock_t locks[NUM_LOCKS];
    pthread_mutex_t segment_lock;
};

void shared_lock_init(struct shared_lock_t* lock);

int shared_lock_global_clock_get(struct shared_lock_t* lock);

int shared_lock_global_clock_increment_and_get(struct shared_lock_t* lock);

int find_lock(const void* shared);

bool shared_lock_versioned_spinlock_acquire(struct shared_lock_t* lock, const void* shared);

void shared_lock_versioned_spinlock_release(struct shared_lock_t* lock, const void* shared);

void shared_lock_versioned_spinlock_update(struct shared_lock_t* lock, const void* shared, int version);

bool shared_lock_versioned_spinlock_validate(struct shared_lock_t* lock, const void* shared, int version);

void shared_lock_segment_lock_acquire(struct shared_lock_t* lock);

void shared_lock_segment_lock_release(struct shared_lock_t* lock);

void shared_lock_cleanup(struct shared_lock_t* lock);