#include "ov_status_system.h"
#include <string.h>

void ov_status_system_init(ov_status_registry_t *reg) {
    if (!reg) return;
    memset(reg, 0, sizeof(ov_status_registry_t));

    // 1. ARM64 CPU Emulation
    reg->items[reg->item_count++] = (ov_status_item_t){
        .feature_name = "ARM64 Translation (OVIR-CPU)",
        .category = OV_CATEGORY_CPU_ARCH,
        .platform = "macOS / Linux",
        .architecture = "ARM64 -> x86-64",
        .native_support = false,
        .translation_support = true,
        .cpu_fallback_support = true,
        .gpu_reconstruction_support = false,
        .status_level = OV_STATUS_TRANSLATED,
        .known_limitations = "Integer arithmetic subset (MOV, ADD, SUB, MUL, RET); vector/FP in development",
        .test_status = "PASS",
        .last_verified_date = "2026-10-03",
        .compatibility_notes = "Deterministic JIT verified with translation cache"
    };

    // 2. x86-64 Native Execution
    reg->items[reg->item_count++] = (ov_status_item_t){
        .feature_name = "x86-64 Native Execution",
        .category = OV_CATEGORY_CPU_ARCH,
        .platform = "macOS / Linux",
        .architecture = "x86-64",
        .native_support = true,
        .translation_support = false,
        .cpu_fallback_support = false,
        .gpu_reconstruction_support = false,
        .status_level = OV_STATUS_SUPPORTED,
        .known_limitations = "Requires host CPU feature parity (AVX, FMA)",
        .test_status = "PASS",
        .last_verified_date = "2026-10-03",
        .compatibility_notes = "Direct native dispatch without translation overhead"
    };

    // 3. Vulkan via MoltenVK / Metal
    reg->items[reg->item_count++] = (ov_status_item_t){
        .feature_name = "Vulkan Graphics Translation",
        .category = OV_CATEGORY_GRAPHICS_API,
        .platform = "macOS",
        .architecture = "Any",
        .native_support = false,
        .translation_support = true,
        .cpu_fallback_support = true,
        .gpu_reconstruction_support = true,
        .status_level = OV_STATUS_TRANSLATED,
        .known_limitations = "Limited to Metal 1 feature set on legacy Intel/Kepler hardware",
        .test_status = "PASS",
        .last_verified_date = "2026-10-03",
        .compatibility_notes = "Reconstructs compute & draw calls via host Metal backend"
    };

    // 4. Direct3D 11 via OVIR-GPU
    reg->items[reg->item_count++] = (ov_status_item_t){
        .feature_name = "Direct3D 11 Translation",
        .category = OV_CATEGORY_GRAPHICS_API,
        .platform = "macOS",
        .architecture = "Any",
        .native_support = false,
        .translation_support = true,
        .cpu_fallback_support = true,
        .gpu_reconstruction_support = true,
        .status_level = OV_STATUS_PARTIAL,
        .known_limitations = "Feature Level 11_0 shaders; complex geometry shaders trigger reconstruction",
        .test_status = "PASS",
        .last_verified_date = "2026-10-03",
        .compatibility_notes = "Falls back to CPU software rasterization when hardware limits exceeded"
    };

    // 5. Direct3D 12
    reg->items[reg->item_count++] = (ov_status_item_t){
        .feature_name = "Direct3D 12 Graphics",
        .category = OV_CATEGORY_GRAPHICS_API,
        .platform = "macOS",
        .architecture = "x86-64",
        .native_support = false,
        .translation_support = false,
        .cpu_fallback_support = true,
        .gpu_reconstruction_support = false,
        .status_level = OV_STATUS_EXPERIMENTAL,
        .known_limitations = "Requires Resource Binding Tier 3 and Mesh Shaders; Kepler limited",
        .test_status = "PASS",
        .last_verified_date = "2026-10-03",
        .compatibility_notes = "CPU fallback available for non-accelerated compute shaders"
    };

    // 6. MacBookPro9,1 Hardware Target
    reg->items[reg->item_count++] = (ov_status_item_t){
        .feature_name = "MacBookPro9,1 (Mid-2012 15\")",
        .category = OV_CATEGORY_MAC_MODEL,
        .platform = "macOS Catalina 10.15.8",
        .architecture = "x86-64",
        .native_support = true,
        .translation_support = true,
        .cpu_fallback_support = true,
        .gpu_reconstruction_support = true,
        .status_level = OV_STATUS_SUPPORTED,
        .known_limitations = "Mechanical HDD requires HDD-aware cache with RAM Hot tier",
        .test_status = "PASS",
        .last_verified_date = "2026-10-03",
        .compatibility_notes = "Primary Phase 10 & 11 validation hardware platform"
    };

    // 7. Apple Silicon (M-Series)
    reg->items[reg->item_count++] = (ov_status_item_t){
        .feature_name = "Apple Silicon Macs (M1/M2/M3)",
        .category = OV_CATEGORY_MAC_MODEL,
        .platform = "macOS Sonoma / Sequoia",
        .architecture = "ARM64",
        .native_support = true,
        .translation_support = true,
        .cpu_fallback_support = false,
        .gpu_reconstruction_support = true,
        .status_level = OV_STATUS_EXPERIMENTAL,
        .known_limitations = "x86-64 -> ARM64 translator currently under development",
        .test_status = "UNVERIFIED",
        .last_verified_date = "2026-10-03",
        .compatibility_notes = "Native ARM64 workloads execute natively; x86 workloads require Rosetta or OVIR"
    };

    // 8. Arch Linux
    reg->items[reg->item_count++] = (ov_status_item_t){
        .feature_name = "Arch Linux x86_64",
        .category = OV_CATEGORY_OS_VERSION,
        .platform = "Linux",
        .architecture = "x86-64",
        .native_support = true,
        .translation_support = true,
        .cpu_fallback_support = true,
        .gpu_reconstruction_support = true,
        .status_level = OV_STATUS_SUPPORTED,
        .known_limitations = "Requires Vulkan ICD or OpenGL Core Profile 4.x drivers",
        .test_status = "PASS",
        .last_verified_date = "2026-10-03",
        .compatibility_notes = "Secondary developer test platform verified in CI"
    };
}

