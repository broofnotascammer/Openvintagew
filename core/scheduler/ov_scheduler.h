#ifndef OV_SCHEDULER_H
#define OV_SCHEDULER_H

#include "../types/ov_types.h"
#include "../resolver/ov_resolver.h"

typedef enum {
    OV_SCHED_ACCEPTED = 0,
    OV_SCHED_REJECTED_CORES,
    OV_SCHED_REJECTED_MEMORY,
    OV_SCHED_REJECTED_VRAM,
    OV_SCHED_REJECTED_UNSUPPORTED_PLAN
} ov_schedule_status_t;

typedef struct {
    uint32_t total_cores;
    uint32_t allocated_cores;
    uint64_t total_ram_mb;
    uint64_t allocated_ram_mb;
    uint64_t total_vram_mb;
    uint64_t allocated_vram_mb;
} ov_resource_pool_t;

typedef struct {
    uint32_t ticket_id;
    ov_schedule_status_t status;
    uint32_t assigned_cores;
    uint64_t assigned_ram_mb;
    uint64_t assigned_vram_mb;
    ov_scheduler_profile_t profile;
    const char *reason_description;
} ov_schedule_ticket_t;

typedef struct {
    ov_resource_pool_t pool;
    uint32_t next_ticket_id;
    uint32_t total_schedules;
    uint32_t total_rejections;
} ov_scheduler_t;

void ov_scheduler_init(ov_scheduler_t *sched, uint32_t total_cores, uint64_t total_ram_mb, uint64_t total_vram_mb);
ov_status_t ov_scheduler_evaluate(
    ov_scheduler_t *sched,
    const ov_workload_desc_t *workload,
    const ov_execution_plan_t *plan,
    ov_schedule_ticket_t *out_ticket
);
void ov_scheduler_release(ov_scheduler_t *sched, const ov_schedule_ticket_t *ticket);

#endif /* OV_SCHEDULER_H */
