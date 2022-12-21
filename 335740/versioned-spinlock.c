#include "versioned-spinlock.h"

void versioned_spinlock_init(struct versioned_spinlock_t* lock) {
    atomic_store(&lock->lock, false);
    lock->version = 0;
}

bool versioned_spinlock_acquire(struct versioned_spinlock_t* lock) {
    int bound = 0;
    while (true)
    {
        if (!atomic_exchange(&lock->lock, true)) return true;

        do
        {
            if (bound > 3) return false;
            for (int i = 0; i < 4; i++) _mm_pause();
            bound++;
        } while (atomic_load(&lock->lock));
    }
}

void versioned_spinlock_release(struct versioned_spinlock_t* lock) {
    atomic_store(&lock->lock, false);
}

void versioned_spinlock_update(struct versioned_spinlock_t* lock, int version) {
    lock->version = version;
}

bool versioned_spinlock_validate(struct versioned_spinlock_t* lock, int version) {
    if (atomic_load(&lock->lock)) {
        return false;
    }

    if (lock->version > version) {
        return false;
    }

    return true;
}