/**
 * OpenVintage Pre-Boot Simulator - Safe Memory Subsystem Implementation
 * Strict, portable tracking without non-standard malloc.h or platform-specific extensions.
 */

#include "ov_memory.h"
#include "ov_logger.h"
#include <stdlib.h>
#include <string.h>

#define OV_MEM_MAGIC 0x4F564D45 /* "OVME" */

typedef struct {
    uint32_t magic;
    uint32_t flags;
    size_t   size;
} ov_mem_header_t;

static size_t   g_allocated_total = 0;
static size_t   g_freed_total = 0;
static size_t   g_peak_usage = 0;
static uint32_t g_allocation_count = 0;
static uint32_t g_free_count = 0;

ov_status_t ov_memory_init(size_t pool_size) {
    (void)pool_size;
    g_allocated_total = 0;
    g_freed_total = 0;
    g_peak_usage = 0;
    g_allocation_count = 0;
    g_free_count = 0;
    ov_log_info("Memory subsystem initialized (portable header tracking)");
    return OV_SUCCESS;
}

void* ov_malloc(size_t size) {
    if (size == 0) {
        return NULL;
    }

    size_t total_size = sizeof(ov_mem_header_t) + size;
    void *raw = malloc(total_size);
    if (!raw) {
        ov_log_error("Out of memory attempting to allocate %zu bytes", size);
        return NULL;
    }

    ov_mem_header_t *hdr = (ov_mem_header_t*)raw;
    hdr->magic = OV_MEM_MAGIC;
    hdr->flags = 0;
    hdr->size  = size;

    g_allocated_total += size;
    g_allocation_count++;

    size_t current = (g_allocated_total > g_freed_total) ? (g_allocated_total - g_freed_total) : 0;
    if (current > g_peak_usage) {
        g_peak_usage = current;
    }

    return (void*)((uint8_t*)raw + sizeof(ov_mem_header_t));
}

void* ov_calloc(size_t num, size_t size) {
    size_t total = num * size;
    void *ptr = ov_malloc(total);
    if (ptr) {
        memset(ptr, 0, total);
    }
    return ptr;
}

void* ov_realloc(void *ptr, size_t size) {
    if (!ptr) {
        return ov_malloc(size);
    }
    if (size == 0) {
        ov_free(ptr);
        return NULL;
    }

    ov_mem_header_t *old_hdr = (ov_mem_header_t*)((uint8_t*)ptr - sizeof(ov_mem_header_t));
    if (old_hdr->magic != OV_MEM_MAGIC) {
        /* Unrecognized pointer or corrupted header, fallback to system realloc */
        return realloc(ptr, size);
    }

    size_t old_size = old_hdr->size;
    void *new_ptr = ov_malloc(size);
    if (!new_ptr) {
        return NULL;
    }

    size_t copy_size = (old_size < size) ? old_size : size;
    memcpy(new_ptr, ptr, copy_size);
    ov_free(ptr);

    return new_ptr;
}

void ov_free(void *ptr) {
    if (!ptr) {
        return;
    }

    ov_mem_header_t *hdr = (ov_mem_header_t*)((uint8_t*)ptr - sizeof(ov_mem_header_t));
    if (hdr->magic != OV_MEM_MAGIC) {
        /* External or corrupted allocation, pass to free directly */
        free(ptr);
        return;
    }

    size_t alloc_size = hdr->size;
    hdr->magic = 0xDEADBEEF; /* Invalidate header magic to catch double-frees */

    g_freed_total += alloc_size;
    g_free_count++;

    free(hdr);
}

ov_memory_stats_t ov_memory_get_stats(void) {
    size_t current = (g_allocated_total >= g_freed_total) ? (g_allocated_total - g_freed_total) : 0;
    ov_memory_stats_t stats;
    stats.allocated = g_allocated_total;
    stats.freed = g_freed_total;
    stats.current_usage = current;
    stats.peak_usage = g_peak_usage;
    stats.num_allocations = g_allocation_count;
    stats.num_frees = g_free_count;
    stats.has_leaks = (current > 0);
    return stats;
}

void ov_memory_print_stats(void) {
    ov_memory_stats_t stats = ov_memory_get_stats();
    ov_log_info("Memory Statistics:");
    ov_log_info("  Allocated: %zu bytes in %u blocks", stats.allocated, stats.num_allocations);
    ov_log_info("  Freed:     %zu bytes in %u blocks", stats.freed, stats.num_frees);
    ov_log_info("  Current:   %zu bytes active", stats.current_usage);
    ov_log_info("  Peak:      %zu bytes", stats.peak_usage);
    ov_log_info("  Leaks:     %s", stats.has_leaks ? "DETECTED" : "None (Clean)");
}

void ov_memory_cleanup(void) {
    ov_memory_stats_t stats = ov_memory_get_stats();
    if (stats.has_leaks) {
        ov_log_warn("Memory cleanup: %zu bytes still un-freed across %u remaining blocks",
                    stats.current_usage, stats.num_allocations - stats.num_frees);
    } else {
        ov_log_info("Memory cleanup: All tracked memory cleanly released (0 leaks)");
    }
}
