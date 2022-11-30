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
#include <stdio.h>

#include "macros.h"

/**
 * @brief ...
 * ...
 */
struct read_node {
    const void* address;
    struct read_node* next;
};

void read_set_init(struct read_node**, struct read_node**);

bool read_set_add(struct read_node**, struct read_node**, const void*);

//private function
bool read_set_add_first(struct read_node**, struct read_node**, const void*);

//private function
bool read_set_add_next(struct read_node**, const void*);

//private function
bool read_node_create(struct read_node**, const void*);

void read_set_cleanup(struct read_node*);


