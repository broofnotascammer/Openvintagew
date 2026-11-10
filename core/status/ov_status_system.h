#ifndef OV_STATUS_SYSTEM_H
#define OV_STATUS_SYSTEM_H

#include "../types/ov_types.h"

typedef enum {
    OV_STATUS_SUPPORTED = 0,
    OV_STATUS_PARTIAL,
    OV_STATUS_EXPERIMENTAL,
    OV_STATUS_TRANSLATED,
    OV_STATUS_CPU_FALLBACK,
    OV_STATUS_UNSUPPORTED,
    OV_STATUS_NOT_TESTED,
    OV_STATUS_PLATFORM_LIMITED,
    OV_STATUS_RESOURCE_LIMITED
} ov_compat_status_t;

typedef enum {
    OV_CATEGORY_CPU_ARCH = 0,
    OV_CATEGORY_GPU_ARCH,
    OV_CATEGORY_GRAPHICS_API,
    OV_CATEGORY_OS_VERSION,
    OV_CATEGORY_MAC_MODEL,
    OV_CATEGORY_APPLICATION_WORKLOAD
} ov_status_category_t;

typedef struct {
    char feature_name[64];
    ov_status_category_t category;
    char platform[32];
    char architecture[16];
    bool native_support;
    bool translation_support;
    bool cpu_fallback_support;
    bool gpu_reconstruction_support;
    ov_compat_status_t status_level;
    char known_limitations[128];
    char test_status[16]; // "PASS", "FAIL", "UNVERIFIED"
    char last_verified_date[16];
    char compatibility_notes[128];
} ov_status_item_t;

#define OV_MAX_STATUS_ITEMS 32

typedef struct {
    ov_status_item_t items[OV_MAX_STATUS_ITEMS];
    size_t item_count;
} ov_status_registry_t;

void ov_status_system_init(ov_status_registry_t *reg);
const ov_status_item_t *ov_status_find(const ov_status_registry_t *reg, const char *feature_name);
const char *ov_compat_status_name(ov_compat_status_t status);
const char *ov_status_category_name(ov_status_category_t cat);

#endif /* OV_STATUS_SYSTEM_H */
