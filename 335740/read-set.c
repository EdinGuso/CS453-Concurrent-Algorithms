#include "read-set.h"

void read_set_init(struct read_node** first_node, struct read_node** last_node) {
    *first_node = NULL;
    *last_node = NULL;
}

bool read_set_add(struct read_node** first_node, struct read_node** last_node, const void* segment) {
    // printf("read_set_add() called.\n");
    if (*first_node == NULL) {
        return read_set_add_first(first_node, last_node, segment);
    }
    return read_set_add_next(last_node, segment);
}

bool read_set_add_first(struct read_node** first_node, struct read_node** last_node, const void* segment) {
    // printf("read_set_add_first() called.\n");
    struct read_node* new_node;
    if (!read_node_create(&new_node, segment)) {
        // printf("read_set_add_first() terminated : return false.\n");
        return false;
    }

    *first_node = new_node;
    *last_node = new_node;
    // printf("read_set_add_first() terminated : return true.\n");
    return true;
}

bool read_set_add_next(struct read_node** last_node, const void* segment) {
    // printf("read_set_add_next() called.\n");
    struct read_node* new_node;
    if (!read_node_create(&new_node, segment)) {
        // printf("read_set_add_next() terminated : return false.\n");
        return false;
    }

    (*last_node)->next = new_node;
    *last_node = new_node;
    // printf("read_set_add_next() terminated : return true.\n");
    return true;
}

bool read_node_create(struct read_node** new_node, const void* segment) {
    // printf("read_node_create() called.\n");
    *new_node = (struct read_node*) malloc(sizeof(struct read_node));
    if (unlikely(!new_node)) {
        // printf("read_node_create() terminated : return false.\n");
        return false;
    }

    (*new_node)->address = segment;
    (*new_node)->next = NULL;
    // printf("read_node_create() terminated : return true.\n");
    return true;
}

void read_set_cleanup(struct read_node* first_node) {
    // printf("read_set_cleanup() called.\n");
    int count = 0;
    while (first_node != NULL) {
        struct read_node* next_node = first_node->next;
        free(first_node);
        first_node = next_node;
        count++;
    }
    // printf("read_set_cleanup() terminated (freed %d read nodes).\n", count);
}
