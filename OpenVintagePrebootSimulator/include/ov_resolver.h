#ifndef OV_RESOLVER_H
#define OV_RESOLVER_H

#include "ov_types.h"

/* Resolver decision engine */
ov_status_t ov_resolver_init(ov_cpu_info_t *cpu, ov_gpu_info_t *gpu, ov_memory_info_t *mem);
ov_resolution_t ov_resolver_route(ov_workload_t *workload);
ovoid ov_resolver_cleanup(void);

/* Decision analysis */
ovoid ov_resolver_print_decision(ov_resolution_t *decision);
ovoid ov_resolver_analyze_cpu(ov_cpu_info_t *cpu);
ovoid ov_resolver_analyze_gpu(ov_gpu_info_t *gpu);

#endif /* OV_RESOLVER_H */
