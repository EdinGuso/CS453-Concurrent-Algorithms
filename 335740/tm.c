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
 * Software Transactional Memory (STM) implementation using
 * Transactional Locking (TL2) algorithm.
**/

#define _GNU_SOURCE
#define _POSIX_C_SOURCE   200809L
#ifdef __STDC_NO_ATOMICS__
    #error Current C11 compiler does not support atomic operations
#endif

// External headers
#include <string.h>
#include <stdlib.h>

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
};
typedef struct segment_node* segment_list;

/**
 * @brief Simple Shared Memory Region (a.k.a Transactional Memory).
 */
struct region {
    struct shared_lock_t lock;  // Global lock
    void* start;                // Start of the shared memory region (i.e., of the non-deallocable memory segment)
    segment_list allocs;        // Shared memory segments dynamically allocated via tm_alloc within transactions
    size_t size;                // Size of the non-deallocable memory segment (in bytes)
    size_t align;               // Size of a word in the shared memory region (in bytes)
};



// Start of helper functions used to implement TL2

/** Unlock the written memory addresses.
 * @param lock              Global lock object stored in region
 * @param first_write_node  Pointer to the write node which contains the first memory address that needs to be unlocked
 * @param next_to_lock      Pointer to the write node which contains the last memory address that needs to be unlocked
 *                          -(NULL if all addresses stored in write set need to be unlocked)
**/
void unlock_write_set(struct shared_lock_t* lock, struct write_node* first_write_node, struct write_node* next_to_lock) {
    while (first_write_node != next_to_lock) {
        shared_lock_versioned_spinlock_release(lock, first_write_node->address);
        first_write_node = first_write_node->next; 
    }
}

/** Lock the written memory addresses.
 * @param lock              Global lock object stored in region
 * @param first_write_node  Pointer to the write node which contains the first memory address that needs to be locked
 * @return Whether the all the written addresses were locked successfully or not
**/
bool lock_write_set(struct shared_lock_t* lock, struct write_node* first_write_node) {
    struct write_node* begin = first_write_node;
    while (first_write_node != NULL) {
        if (!shared_lock_versioned_spinlock_acquire(lock, first_write_node->address)) {
            unlock_write_set(lock, begin, first_write_node);
            return false;
        }
        first_write_node = first_write_node->next;
    }
    return true;
}

/** Validate the read memory addresses.
 * @param lock              Global lock object stored in region
 * @param first_read_node  Pointer to the read node which contains the first memory address that needs to be validated
 * @param rv                Read version (according to TL2 algorithm)
 * @return Whether the all the read addresses were validated successfully or not
**/
bool validate_read_set(struct shared_lock_t* lock, struct read_node* first_read_node, int rv) {
    while (first_read_node != NULL) {
        if (!shared_lock_versioned_spinlock_validate(lock, first_read_node->address, rv)) {
            return false;
        }
        first_read_node = first_read_node->next;
    }
    return true;
}

/** Store the written data to the shared memory region.
 * @param lock              Global lock object stored in region
 * @param first_write_node  Pointer to the write node which contains the first memory address and the value written to it
 * @param size              Size of the memory region to be copied (equal to the alignment of the region)
 * @param wv                Write version (according to TL2 algorithm)
**/
void store_write_set(struct shared_lock_t* lock, struct write_node* first_write_node, size_t size, int wv) {
    while (first_write_node != NULL) {
        memcpy(first_write_node->address, first_write_node->value, size);
        shared_lock_versioned_spinlock_update(lock, first_write_node->address, wv);
        shared_lock_versioned_spinlock_release(lock, first_write_node->address);
        first_write_node = first_write_node->next;
    }
}

// End of helper functions used to implement TL2



