/**
 * OpenVintage - Common Intermediate Representation Implementation (Phase 6)
 */

#include "ov_ir.h"
#include <string.h>

ov_status_t ov_ir_init_metadata(ov_ir_metadata_t *meta, ov_ir_type_t type, const char *source_name) {
    if (!meta) return OV_ERROR_INVALID_PARAM;
    memset(meta, 0, sizeof(ov_ir_metadata_t));
    meta->type = type;
    if (source_name) {
        strncpy(meta->source_name, source_name, sizeof(meta->source_name) - 1);
    }
    return OV_SUCCESS;
}

const char* ov_ir_type_to_string(ov_ir_type_t type) {
    switch (type) {
        case OV_IR_TYPE_CPU: return "OVIR-CPU";
        case OV_IR_TYPE_GPU: return "OVIR-GPU";
        default:             return "OVIR-Unknown";
    }
}
