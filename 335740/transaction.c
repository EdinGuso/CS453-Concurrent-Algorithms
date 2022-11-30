#include "transaction.h"

void transaction_init(struct transaction* transaction, bool is_ro) {
    // printf("transaction_init() called.\n");
    transaction->is_ro = is_ro;
    // printf("init() checkpoint1.\n");

    transaction->rv = 0;
    // printf("init() checkpoint2.\n");
    read_set_init(&transaction->first_read_node, &transaction->last_read_node);
    // printf("init() checkpoint3.\n");
    write_set_init(&transaction->first_write_node, &transaction->last_write_node);
    // printf("init() checkpoint4.\n");
    bloom_filter_init(&transaction->bloom_filter);
    // printf("init() checkpoint5.\n");
    // printf("transaction_init() terminated.\n");
}

void transaction_cleanup(struct transaction* transaction) {
    // printf("transaction_cleanup() called.\n");
    read_set_cleanup(transaction->first_read_node);
    write_set_cleanup(transaction->first_write_node);
    // printf("transaction_cleanup() terminated.\n");
}

