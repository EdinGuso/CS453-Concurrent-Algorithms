#include "write-set.h"

void write_set_init(struct write_node** first_node, struct write_node** last_node) {
    *first_node = NULL;
    *last_node = NULL;
}

bool write_set_add(struct write_node** first_node, struct write_node** last_node, void* segment, const void* source, size_t size) {
    // printf("write_set_add() called.\n");
    if (*first_node == NULL) {
        return write_set_add_first(first_node, last_node, segment, source, size);
    }
    return write_set_add_next(last_node, segment, source, size);
}

bool write_set_add_first(struct write_node** first_node, struct write_node** last_node, void* segment, const void* source, size_t size) {
    // printf("write_set_add_first() called.\n");
    struct write_node* new_node;
    if (!write_node_create(&new_node, segment, source, size)) {
        // printf("write_set_add_first() terminated : return false.\n");
        return false;
    }

    *first_node = new_node;
    *last_node = new_node;
    // printf("write_set_add_first() terminated : return true.\n");
    return true;
}

bool write_set_add_next(struct write_node** last_node, void* segment, const void* source, size_t size) {
    // printf("write_set_add_next() called.\n");
    struct write_node* new_node;
    if (!write_node_create(&new_node, segment, source, size)) {
        // printf("write_set_add_next() terminated : return false.\n");
        return false;
    }

    (*last_node)->next = new_node;
    *last_node = new_node;
    // printf("write_set_add_next() terminated : return true.\n");
    return true;
}

bool write_node_create(struct write_node** new_node, void* segment, const void* source, size_t size) {
    // printf("write_node_create() called.\n");
    *new_node = (struct write_node*) malloc(sizeof(struct write_node));
    if (unlikely(!new_node)) {
        // printf("write_node_create() terminated : return false (malloc write node).\n");
        return false;
    }
    (*new_node)->address = segment;
    (*new_node)->value = (void*) malloc(size);
    if (unlikely(!(*new_node)->value)) {
        // printf("write_node_create() terminated : return false (malloc size).\n");
        return false;
    }
    memcpy((*new_node)->value, source, size);
    (*new_node)->next = NULL;
    // printf("write_node_create() terminated : return true.\n");
    return true;
}

void write_node_overwrite(struct write_node* node, const void* source, size_t size) {
    memcpy(node->value, source, size);
}

void write_set_cleanup(struct write_node* first_node) {
    // printf("write_set_cleanup() called.\n");
    int count = 0;
    struct write_node* iterator = first_node;
    while (iterator != NULL) {
        struct write_node* next_node = iterator->next;
        free(iterator->value);
        free(iterator);
        iterator = next_node;
        count++;
    }
    // printf("write_set_cleanup() terminated (freed %d write nodes).\n", count);
}
