#pragma once

#include <stdbool.h>
#include <stdlib.h>

#include "macros.h"

/**
 * @brief Holds address of a read word and a pointer to the next read node.
 */
struct read_node {
    const void* address;
    struct read_node* next;
};

void read_set_init(struct read_node** first_node, struct read_node** last_node);

bool read_set_add(struct read_node** first_node, struct read_node** last_node, const void* source_word);

bool read_set_add_first(struct read_node** first_node, struct read_node** last_node, const void* source_word); // private fn

bool read_set_add_next(struct read_node** last_node, const void* source_word); // private fn

bool read_node_create(struct read_node** new_node, const void* source_word); // private fn

void read_set_cleanup(struct read_node* first_node);


