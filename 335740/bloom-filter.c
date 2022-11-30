#include "bloom-filter.h"

void bloom_filter_init(struct bloom_filter* bloom_filter) {
    // printf("bloom_filter_init() called.\n");
    for (int i = 0; i < NUM_SLOTS; i++) {
        bloom_filter->segments[i] = NULL;
        bloom_filter->write_nodes[i] = NULL;
    }
    // printf("bloom_filter_init() terminated.\n");
}

int find_slot(const void* segment) {
    return (uintptr_t)segment % NUM_SLOTS;
}

void bloom_filter_add(struct bloom_filter* bloom_filter, const void* segment, void* write_node) {
    // printf("bloom_filter_add() called.\n");
    int slot = find_slot(segment);
    // printf("bloom_filter_add() value returned from find_slot() is %d.\n", slot);
    while (bloom_filter->segments[slot] != NULL) {
        // printf("bloom_filter_add() offsetting.\n");
        slot = (slot + 1) % NUM_SLOTS;
    }
    bloom_filter->segments[slot] = segment;
    bloom_filter->write_nodes[slot] = write_node;
    // printf("bloom_filter_add() terminated.\n");
}

void* bloom_filter_get(struct bloom_filter* bloom_filter, const void* segment) {
    // printf("bloom_filter_get() called.\n");
    int slot = find_slot(segment);
    while (bloom_filter->segments[slot] != segment) {
        if (bloom_filter->segments[slot] == NULL) {
            // printf("bloom_filter_get() terminated : return NULL.\n");
            return NULL;
        }
        // printf("bloom_filter_get() offsetting.\n");
        slot = (slot + 1) % NUM_SLOTS;
    }
    // printf("bloom_filter_get() terminated : return write_node.\n");
    return bloom_filter->write_nodes[slot];;
}
