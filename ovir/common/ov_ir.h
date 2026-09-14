/**
 * OpenVintage - Common Intermediate Representation Header (Phase 6)
 */

#ifndef OV_IR_H
#define OV_IR_H

#include "ov_types.h"
#include <stdint.h>
#include <stdbool.h>
#include <stddef.h>

typedef enum {
    OV_IR_TYPE_CPU = 1,
    OV_IR_TYPE_GPU = 2
} ov_ir_type_t;

typedef struct {
    uint32_t     ir_id;
    ov_ir_type_t type;
    char         source_name[64];
    uint32_t     instruction_count;
    bool         optimized;
    uint32_t     memory_footprint;
} ov_ir_metadata_t;

ov_status_t ov_ir_init_metadata(ov_ir_metadata_t *meta, ov_ir_type_t type, const char *source_name);
const char* ov_ir_type_to_string(ov_ir_type_t type);

#endif /* OV_IR_H */
