#pragma once

#include <stdbool.h>

#include "read-set.h"
#include "write-set.h"

/**
 * @brief Holds all the reads/writes performed by a transaction.
 */
struct transaction {
    bool is_ro;
    int rv;
    struct read_node* first_read_node;
    struct read_node* last_read_node;
    struct write_node* first_write_node;
    struct write_node* last_write_node;
};

void transaction_init(struct transaction* transaction, bool is_ro);

void transaction_cleanup(struct transaction* transaction);



