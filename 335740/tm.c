/**
 * @file   tm.c
 * @author Edin Guso <edin.guso@epfl.ch>
 *
 * @section LICENSE
 *
 * Copyright © 2022 Edin Guso.
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 3 of the License, or
 * any later version. Please see https://gnu.org/licenses/gpl.html
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 * GNU General Public License for more details.
 *
 * @section DESCRIPTION
 *
 * TL2 implementation.
**/

#define _GNU_SOURCE
#define _POSIX_C_SOURCE   200809L
#ifdef __STDC_NO_ATOMICS__
    #error Current C11 compiler does not support atomic operations
#endif

// External headers
#include <string.h>
#include <stdlib.h>
#include <stdio.h>
#include <stdatomic.h>

// Internal headers
#include <tm.h>
#include "macros.h"
#include "shared-lock.h"
#include "transaction.h"

/**
 * @brief List of dynamically allocated segments.
 */
struct segment_node {
    struct segment_node* prev;
    struct segment_node* next;
    // uint8_t segment[] // segment of dynamic size
};
typedef struct segment_node* segment_list;

/**
 * @brief Simple Shared Memory Region (a.k.a Transactional Memory).
 */
struct region {
    struct shared_lock_t lock; // Global lock
    void* start;        // Start of the shared memory region (i.e., of the non-deallocable memory segment)
    segment_list allocs; // Shared memory segments dynamically allocated via tm_alloc within transactions
    size_t size;        // Size of the non-deallocable memory segment (in bytes)
    size_t align;       // Size of a word in the shared memory region (in bytes)
};




// HELPER FUNCS START

void unlock_write_set(struct shared_lock_t* lock, struct write_node* first_write_node, void* next_to_lock) {
    // printf("unlock_write_set() called.\n");
    struct write_node* iterator = first_write_node;
    int count = 0;
    while (iterator != next_to_lock) {
        shared_lock_versioned_spinlock_release(lock, iterator->address);
        iterator = iterator->next; 
        count++;
    }
    // printf("unlock_write_set() terminated (unlocked %d locks).\n", count);
}

bool lock_write_set(struct shared_lock_t* lock, struct write_node* first_write_node) {
    // printf("lock_write_set() called.\n");
    struct write_node* iterator = first_write_node;
    int count = 0;
    while (iterator != NULL) {
        if (!shared_lock_versioned_spinlock_acquire(lock, iterator->address)) {
            unlock_write_set(lock, first_write_node, iterator);
            // printf("lock_write_set() terminated : return false (locked %d locks).\n", count);
            iterator = first_write_node;
            while (iterator != NULL) {
                iterator = iterator->next;
            }
            return false;
        }
        iterator = iterator->next;
        count++;
    }

    // printf("lock_write_set() terminated : return true (locked %d locks).\n", count);
    return true;
}

bool validate_read_set(struct shared_lock_t* lock, struct read_node* first_read_node, int rv) {
    // printf("validate_read_set() called.\n");
    struct read_node* iterator = first_read_node;
    while (iterator != NULL) {
        if (!shared_lock_versioned_spinlock_validate(lock, iterator->address, rv)) {
            // printf("validate_read_set() terminated : return false.\n");
            return false;
        }
        iterator = iterator->next;
    }
    // printf("validate_read_set() terminated : return true.\n");
    return true;
}

void store_write_set(struct shared_lock_t* lock, struct write_node* first_write_node, size_t size, int wv) {
    // printf("store_write_set() called.\n");
    struct write_node* iterator = first_write_node;
    int count = 0;
    while (iterator != NULL) {
        memcpy(iterator->address, iterator->value, size);
        shared_lock_versioned_spinlock_update(lock, iterator->address, wv);
        shared_lock_versioned_spinlock_release(lock, iterator->address);
        iterator = iterator->next;
        count++;
    }
    // printf("store_write_set() terminated (unlocked %d locks).\n", count);
}

// HELPER FUNCS END




/** Create (i.e. allocate + init) a new shared memory region, with one first non-free-able allocated segment of the requested size and alignment.
 * @param size  Size of the first shared segment of memory to allocate (in bytes), must be a positive multiple of the alignment
 * @param align Alignment (in bytes, must be a power of 2) that the shared memory region must support
 * @return Opaque shared memory region handle, 'invalid_shared' on failure
**/
shared_t tm_create(size_t size, size_t align) {
    printf("tm_create() called.\n");
    struct region* region = (struct region*) malloc(sizeof(struct region));

    if (unlikely(!region)) {
        printf("tm_create() terminated : return invalid_shared (malloc fail).\n");
        return invalid_shared;
    }
    // We allocate the shared memory buffer such that its words are correctly
    // aligned.
    if (posix_memalign(&(region->start), align, size) != 0) {
        free(region);
        printf("tm_create() terminated : return invalid_shared (posix_memalign fail).\n");
        return invalid_shared;
    }

    shared_lock_init(&(region->lock));
    
    memset(region->start, 0, size);

    region->allocs      = NULL;
    region->size        = size;
    region->align       = align;

    printf("tm_create() terminated : return region.\n");
    return region;
}

