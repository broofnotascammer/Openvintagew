/**
 * OpenVintage Pre-Boot Simulator - Safe Memory Subsystem
 * Tracks allocations, peaks, prevents and detects leaks, portable across Linux and macOS.
 */

#ifndef OV_MEMORY_H
#define OV_MEMORY_H

#include "ov_types.h"
#include <stdlib.h>

ov_status_t ov_memory_init(size_t pool_size);
void*       ov_malloc(size_t size);
void*       ov_calloc(size_t num, size_t size);
void*       ov_realloc(void *ptr, size_t size);
void        ov_free(void *ptr);
void        ov_memory_cleanup(void);

/* Memory statistics */
typedef struct {
    size_t   allocated;
    size_t   freed;
    size_t   current_usage;
    size_t   peak_usage;
    uint32_t num_allocations;
    uint32_t num_frees;
    bool     has_leaks;
} ov_memory_stats_t;

ov_memory_stats_t ov_memory_get_stats(void);
void              ov_memory_print_stats(void);
bool              ov_memory_is_clean(void);
size_t            ov_memory_get_active_bytes(void);

#endif /* OV_MEMORY_H */
