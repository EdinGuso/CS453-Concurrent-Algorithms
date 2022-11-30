#include "versioned-spinlock.h"

void versioned_spinlock_init(struct versioned_spinlock_t* lock) {
    atomic_store(&lock->lock, false);
    atomic_store(&lock->version, 0);
}

bool versioned_spinlock_acquire(struct versioned_spinlock_t* lock) {
    //bounded passive backoff spin
    int bound = 0;
    while (1)
    {
        if (!atomic_exchange(&lock->lock, true)) return true;

        do
        {
            if (bound > 10) return false;
            for (int i = 0; i < 4; i++) _mm_pause();
            bound++;
        } while (atomic_load(&lock->lock));
    }
}

void versioned_spinlock_release(struct versioned_spinlock_t* lock) {
    atomic_store(&lock->lock, false);
}

int versioned_spinlock_get(struct versioned_spinlock_t* lock) {
    return atomic_load(&lock->version);
}

void versioned_spinlock_update(struct versioned_spinlock_t* lock, int version) {
    atomic_store(&lock->version, version);
}

bool versioned_spinlock_validate(struct versioned_spinlock_t* lock, int version) {
    if (atomic_load(&lock->lock)) {
        return false;
    }

    if (atomic_load(&lock->version) > version) {
        return false;
    }

    return true;
}