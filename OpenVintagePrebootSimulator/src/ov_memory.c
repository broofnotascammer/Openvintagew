#include "ov_memory.h"
#include "ov_logger.h"
#include <malloc.h>

static size_t allocated_total = 0;
static size_t freed_total = 0;
static size_t peak_usage = 0;
static uint32_t allocation_count = 0;

ov_status_t ov_memory_init(size_t pool_size) {
    ov_log_info("Initializing memory pool with %zu bytes", pool_size);
    allocated_total = 0;
    freed_total = 0;
    peak_usage = 0;
    allocation_count = 0;
    return OV_SUCCESS;
}

void* ov_malloc(size_t size) {
    void *ptr = malloc(size);
    if (ptr) {
        allocated_total += size;
        allocation_count++;
        if (allocated_total - freed_total > peak_usage) {
            peak_usage = allocated_total - freed_total;
        }
        ov_log_debug("Allocated %zu bytes (total: %zu)", size, allocated_total);
    }
    return ptr;
}

void* ov_realloc(void *ptr, size_t size) {
    void *new_ptr = realloc(ptr, size);
    if (new_ptr) {
        allocated_total += size;
        if (allocated_total - freed_total > peak_usage) {
            peak_usage = allocated_total - freed_total;
        }
    }
    return new_ptr;
}

void ov_free(void *ptr) {
    if (ptr) {
        size_t size = malloc_usable_size(ptr);
        freed_total += size;
        free(ptr);
        ov_log_debug("Freed %zu bytes (total freed: %zu)", size, freed_total);
    }
}

ov_memory_stats_t ov_memory_get_stats(void) {
    ov_memory_stats_t stats = {
        .allocated = allocated_total,
        .freed = freed_total,
        .peak_usage = peak_usage,
        .num_allocations = allocation_count
    };
    return stats;
}

void ov_memory_print_stats(void) {
    ov_memory_stats_t stats = ov_memory_get_stats();
    ov_log_info("Memory Statistics:");
    ov_log_info("  Allocated: %zu bytes", stats.allocated);
    ov_log_info("  Freed: %zu bytes", stats.freed);
    ov_log_info("  Peak Usage: %zu bytes", stats.peak_usage);
    ov_log_info("  Allocations: %u", stats.num_allocations);
}

void ov_memory_cleanup(void) {
    ov_log_info("Memory subsystem cleanup");
}
