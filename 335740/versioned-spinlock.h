#pragma once

#include <stdatomic.h>
#include <emmintrin.h>
#include <stdbool.h>
#include <string.h>

/**
 * @brief Versioned spinlock implementation with bounded passive backoff spin.
 */
struct versioned_spinlock_t {
    _Atomic bool lock;
    int version;
};

void versioned_spinlock_init(struct versioned_spinlock_t* lock);

bool versioned_spinlock_acquire(struct versioned_spinlock_t* lock);

void versioned_spinlock_release(struct versioned_spinlock_t* lock);

void versioned_spinlock_update(struct versioned_spinlock_t* lock, int version);

bool versioned_spinlock_validate(struct versioned_spinlock_t* lock, int version);