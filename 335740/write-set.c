#include "write-set.h"

void write_set_init(struct write_node** first_node, struct write_node** last_node) {
    *first_node = NULL;
    *last_node = NULL;
}

bool write_set_add(struct write_node** first_node, struct write_node** last_node, void* target_word, const void* source_word, size_t size) {
    if (*first_node == NULL) {
        return write_set_add_first(first_node, last_node, target_word, source_word, size);
    }
    return write_set_add_next(last_node, target_word, source_word, size);
}

bool write_set_add_first(struct write_node** first_node, struct write_node** last_node, void* target_word, const void* source_word, size_t size) {
    struct write_node* new_node;
    if (unlikely(!write_node_create(&new_node, target_word, source_word, size))) {
        return false;
    }

    *first_node = new_node;
    *last_node = new_node;
    return true;
}

bool write_set_add_next(struct write_node** last_node, void* target_word, const void* source_word, size_t size) {
    struct write_node* new_node;
    if (unlikely(!write_node_create(&new_node, target_word, source_word, size))) {
        return false;
    }

    (*last_node)->next = new_node;
    *last_node = new_node;
    return true;
}

bool write_node_create(struct write_node** new_node, void* target_word, const void* source_word, size_t size) {
    *new_node = (struct write_node*) malloc(sizeof(struct write_node));
    if (unlikely(!new_node)) {
        return false;
    }
    (*new_node)->address = target_word;
    (*new_node)->value = (void*) malloc(size);
    if (unlikely(!(*new_node)->value)) {
        return false;
    }

    memcpy((*new_node)->value, source_word, size);
    (*new_node)->next = NULL;
    return true;
}

void write_node_overwrite(struct write_node* node, const void* source_word, size_t size) {
    memcpy(node->value, source_word, size);
}

struct write_node* write_node_find(struct write_node* first_node, const void* target_word) {
    while(first_node != NULL) {
        if(first_node->address == target_word) {
            return first_node;
        }
        first_node = first_node->next;
    }
    return NULL;
}

void write_set_cleanup(struct write_node* first_node) {
    struct write_node* next_node;
    while (first_node != NULL) {
        next_node = first_node->next;
        free(first_node->value);
        free(first_node);
        first_node = next_node;
    }
}