/** Create (i.e. allocate + init) a new shared memory region, with one first non-free-able allocated segment of the requested size and alignment.
 * @param size  Size of the first shared segment of memory to allocate (in bytes), must be a positive multiple of the alignment
 * @param align Alignment (in bytes, must be a power of 2) that the shared memory region must support
 * @return Opaque shared memory region handle, 'invalid_shared' on failure
**/
shared_t tm_create(size_t size, size_t align) {
    struct region* region = (struct region*) malloc(sizeof(struct region));

    if (unlikely(!region)) {
        return invalid_shared;
    }

    if (unlikely(posix_memalign(&(region->start), align, size) != 0)) {
        free(region);
        return invalid_shared;
    }

    shared_lock_init(&(region->lock));
    memset(region->start, 0, size);
    region->allocs      = NULL;
    region->size        = size;
    region->align       = align;
    return region;
}

/** Destroy (i.e. clean-up + free) a given shared memory region.
 * @param shared Shared memory region to destroy, with no running transaction
**/
void tm_destroy(shared_t shared) {
    struct region* region = (struct region*) shared;

    while (region->allocs) { // Free allocated segments
        segment_list tail = region->allocs->next;
        free(region->allocs);
        region->allocs = tail;
    }

    shared_lock_cleanup(&region->lock);
    free(region->start);
    free(region);
}

/** [thread-safe] Return the start address of the first allocated segment in the shared memory region.
 * @param shared Shared memory region to query
 * @return Start address of the first allocated segment
**/
void* tm_start(shared_t shared) {
    return ((struct region*) shared)->start;
}

/** [thread-safe] Return the size (in bytes) of the first allocated segment of the shared memory region.
 * @param shared Shared memory region to query
 * @return First allocated segment size
**/
size_t tm_size(shared_t shared) {
    return ((struct region*) shared)->size;
}

/** [thread-safe] Return the alignment (in bytes) of the memory accesses on the given shared memory region.
 * @param shared Shared memory region to query
 * @return Alignment used globally
**/
size_t tm_align(shared_t shared) {
    return ((struct region*) shared)->align;
}

/** [thread-safe] Begin a new transaction on the given shared memory region.
 * @param shared Shared memory region to start a transaction on
 * @param is_ro  Whether the transaction is read-only
 * @return Opaque transaction ID, 'invalid_tx' on failure
**/
tx_t tm_begin(shared_t shared, bool is_ro) {
    struct region* region = (struct region*) shared;

    struct transaction* transaction = (struct transaction*) malloc(sizeof(struct transaction));

    if (unlikely(!transaction)) {
        return invalid_tx;
    }

    transaction_init(transaction, is_ro);
    transaction->rv = shared_lock_global_clock_get(&region->lock); // Sample the global clock and store it as read version
    return (tx_t)transaction; // Return a pointer to the transaction
}