const ov_status_item_t *ov_status_find(const ov_status_registry_t *reg, const char *feature_name) {
    if (!reg || !feature_name) return NULL;
    for (size_t i = 0; i < reg->item_count; i++) {
        if (strstr(reg->items[i].feature_name, feature_name) != NULL) {
            return &reg->items[i];
        }
    }
    return NULL;
}

const char *ov_compat_status_name(ov_compat_status_t status) {
    switch (status) {
        case OV_STATUS_SUPPORTED: return "SUPPORTED";
        case OV_STATUS_PARTIAL: return "PARTIAL";
        case OV_STATUS_EXPERIMENTAL: return "EXPERIMENTAL";
        case OV_STATUS_TRANSLATED: return "TRANSLATED";
        case OV_STATUS_CPU_FALLBACK: return "CPU FALLBACK";
        case OV_STATUS_UNSUPPORTED: return "UNSUPPORTED";
        case OV_STATUS_NOT_TESTED: return "NOT TESTED";
        case OV_STATUS_PLATFORM_LIMITED: return "PLATFORM LIMITED";
        case OV_STATUS_RESOURCE_LIMITED: return "RESOURCE LIMITED";
        default: return "UNKNOWN";
    }
}

const char *ov_status_category_name(ov_status_category_t cat) {
    switch (cat) {
        case OV_CATEGORY_CPU_ARCH: return "CPU Architecture";
        case OV_CATEGORY_GPU_ARCH: return "GPU Architecture";
        case OV_CATEGORY_GRAPHICS_API: return "Graphics API";
        case OV_CATEGORY_OS_VERSION: return "Operating System";
        case OV_CATEGORY_MAC_MODEL: return "Mac Model";
        case OV_CATEGORY_APPLICATION_WORKLOAD: return "Application / Workload";
        default: return "General";
    }
}
