#ifndef OV_MEMORY_H
#define OV_MEMORY_H

#include "../types/ov_types.h"

#define OV_PAGE_SIZE 4096
#define OV_MAX_MEM_REGIONS 32

typedef struct {
    uint64_t guest_base_addr;
    size_t size;
    uint8_t *host_ptr;
    uint32_t permissions; // Bitmask of OV_MEM_READ, OV_MEM_WRITE, OV_MEM_EXECUTE
    char name[32];
    bool is_allocated;
    bool allow_rwx_override;
} ov_guest_mem_region_t;

typedef struct {
    ov_guest_mem_region_t regions[OV_MAX_MEM_REGIONS];
    size_t region_count;
    uint64_t total_mapped_bytes;
    uint64_t memory_faults;
    uint64_t execution_violations;
} ov_guest_memory_t;

void ov_mem_init(ov_guest_memory_t *mem);
ov_status_t ov_mem_map(
    ov_guest_memory_t *mem,
    uint64_t guest_addr,
    size_t size,
    uint32_t perms,
    const char *name
);
ov_status_t ov_mem_unmap(ov_guest_memory_t *mem, uint64_t guest_addr, size_t size);
ov_status_t ov_mem_protect(
    ov_guest_memory_t *mem,
    uint64_t guest_addr,
    size_t size,
    uint32_t new_perms
);

// Safe translation from guest address to host pointer with bounds and permission check
ov_status_t ov_mem_translate(
    const ov_guest_memory_t *mem,
    uint64_t guest_addr,
    size_t access_size,
    uint32_t required_perm,
    void **out_host_ptr
);

// Checked read and write helpers
ov_status_t ov_mem_read(
    const ov_guest_memory_t *mem,
    uint64_t guest_addr,
    void *dst,
    size_t size
);

ov_status_t ov_mem_write(
    ov_guest_memory_t *mem,
    uint64_t guest_addr,
    const void *src,
    size_t size
);

void ov_mem_destroy(ov_guest_memory_t *mem);

#endif /* OV_MEMORY_H */