/** [thread-safe] End the given transaction.
 * @param shared Shared memory region associated with the transaction
 * @param tx     Transaction to end
 * @return Whether the whole transaction committed
**/
bool tm_end(shared_t shared, tx_t tx) {
    struct transaction* transaction = (struct transaction*) tx;

    if (!transaction->is_ro) { // if it is a read-write transaction
        struct region* region = (struct region*) shared;

        if (!lock_write_set(&region->lock, transaction->first_write_node)) { // Attempt to lock the write set
            transaction_cleanup(transaction);
            free(transaction);
            return false;
        }

        int wv = shared_lock_global_clock_increment_and_get(&region->lock); // Sample the global clock and store it as write version

        if (wv != transaction->rv + 1) { // If write version is 1 more than read version, we do not need to perform any other validations
            if (!validate_read_set(&region->lock, transaction->first_read_node, transaction->rv)) { // Otherwise, we attempt to validate the read set
                unlock_write_set(&region->lock, transaction->first_write_node, NULL);
                transaction_cleanup(transaction);
                free(transaction);
                return false;
            }
        }
        
        store_write_set(&region->lock, transaction->first_write_node, region->align, wv); // Commit changes
        transaction_cleanup(transaction);
    }

    free(transaction);
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
    struct region* region = (struct region*) shared;
    struct transaction* transaction = (struct transaction*) tx;

    size_t num_bytes_read = 0;
    const void* source_word = source;
    void* target_word = target;
    do
    {
        if (transaction->is_ro) { // If it is a read-only transaction
            memcpy(target_word, source_word, region->align);
        }
        else { // If it is a read-write transaction
            struct write_node* old_node = write_node_find(transaction->first_write_node, source_word);

            if (old_node == NULL) { // If the address we are trying to read is not in the write set
                if (unlikely(!read_set_add(&transaction->first_read_node, &transaction->last_read_node, source_word))) {
                    transaction_cleanup(transaction);
                    free(transaction);
                    return false;
                }
                memcpy(target_word, source_word, region->align);
            }
            else { // If the address we are trying to read is in the write set
                memcpy(target_word, old_node->value, region->align);
            }
        }

        if (!shared_lock_versioned_spinlock_validate(&region->lock, source_word, transaction->rv)) { // Attempt to validate the address we read
            transaction_cleanup(transaction);
            free(transaction);
            return false;
        }

        source_word += region->align;
        target_word += region->align;
        num_bytes_read += region->align;
    } while (num_bytes_read < size); // Check if we have read the requested number of bytes

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
    struct region* region = (struct region*) shared;
    struct transaction* transaction = (struct transaction*) tx;

    size_t num_bytes_read = 0;
    const void* source_word = source;
    void* target_word = target;

    do
    {
        struct write_node* old_node = write_node_find(transaction->first_write_node, target_word);

        if (old_node == NULL) { // If the address we are trying to write is not in the write set
            if (unlikely(!write_set_add(&transaction->first_write_node, &transaction->last_write_node, target_word, source_word, region->align))) {
                transaction_cleanup(transaction);
                free(transaction);
                return false;
            }
        }
        else { // If the address we are trying to write is in the write set
            write_node_overwrite(old_node, source_word, region->align);
        }

        source_word += region->align;
        target_word += region->align;
        num_bytes_read += region->align;
    } while (num_bytes_read < size); // Check if we have written the requested number of bytes
    
    return true;
}

/** [thread-safe] Memory allocation in the given transaction.
 * @param shared Shared memory region associated with the transaction
 * @param tx     Transaction to use
 * @param size   Allocation requested size (in bytes), must be a positive multiple of the alignment
 * @param target Pointer in private memory receiving the address of the first byte of the newly allocated, aligned segment
 * @return Whether the whole transaction can continue (success/nomem), or not (abort_alloc)
**/
alloc_t tm_alloc(shared_t shared, tx_t unused(tx), size_t size, void** target) {
    struct region* region = (struct region*) shared;

    size_t align = region->align;
    align = align < sizeof(struct segment_node*) ? sizeof(void*) : align;

    struct segment_node* sn;
    if (unlikely(posix_memalign((void**)&sn, align, sizeof(struct segment_node) + size) != 0)) {
        return nomem_alloc;
    }
    sn->prev = NULL;

    shared_lock_segment_lock_acquire(&region->lock); // Only one transaction is allowed to add allocated segments to the region at a time
    sn->next = region->allocs;
    if (sn->next) sn->next->prev = sn;
    region->allocs = sn;
    shared_lock_segment_lock_release(&region->lock);

    void* segment = (void*) ((uintptr_t) sn + sizeof(struct segment_node));
    memset(segment, 0, size);
    *target = segment;

    return success_alloc;
}

/** [thread-safe] Memory freeing in the given transaction.
 * @param shared Shared memory region associated with the transaction
 * @param tx     Transaction to use
 * @param target Address of the first byte of the previously allocated segment to deallocate
 * @return Whether the whole transaction can continue
**/
bool tm_free(shared_t unused(shared), tx_t unused(tx), void* unused(segment)) {
    return true; // No need to free anything here. All the allocated segments will be freed upon tm_destroy call
}
