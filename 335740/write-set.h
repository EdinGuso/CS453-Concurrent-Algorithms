#pragma once

#ifndef _GNU_SOURCE
#define _GNU_SOURCE
#endif
#ifndef __USE_XOPEN2K
#define __USE_XOPEN2K
#endif

#include <stdbool.h>
#include <stddef.h>
#include <stdlib.h>
#include <string.h>
#include <stdio.h>

#include "macros.h"

/**
 * @brief ...
 * ...
 */
struct write_node { //memory is structured [segment, actual value]
    void* address; //pointer to shared memory region
    void* value; //pointer to value 
    struct write_node* next;
};

void write_set_init(struct write_node**, struct write_node**);

bool write_set_add(struct write_node**, struct write_node**, void*, const void*, size_t);

//private function
bool write_set_add_first(struct write_node**, struct write_node**, void*, const void*, size_t);

//private function
bool write_set_add_next(struct write_node**, void*, const void*, size_t);

//private function
bool write_node_create(struct write_node**, void*, const void*, size_t);

void write_node_overwrite(struct write_node*, const void*, size_t);

void write_set_cleanup(struct write_node*);
