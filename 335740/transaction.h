#pragma once

// Requested feature: pthread_rwlock_t
#ifndef _GNU_SOURCE
#define _GNU_SOURCE
#endif
#ifndef __USE_XOPEN2K
#define __USE_XOPEN2K
#endif

#include <stdbool.h>
#include <stdio.h>

#include "read-set.h"
#include "write-set.h"
#include "bloom-filter.h"

/**
 * @brief ...
 * ...
 */
struct transaction {
    bool is_ro;
    int rv;
    struct read_node* first_read_node;
    struct read_node* last_read_node;
    struct write_node* first_write_node;
    struct write_node* last_write_node;
    struct bloom_filter bloom_filter;
};

void transaction_init(struct transaction* transaction, bool is_ro);

void transaction_cleanup(struct transaction* transaction);



