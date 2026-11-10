#ifndef OV_OS_SCALE_H
#define OV_OS_SCALE_H

#include "../types/ov_types.h"
#include <stdint.h>
#include <stdbool.h>
#include <stddef.h>

// Privileged Execution Rings
typedef enum {
    OV_RING_USER = 0,        // Ring 3 / EL0: User application code
    OV_RING_SUPERVISOR = 1,  // Ring 0 / EL1: Kernel / Supervisor mode
    OV_RING_HYPERVISOR = 2   // Ring -1 / EL2: Hypervisor / Virtualization
} ov_privilege_ring_t;

#ifndef OV_VEC128_DEFINED
#define OV_VEC128_DEFINED
// 128-Bit SIMD / Vector Register Structure (ARM NEON / x86 SSE)
typedef union {
    uint8_t  u8[16];
    uint16_t u16[8];
    uint32_t u32[4];
    uint64_t u64[2];
    int8_t   s8[16];
    int16_t  s16[8];
    int32_t  s32[4];
    int64_t  s64[2];
    float    f32[4];
    double   f64[2];
} ovir_vec128_t;
#endif

// Memory Barrier Types & Scopes
typedef enum {
    OV_BARRIER_ISB = 0, // Instruction Synchronization Barrier
    OV_BARRIER_DSB = 1, // Data Synchronization Barrier
    OV_BARRIER_DMB = 2  // Data Memory Barrier
} ov_barrier_type_t;

typedef enum {
    OV_BARRIER_SCOPE_OSH = 0, // Outer Shareable
    OV_BARRIER_SCOPE_NSH = 1, // Non-shareable
    OV_BARRIER_SCOPE_ISH = 2, // Inner Shareable
    OV_BARRIER_SCOPE_SY  = 3  // Full System
} ov_barrier_scope_t;

typedef struct {
    ov_barrier_type_t type;
    ov_barrier_scope_t scope;
    bool order_reads;
    bool order_writes;
} ov_barrier_desc_t;

// System Call Architecture
#define OV_MAX_SYSCALLS 256

#define OV_SYS_EXIT  0
#define OV_SYS_WRITE 1
#define OV_SYS_READ  2
#define OV_SYS_MMAP  3

typedef uint64_t (*ov_syscall_handler_t)(
    uint64_t arg0,
    uint64_t arg1,
    uint64_t arg2,
    uint64_t arg3,
    void *user_data
);

typedef struct {
    ov_syscall_handler_t handlers[OV_MAX_SYSCALLS];
    const char *names[OV_MAX_SYSCALLS];
    uint64_t call_counts[OV_MAX_SYSCALLS];
    void *user_data;
} ov_syscall_table_t;

// API Functions
void ov_syscall_table_init(ov_syscall_table_t *table, void *user_data);
ov_status_t ov_syscall_register(
    ov_syscall_table_t *table,
    uint32_t syscall_num,
    const char *name,
    ov_syscall_handler_t handler
);
int64_t ov_syscall_dispatch(
    ov_syscall_table_t *table,
    uint32_t syscall_num,
    uint64_t arg0,
    uint64_t arg1,
    uint64_t arg2,
    uint64_t arg3
);

// SIMD Vector Operations
void ov_vec128_add_u32(ovir_vec128_t *dst, const ovir_vec128_t *a, const ovir_vec128_t *b);
void ov_vec128_sub_u32(ovir_vec128_t *dst, const ovir_vec128_t *a, const ovir_vec128_t *b);
void ov_vec128_mul_u32(ovir_vec128_t *dst, const ovir_vec128_t *a, const ovir_vec128_t *b);
void ov_vec128_and(ovir_vec128_t *dst, const ovir_vec128_t *a, const ovir_vec128_t *b);
void ov_vec128_or(ovir_vec128_t *dst, const ovir_vec128_t *a, const ovir_vec128_t *b);

#define ov_vec128_add_4s ov_vec128_add_u32
#define ov_vec128_sub_4s ov_vec128_sub_u32
#define ov_vec128_mul_4s ov_vec128_mul_u32

// Privilege Verification
bool ov_check_privilege(ov_privilege_ring_t current_ring, ov_privilege_ring_t required_ring);
const char *ov_privilege_ring_name(ov_privilege_ring_t ring);

#endif /* OV_OS_SCALE_H */
