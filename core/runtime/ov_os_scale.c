#include "ov_os_scale.h"
#include <string.h>
#include <stdio.h>

const char *ov_privilege_ring_name(ov_privilege_ring_t ring) {
    switch (ring) {
        case OV_RING_USER: return "USER (Ring 3 / EL0)";
        case OV_RING_SUPERVISOR: return "SUPERVISOR (Ring 0 / EL1)";
        case OV_RING_HYPERVISOR: return "HYPERVISOR (Ring -1 / EL2)";
        default: return "UNKNOWN";
    }
}

bool ov_check_privilege(ov_privilege_ring_t current_ring, ov_privilege_ring_t required_ring) {
    // Higher enum numerical value = higher privilege
    return (uint32_t)current_ring >= (uint32_t)required_ring;
}

// Built-in standard syscall handlers
static uint64_t default_sys_exit(uint64_t status, uint64_t a1, uint64_t a2, uint64_t a3, void *ud) {
    (void)a1; (void)a2; (void)a3; (void)ud;
    return status;
}

static uint64_t default_sys_write(uint64_t fd, uint64_t buf, uint64_t count, uint64_t a3, void *ud) {
    (void)fd; (void)buf; (void)a3; (void)ud;
    return count; // simulated success
}

void ov_syscall_table_init(ov_syscall_table_t *table, void *user_data) {
    if (!table) return;
    memset(table, 0, sizeof(ov_syscall_table_t));
    table->user_data = user_data;

    // Register default syscalls
    ov_syscall_register(table, OV_SYS_EXIT, "sys_exit", default_sys_exit);
    ov_syscall_register(table, OV_SYS_WRITE, "sys_write", default_sys_write);
}

ov_status_t ov_syscall_register(
    ov_syscall_table_t *table,
    uint32_t syscall_num,
    const char *name,
    ov_syscall_handler_t handler
) {
    if (!table || syscall_num >= OV_MAX_SYSCALLS || !handler) {
        return OV_ERR_INVALID_PARAM;
    }
    table->handlers[syscall_num] = handler;
    table->names[syscall_num] = name;
    return OV_SUCCESS;
}

int64_t ov_syscall_dispatch(
    ov_syscall_table_t *table,
    uint32_t syscall_num,
    uint64_t arg0,
    uint64_t arg1,
    uint64_t arg2,
    uint64_t arg3
) {
    if (!table || syscall_num >= OV_MAX_SYSCALLS || !table->handlers[syscall_num]) {
        return -38; // -ENOSYS
    }
    table->call_counts[syscall_num]++;
    return (int64_t)table->handlers[syscall_num](arg0, arg1, arg2, arg3, table->user_data);
}

void ov_vec128_add_u32(ovir_vec128_t *dst, const ovir_vec128_t *a, const ovir_vec128_t *b) {
    if (!dst || !a || !b) return;
    for (int i = 0; i < 4; i++) {
        dst->u32[i] = a->u32[i] + b->u32[i];
    }
}

void ov_vec128_sub_u32(ovir_vec128_t *dst, const ovir_vec128_t *a, const ovir_vec128_t *b) {
    if (!dst || !a || !b) return;
    for (int i = 0; i < 4; i++) {
        dst->u32[i] = a->u32[i] - b->u32[i];
    }
}

void ov_vec128_mul_u32(ovir_vec128_t *dst, const ovir_vec128_t *a, const ovir_vec128_t *b) {
    if (!dst || !a || !b) return;
    for (int i = 0; i < 4; i++) {
        dst->u32[i] = a->u32[i] * b->u32[i];
    }
}

void ov_vec128_and(ovir_vec128_t *dst, const ovir_vec128_t *a, const ovir_vec128_t *b) {
    if (!dst || !a || !b) return;
    dst->u64[0] = a->u64[0] & b->u64[0];
    dst->u64[1] = a->u64[1] & b->u64[1];
}

void ov_vec128_or(ovir_vec128_t *dst, const ovir_vec128_t *a, const ovir_vec128_t *b) {
    if (!dst || !a || !b) return;
    dst->u64[0] = a->u64[0] | b->u64[0];
    dst->u64[1] = a->u64[1] | b->u64[1];
}