/** Destroy (i.e. clean-up + free) a given shared memory region.
 * @param shared Shared memory region to destroy, with no running transaction
**/
void tm_destroy(shared_t shared) {
    // Note: To be compatible with any implementation, shared_t is defined as a
    // void*. For this particular implementation, the "real" type of a shared_t
    // is a struct region*.
    printf("tm_destroy () called.\n");
    struct region* region = (struct region*) shared;
    while (region->allocs) { // Free allocated segments
        segment_list tail = region->allocs->next;
        free(region->allocs);
        region->allocs = tail;
    }
    free(region->start);
    free(region);

    printf("tm_destroy() terminated.\n");
}

/** [thread-safe] Return the start address of the first allocated segment in the shared memory region.
 * @param shared Shared memory region to query
 * @return Start address of the first allocated segment
**/
void* tm_start(shared_t shared) {
    // printf("tm_start() called.\n");
    // printf("tm_start() terminated : return start.\n");
    return ((struct region*) shared)->start;
}

/** [thread-safe] Return the size (in bytes) of the first allocated segment of the shared memory region.
 * @param shared Shared memory region to query
 * @return First allocated segment size
**/
size_t tm_size(shared_t shared) {
    // printf("tm_size() called.\n");
    // printf("tm_size() terminated : return size.\n");
    return ((struct region*) shared)->size;
}

/** [thread-safe] Return the alignment (in bytes) of the memory accesses on the given shared memory region.
 * @param shared Shared memory region to query
 * @return Alignment used globally
**/
size_t tm_align(shared_t shared) {
    // printf("tm_align() called.\n");
    // printf("tm_align() terminated : return align.\n");
    return ((struct region*) shared)->align;
}

/** [thread-safe] Begin a new transaction on the given shared memory region.
 * @param shared Shared memory region to start a transaction on
 * @param is_ro  Whether the transaction is read-only
 * @return Opaque transaction ID, 'invalid_tx' on failure
**/
tx_t tm_begin(shared_t shared, bool is_ro) {
    // printf("tm_begin() called.\n");
    struct region* region = (struct region*) shared;
    struct transaction* transaction = (struct transaction*) malloc(sizeof(struct transaction));

    if (unlikely(!transaction)) {
        printf("tm_begin() terminated : return invalid_tx.\n");
        return invalid_tx;
    }

    transaction_init(transaction, is_ro);

    transaction->rv = shared_lock_global_clock_get(&region->lock);

    // printf("tm_begin() terminated : return transaction.\n");
    return (tx_t)transaction;
}

/** [thread-safe] End the given transaction.
 * @param shared Shared memory region associated with the transaction
 * @param tx     Transaction to end
 * @return Whether the whole transaction committed
**/
bool tm_end(shared_t shared, tx_t tx) {
    // printf("tm_end() called.\n");
    struct region* region = (struct region*) shared;
    struct transaction* transaction = (struct transaction*) tx;

    if (!transaction->is_ro) {
        // printf("tm_end() read-write.\n");
        if (!lock_write_set(&region->lock, transaction->first_write_node)) {
            transaction_cleanup(transaction);
            free(transaction);
            // printf("tm_end() terminated : return false (lock write set failed).\n");
            return false;
        }

        int wv = shared_lock_global_clock_increment_and_get(&region->lock);

        if (wv != transaction->rv + 1) {
            if (!validate_read_set(&region->lock, transaction->first_read_node, transaction->rv)) {
                unlock_write_set(&region->lock, transaction->first_write_node, NULL);
                transaction_cleanup(transaction);
                free(transaction);
                // printf("tm_end() terminated : return false (validate read set failed).\n");
                return false;
            }
        }

        store_write_set(&region->lock, transaction->first_write_node, region->align, wv); //commit

        transaction_cleanup(transaction);
        free(transaction);
    }
    else {
        // printf("tm_end() read-only (do nothing).\n");
        free(transaction);
    }

    return true;
}

