#ifndef OV_MEMORY_H
#define OV_MEMORY_H

#include "ov_types.h"
#include <stdlib.h>

/* Memory pool management */
typedef struct ov_memory_pool ov_memory_pool_t;

ov_status_t ov_memory_init(size_t pool_size);
ovoid* ov_malloc(size_t size);
void* ov_realloc(void *ptr, size_t size);
void ov_free(void *ptr);
ovoid ov_memory_cleanup(void);

/* Memory statistics */
typedef struct {
    size_t allocated;
    size_t freed;
    size_t peak_usage;
    uint32_t num_allocations;
} ov_memory_stats_t;

ov_memory_stats_t ov_memory_get_stats(void);
void ov_memory_print_stats(void);

#endif /* OV_MEMORY_H */
