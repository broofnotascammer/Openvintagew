#include "ov_scheduler.h"
#include <string.h>

void ov_scheduler_init(ov_scheduler_t *sched, uint32_t total_cores, uint64_t total_ram_mb, uint64_t total_vram_mb) {
    if (!sched) return;
    memset(sched, 0, sizeof(ov_scheduler_t));
    sched->pool.total_cores = total_cores;
    sched->pool.total_ram_mb = total_ram_mb;
    sched->pool.total_vram_mb = total_vram_mb;
    sched->next_ticket_id = 1000;
}

ov_status_t ov_scheduler_evaluate(
    ov_scheduler_t *sched,
    const ov_workload_desc_t *workload,
    const ov_execution_plan_t *plan,
    ov_schedule_ticket_t *out_ticket
) {
    if (!sched || !workload || !plan || !out_ticket) return OV_ERR_INVALID_PARAM;
    memset(out_ticket, 0, sizeof(ov_schedule_ticket_t));
    sched->total_schedules++;

    if (!plan->is_supported || plan->decision == OV_DECISION_UNSUPPORTED) {
        out_ticket->status = OV_SCHED_REJECTED_UNSUPPORTED_PLAN;
        out_ticket->reason_description = "Workload rejected due to unsupported execution plan";
        sched->total_rejections++;
        return OV_ERR_UNSUPPORTED;
    }

    // Check available core pool
    uint32_t needed_cores = (workload->required_cpu_cores > 0) ? workload->required_cpu_cores : 1;
    if (sched->pool.allocated_cores + needed_cores > sched->pool.total_cores) {
        out_ticket->status = OV_SCHED_REJECTED_CORES;
        out_ticket->reason_description = "Host CPU core pool exhausted";
        sched->total_rejections++;
        return OV_ERR_RESOURCE_EXHAUSTED;
    }

    // Check available RAM pool
    if (sched->pool.allocated_ram_mb + workload->ram_requirement_mb > sched->pool.total_ram_mb) {
        out_ticket->status = OV_SCHED_REJECTED_MEMORY;
        out_ticket->reason_description = "Host RAM pool exhausted";
        sched->total_rejections++;
        return OV_ERR_RESOURCE_EXHAUSTED;
    }

    // Check available VRAM pool
    if (sched->pool.allocated_vram_mb + workload->vram_requirement_mb > sched->pool.total_vram_mb) {
        out_ticket->status = OV_SCHED_REJECTED_VRAM;
        out_ticket->reason_description = "GPU VRAM pool exhausted";
        sched->total_rejections++;
        return OV_ERR_RESOURCE_EXHAUSTED;
    }

    // Allocate resources to ticket
    sched->pool.allocated_cores += needed_cores;
    sched->pool.allocated_ram_mb += workload->ram_requirement_mb;
    sched->pool.allocated_vram_mb += workload->vram_requirement_mb;

    out_ticket->ticket_id = ++sched->next_ticket_id;
    out_ticket->status = OV_SCHED_ACCEPTED;
    out_ticket->assigned_cores = needed_cores;
    out_ticket->assigned_ram_mb = workload->ram_requirement_mb;
    out_ticket->assigned_vram_mb = workload->vram_requirement_mb;
    out_ticket->profile = plan->selected_profile;
    out_ticket->reason_description = "Workload admitted and scheduled successfully";

    return OV_SUCCESS;
}

void ov_scheduler_release(ov_scheduler_t *sched, const ov_schedule_ticket_t *ticket) {
    if (!sched || !ticket || ticket->status != OV_SCHED_ACCEPTED) return;

    if (sched->pool.allocated_cores >= ticket->assigned_cores) {
        sched->pool.allocated_cores -= ticket->assigned_cores;
    } else {
        sched->pool.allocated_cores = 0;
    }

    if (sched->pool.allocated_ram_mb >= ticket->assigned_ram_mb) {
        sched->pool.allocated_ram_mb -= ticket->assigned_ram_mb;
    } else {
        sched->pool.allocated_ram_mb = 0;
    }

    if (sched->pool.allocated_vram_mb >= ticket->assigned_vram_mb) {
        sched->pool.allocated_vram_mb -= ticket->assigned_vram_mb;
    } else {
        sched->pool.allocated_vram_mb = 0;
    }
}
