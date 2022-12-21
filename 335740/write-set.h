#pragma once

#include <stdbool.h>
#include <stddef.h>
#include <stdlib.h>
#include <string.h>

#include "macros.h"

/**
 * @brief Holds address of a written word, value written to it, and a pointer to the next write node.
 */
struct write_node {
    void* address;
    void* value;
    struct write_node* next;
};

void write_set_init(struct write_node**, struct write_node**);

bool write_set_add(struct write_node**, struct write_node**, void*, const void*, size_t);

bool write_set_add_first(struct write_node**, struct write_node**, void*, const void*, size_t); // private fn

bool write_set_add_next(struct write_node**, void*, const void*, size_t); // private fn

bool write_node_create(struct write_node**, void*, const void*, size_t); // private fn

void write_node_overwrite(struct write_node*, const void*, size_t);

struct write_node* write_node_find(struct write_node*, const void*);

void write_set_cleanup(struct write_node*);
