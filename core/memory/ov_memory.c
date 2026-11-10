#include "ov_memory.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

void ov_mem_init(ov_guest_memory_t *mem) {
    if (!mem) return;
    memset(mem, 0, sizeof(ov_guest_memory_t));
}

ov_status_t ov_mem_map(
    ov_guest_memory_t *mem,
    uint64_t guest_addr,
    size_t size,
    uint32_t perms,
    const char *name
) {
    if (!mem || size == 0) return OV_ERR_INVALID_PARAM;

    // Enforce 4KB page alignment
    if ((guest_addr % OV_PAGE_SIZE) != 0 || (size % OV_PAGE_SIZE) != 0) {
        return OV_ERR_INVALID_PARAM;
    }

    // Security check: Prevent uncontrolled simultaneous RWX
    if ((perms & (OV_MEM_WRITE | OV_MEM_EXECUTE)) == (OV_MEM_WRITE | OV_MEM_EXECUTE)) {
        mem->execution_violations++;
        return OV_ERR_SECURITY_VIOLATION;
    }

    // Check for overlap with existing mapped regions
    uint64_t req_end = guest_addr + size;
    for (size_t i = 0; i < mem->region_count; i++) {
        if (!mem->regions[i].is_allocated) continue;
        uint64_t reg_start = mem->regions[i].guest_base_addr;
        uint64_t reg_end = reg_start + mem->regions[i].size;
        if (guest_addr < reg_end && req_end > reg_start) {
            return OV_ERR_INVALID_PARAM; // Overlapping region
        }
    }

    // Find free slot
    int slot = -1;
    for (size_t i = 0; i < mem->region_count; i++) {
        if (!mem->regions[i].is_allocated) {
            slot = (int)i;
            break;
        }
    }
    if (slot < 0) {
        if (mem->region_count >= OV_MAX_MEM_REGIONS) {
            return OV_ERR_OUT_OF_MEMORY;
        }
        slot = (int)mem->region_count++;
    }

    uint8_t *host_buf = (uint8_t *)calloc(1, size);
    if (!host_buf) return OV_ERR_OUT_OF_MEMORY;

    ov_guest_mem_region_t *reg = &mem->regions[slot];
    reg->guest_base_addr = guest_addr;
    reg->size = size;
    reg->host_ptr = host_buf;
    reg->permissions = perms;
    reg->is_allocated = true;
    reg->allow_rwx_override = false;
    if (name) {
        strncpy(reg->name, name, sizeof(reg->name) - 1);
        reg->name[sizeof(reg->name) - 1] = '\0';
    } else {
        snprintf(reg->name, sizeof(reg->name), "region_%d", slot);
    }

    mem->total_mapped_bytes += size;
    return OV_SUCCESS;
}

ov_status_t ov_mem_unmap(ov_guest_memory_t *mem, uint64_t guest_addr, size_t size) {
    if (!mem) return OV_ERR_INVALID_PARAM;

    for (size_t i = 0; i < mem->region_count; i++) {
        ov_guest_mem_region_t *reg = &mem->regions[i];
        if (reg->is_allocated && reg->guest_base_addr == guest_addr && reg->size == size) {
            free(reg->host_ptr);
            reg->host_ptr = NULL;
            reg->is_allocated = false;
            mem->total_mapped_bytes -= size;
            return OV_SUCCESS;
        }
    }
    return OV_ERR_INVALID_PARAM;
}

ov_status_t ov_mem_protect(
    ov_guest_memory_t *mem,
    uint64_t guest_addr,
    size_t size,
    uint32_t new_perms
) {
    if (!mem) return OV_ERR_INVALID_PARAM;

    // Security check: Uncontrolled RWX prevention
    if ((new_perms & (OV_MEM_WRITE | OV_MEM_EXECUTE)) == (OV_MEM_WRITE | OV_MEM_EXECUTE)) {
        mem->execution_violations++;
        return OV_ERR_SECURITY_VIOLATION;
    }

    for (size_t i = 0; i < mem->region_count; i++) {
        ov_guest_mem_region_t *reg = &mem->regions[i];
        if (reg->is_allocated && reg->guest_base_addr == guest_addr && reg->size == size) {
            reg->permissions = new_perms;
            return OV_SUCCESS;
        }
    }
    return OV_ERR_INVALID_PARAM;
}

ov_status_t ov_mem_translate(
    const ov_guest_memory_t *mem,
    uint64_t guest_addr,
    size_t access_size,
    uint32_t required_perm,
    void **out_host_ptr
) {
    if (!mem || !out_host_ptr || access_size == 0) return OV_ERR_INVALID_PARAM;
    *out_host_ptr = NULL;

    uint64_t access_end = guest_addr + access_size;

    for (size_t i = 0; i < mem->region_count; i++) {
        const ov_guest_mem_region_t *reg = &mem->regions[i];
        if (!reg->is_allocated) continue;

        uint64_t reg_start = reg->guest_base_addr;
        uint64_t reg_end = reg_start + reg->size;

        if (guest_addr >= reg_start && access_end <= reg_end) {
            // Check permission
            if ((reg->permissions & required_perm) != required_perm) {
                ((ov_guest_memory_t *)mem)->memory_faults++;
                if (required_perm & OV_MEM_EXECUTE) {
                    ((ov_guest_memory_t *)mem)->execution_violations++;
                }
                return OV_ERR_SECURITY_VIOLATION;
            }

            uint64_t offset = guest_addr - reg_start;
            *out_host_ptr = (void *)(reg->host_ptr + offset);
            return OV_SUCCESS;
        }
    }

    ((ov_guest_memory_t *)mem)->memory_faults++;
    return OV_ERR_FAULT;
}

ov_status_t ov_mem_read(
    const ov_guest_memory_t *mem,
    uint64_t guest_addr,
    void *dst,
    size_t size
) {
    if (!dst || size == 0) return OV_ERR_INVALID_PARAM;
    void *host_src = NULL;
    ov_status_t st = ov_mem_translate(mem, guest_addr, size, OV_MEM_READ, &host_src);
    if (st != OV_SUCCESS) return st;

    memcpy(dst, host_src, size);
    return OV_SUCCESS;
}

ov_status_t ov_mem_write(
    ov_guest_memory_t *mem,
    uint64_t guest_addr,
    const void *src,
    size_t size
) {
    if (!src || size == 0) return OV_ERR_INVALID_PARAM;
    void *host_dst = NULL;
    ov_status_t st = ov_mem_translate(mem, guest_addr, size, OV_MEM_WRITE, &host_dst);
    if (st != OV_SUCCESS) return st;

    memcpy(host_dst, src, size);
    return OV_SUCCESS;
}

void ov_mem_destroy(ov_guest_memory_t *mem) {
    if (!mem) return;
    for (size_t i = 0; i < mem->region_count; i++) {
        if (mem->regions[i].is_allocated && mem->regions[i].host_ptr) {
            free(mem->regions[i].host_ptr);
            mem->regions[i].host_ptr = NULL;
            mem->regions[i].is_allocated = false;
        }
    }
    mem->region_count = 0;
    mem->total_mapped_bytes = 0;
}
