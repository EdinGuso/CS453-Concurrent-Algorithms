#include "read-set.h"

void read_set_init(struct read_node** first_node, struct read_node** last_node) {
    *first_node = NULL;
    *last_node = NULL;
}

bool read_set_add(struct read_node** first_node, struct read_node** last_node, const void* source_word) {
    if (*first_node == NULL) {
        return read_set_add_first(first_node, last_node, source_word);
    }
    return read_set_add_next(last_node, source_word);
}

bool read_set_add_first(struct read_node** first_node, struct read_node** last_node, const void* source_word) {
    struct read_node* new_node;
    if (unlikely(!read_node_create(&new_node, source_word))) {
        return false;
    }

    *first_node = new_node;
    *last_node = new_node;
    return true;
}

bool read_set_add_next(struct read_node** last_node, const void* source_word) {
    struct read_node* new_node;
    if (unlikely(!read_node_create(&new_node, source_word))) {
        return false;
    }

    (*last_node)->next = new_node;
    *last_node = new_node;
    return true;
}

bool read_node_create(struct read_node** new_node, const void* source_word) {
    *new_node = (struct read_node*) malloc(sizeof(struct read_node));
    if (unlikely(!new_node)) {
        return false;
    }

    (*new_node)->address = source_word;
    (*new_node)->next = NULL;
    return true;
}

void read_set_cleanup(struct read_node* first_node) {
    struct read_node* next_node;
    while (first_node != NULL) {
        next_node = first_node->next;
        free(first_node);
        first_node = next_node;
    }
}
