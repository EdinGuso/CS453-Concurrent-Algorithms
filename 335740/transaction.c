#include "transaction.h"

void transaction_init(struct transaction* transaction, bool is_ro) {
    transaction->is_ro = is_ro;
    read_set_init(&transaction->first_read_node, &transaction->last_read_node);
    write_set_init(&transaction->first_write_node, &transaction->last_write_node);
}

void transaction_cleanup(struct transaction* transaction) {
    read_set_cleanup(transaction->first_read_node);
    write_set_cleanup(transaction->first_write_node);
}