/** [thread-safe] Read operation in the given transaction, source in the shared region and target in a private region.
 * @param shared Shared memory region associated with the transaction
 * @param tx     Transaction to use
 * @param source Source start address (in the shared region)
 * @param size   Length to copy (in bytes), must be a positive multiple of the alignment
 * @param target Target start address (in a private region)
 * @return Whether the whole transaction can continue
**/
bool tm_read(shared_t shared, tx_t tx, void const* source, size_t size, void* target) {
    // printf("tm_read() called : source=%p.\n", source);

    struct region* region = (struct region*) shared;
    struct transaction* transaction = (struct transaction*) tx;

    int num_segments = size / region->align;
    // printf("tm_read() number of segments is %d.\n", num_segments);

    for (int i = 0; i < num_segments; i++) {
        // printf("tm_read() processing segment %d.\n", i);

        const void* cur_segment = source + i * region->align;
        void* cur_target = target + i * region->align;

        int pre_version = shared_lock_versioned_spinlock_get(&region->lock, cur_segment);

        if (transaction->is_ro) { //if it is a read-only transaction
            // printf("tm_read() read-only.\n");
            memcpy(cur_target, cur_segment, region->align);
        }
        else { //otherwise
            // printf("tm_read() read-write.\n");
            struct write_node* old_node = (struct write_node*)bloom_filter_get(&transaction->bloom_filter, cur_segment);

            if (old_node == NULL) { //it is not in the write set
                // printf("tm_read() bloom filter miss.\n");
                if (!read_set_add(&transaction->first_read_node, &transaction->last_read_node, cur_segment)) {
                    printf("tm_read() terminated : return false (read_set_add()).\n");
                    transaction_cleanup(transaction);
                    free(transaction);
                    return false;
                }
                memcpy(cur_target, cur_segment, region->align);
            }
            else { //it is in the write set
                // printf("tm_read() bloom filter hit.\n");
                memcpy(cur_target, old_node->value, region->align);
            }
        }

        int post_version = shared_lock_versioned_spinlock_get(&region->lock, cur_segment);

        if (post_version != pre_version) {
            // printf("tm_read() terminated : return false (version conflict).\n");
            transaction_cleanup(transaction);
            free(transaction);
            return false;
        }

        if (!shared_lock_versioned_spinlock_validate(&region->lock, cur_segment, transaction->rv)) {
            // printf("tm_read() terminated : return false (lock validation fail).\n");
            transaction_cleanup(transaction);
            free(transaction);
            return false;
        }
    }
    // printf("tm_read() terminated : return true.\n");
    return true;
}

/** [thread-safe] Write operation in the given transaction, source in a private region and target in the shared region.
 * @param shared Shared memory region associated with the transaction
 * @param tx     Transaction to use
 * @param source Source start address (in a private region)
 * @param size   Length to copy (in bytes), must be a positive multiple of the alignment
 * @param target Target start address (in the shared region)
 * @return Whether the whole transaction can continue
**/
bool tm_write(shared_t shared, tx_t tx, void const* source, size_t size, void* target) {
    // printf("tm_write() called : target=%p.\n", target);

    struct region* region = (struct region*) shared;
    struct transaction* transaction = (struct transaction*) tx;

    int num_segments = size / region->align;
    // printf("tm_write() number of segments is %d.\n", num_segments);

    for (int i = 0; i < num_segments; i++) {
        // printf("tm_write() processing segment %d.\n", i);

        void* cur_segment = target + i * region->align;
        const void* cur_source = source + i * region->align;

        // printf("tm_write() checkpoint1.\n");
        struct write_node* old_node = (struct write_node*)bloom_filter_get(&transaction->bloom_filter, cur_segment);
        // printf("tm_write() checkpoint2.\n");

        if (old_node == NULL) { //new write
            // printf("tm_write() creating new write node.\n");
            if (!write_set_add(&transaction->first_write_node, &transaction->last_write_node, cur_segment, cur_source, region->align)) {
                printf("tm_write() terminated : return false.\n");
                transaction_cleanup(transaction);
                free(transaction);
                return false;
            }
            bloom_filter_add(&transaction->bloom_filter, cur_segment, (void*)(transaction->last_write_node));
        }
        else { //overwrite
            // printf("tm_write() overwriting old write node.\n");
            write_node_overwrite(old_node, cur_source, region->align);
        }
    }
    // printf("tm_write() terminated : return true.\n");
    return true;
}

/** [thread-safe] Memory allocation in the given transaction.
 * @param shared Shared memory region associated with the transaction
 * @param tx     Transaction to use
 * @param size   Allocation requested size (in bytes), must be a positive multiple of the alignment
 * @param target Pointer in private memory receiving the address of the first byte of the newly allocated, aligned segment
 * @return Whether the whole transaction can continue (success/nomem), or not (abort_alloc)
**/
alloc_t tm_alloc(shared_t unused(shared), tx_t unused(tx), size_t unused(size), void** unused(target)) {
    printf("tm_alloc() called.\n");
    printf("tm_alloc() terminated : return abort_alloc.\n");
    return abort_alloc;
}

/** [thread-safe] Memory freeing in the given transaction.
 * @param shared Shared memory region associated with the transaction
 * @param tx     Transaction to use
 * @param target Address of the first byte of the previously allocated segment to deallocate
 * @return Whether the whole transaction can continue
**/
bool tm_free(shared_t unused(shared), tx_t unused(tx), void* unused(segment)) {
    printf("tm_free() called.\n");
    printf("tm_free() terminated : return false.\n");
    return false;
}
