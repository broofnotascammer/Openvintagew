#ifndef OV_DIAGNOSTICS_H
#define OV_DIAGNOSTICS_H

#include "../resolver/ov_resolver.h"
#include "../scheduler/ov_scheduler.h"
#include "../telemetry/ov_telemetry.h"

int ov_diagnostics_generate_report(
    const ov_workload_desc_t *workload,
    const ov_execution_plan_t *plan,
    const ov_schedule_ticket_t *ticket,
    const ov_execution_telemetry_t *telemetry,
    char *out_buffer,
    size_t buffer_size
);

#endif /* OV_DIAGNOSTICS_H */
