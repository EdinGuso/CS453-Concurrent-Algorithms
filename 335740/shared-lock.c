#include "shared-lock.h"

void shared_lock_init(struct shared_lock_t* lock) {
    atomic_store(&lock->clock, 0);
    for (int i = 0; i < NUM_LOCKS; i++) {
        versioned_spinlock_init(&lock->locks[i]);
    }
    pthread_mutex_init(&lock->segment_lock, NULL);
}

int shared_lock_global_clock_get(struct shared_lock_t* lock) {
    return atomic_load(&lock->clock);
}

int shared_lock_global_clock_increment_and_get(struct shared_lock_t* lock) {
    return __atomic_add_fetch(&lock->clock, 1, __ATOMIC_RELAXED);
}

int find_lock(const void* shared) {
    return (uintptr_t)shared % NUM_LOCKS;
}

bool shared_lock_versioned_spinlock_acquire(struct shared_lock_t* lock, const void* shared) {
    return versioned_spinlock_acquire(&lock->locks[find_lock(shared)]);
}

void shared_lock_versioned_spinlock_release(struct shared_lock_t* lock, const void* shared) {
    versioned_spinlock_release(&lock->locks[find_lock(shared)]);
}

void shared_lock_versioned_spinlock_update(struct shared_lock_t* lock, const void* shared, int version) {
    versioned_spinlock_update(&lock->locks[find_lock(shared)], version);
}

bool shared_lock_versioned_spinlock_validate(struct shared_lock_t* lock, const void* shared, int version) {
    return versioned_spinlock_validate(&lock->locks[find_lock(shared)], version);
}

void shared_lock_segment_lock_acquire(struct shared_lock_t* lock) {
    pthread_mutex_lock(&lock->segment_lock);
}

void shared_lock_segment_lock_release(struct shared_lock_t* lock) {
    pthread_mutex_unlock(&lock->segment_lock);
}

void shared_lock_cleanup(struct shared_lock_t* lock) {
    pthread_mutex_destroy(&lock->segment_lock);
}