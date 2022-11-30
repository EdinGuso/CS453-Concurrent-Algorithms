#pragma once

#ifndef _GNU_SOURCE
#define _GNU_SOURCE
#endif
#ifndef __USE_XOPEN2K
#define __USE_XOPEN2K
#endif

#include <stdbool.h>
#include <stdint.h>
#include <stddef.h>
#include <stdio.h>

#define NUM_SLOTS 10000 //upper bound on the number of segments a tx will write?

/**
 * @brief ...
 * ...
 */
struct bloom_filter {
    const void* segments[NUM_SLOTS];
    void* write_nodes[NUM_SLOTS];
};

void bloom_filter_init(struct bloom_filter* bloom_filter);

int find_slot(const void* shared);

//Always adds the write_node
void bloom_filter_add(struct bloom_filter* bloom_filter, const void* segment, void* write_node);

//Returns NULL if does not exist
//Always return write_node otherwise
void* bloom_filter_get(struct bloom_filter* bloom_filter, const void* segment);
