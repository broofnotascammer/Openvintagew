#ifndef OV_TYPES_H
#define OV_TYPES_H

#include <stdint.h>
#include <stdbool.h>
#include <string.h>

/* Status codes */
typedef enum {
    OV_SUCCESS = 0,
    OV_ERROR_INIT = 1,
    OV_ERROR_HARDWARE = 2,
    OV_ERROR_MEMORY = 3,
    OV_ERROR_CONFIG = 4,
    OV_ERROR_INVALID_PARAM = 5
} ov_status_t;

/* Hardware types */
typedef enum {
    OV_CPU_INTEL_LEGACY = 0,
    OV_CPU_INTEL_MODERN = 1,
    OV_CPU_AMD = 2,
    OV_CPU_ARM = 3,
    OV_CPU_UNKNOWN = 4
} ov_cpu_type_t;

typedef enum {
    OV_GPU_INTEL_HD = 0,
    OV_GPU_INTEL_IRIS = 1,
    OV_GPU_NVIDIA = 2,
    OV_GPU_AMD = 3,
    OV_GPU_NONE = 4
} ov_gpu_type_t;

/* Execution modes */
typedef enum {
    OV_EXEC_NATIVE = 0,
    OV_EXEC_TRANSLATED = 1,
    OV_EXEC_FALLBACK = 2
} ov_exec_mode_t;

/* CPU info structure */
typedef struct {
    ov_cpu_type_t type;
    uint32_t cores;
    uint32_t threads;
    uint32_t base_freq_mhz;
    uint32_t max_freq_mhz;
    char model_name[256];
    uint64_t features_mask;
    bool has_sse42;
    bool has_avx;
    bool has_avx2;
    bool has_tsc;
} ov_cpu_info_t;

/* GPU info structure */
typedef struct {
    ov_gpu_type_t type;
    uint32_t vram_mb;
    char model_name[256];
    bool has_compute;
    bool has_graphics;
    uint32_t eu_count;
} ov_gpu_info_t;

/* Memory info structure */
typedef struct {
    uint64_t total_bytes;
    uint64_t available_bytes;
    uint32_t numa_nodes;
    uint32_t channels;
} ov_memory_info_t;

/* PCI device info */
typedef struct {
    uint16_t vendor_id;
    uint16_t device_id;
    uint8_t bus;
    uint8_t slot;
    uint8_t func;
    char class_name[128];
    char vendor_name[128];
    char device_name[256];
} ov_pci_device_t;

/* Workload descriptor */
typedef enum {
    OV_WORKLOAD_COMPUTE = 0,
    OV_WORKLOAD_GRAPHICS = 1,
    OV_WORKLOAD_MEMORY = 2,
    OV_WORKLOAD_IO = 3,
    OV_WORKLOAD_MIXED = 4
} ov_workload_type_t;

typedef struct {
    ov_workload_type_t type;
    uint32_t priority;
    uint32_t timeout_ms;
    bool requires_gpu;
    bool requires_sync;
} ov_workload_t;

/* Resolution decision */
typedef struct {
    ov_exec_mode_t mode;
    uint32_t estimated_cost;
    char reason[256];
} ov_resolution_t;

#endif /* OV_TYPES_H */
