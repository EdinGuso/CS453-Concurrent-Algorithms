#pragma once

#ifndef _GNU_SOURCE
#define _GNU_SOURCE
#endif
#ifndef __USE_XOPEN2K
#define __USE_XOPEN2K
#endif

#include <stdatomic.h>
#include <stdbool.h>
#include <string.h>
#include <stdio.h>
#include <emmintrin.h>

/**
 * @brief ...
 * ...
 */
struct versioned_spinlock_t {
    _Atomic bool lock;
    _Atomic int version;
};

void versioned_spinlock_init(struct versioned_spinlock_t* lock);

bool versioned_spinlock_acquire(struct versioned_spinlock_t* lock);

void versioned_spinlock_release(struct versioned_spinlock_t* lock);

int versioned_spinlock_get(struct versioned_spinlock_t* lock);

void versioned_spinlock_update(struct versioned_spinlock_t* lock, int version);

bool versioned_spinlock_validate(struct versioned_spinlock_t* lock, int version);