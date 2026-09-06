#ifndef OV_LOGGER_H
#define OV_LOGGER_H

#include "ov_types.h"
#include <stdio.h>

typedef enum {
    OV_LOG_DEBUG = 0,
    OV_LOG_INFO = 1,
    OV_LOG_WARN = 2,
    OV_LOG_ERROR = 3
} ov_log_level_t;

/* Logger initialization */
ov_status_t ov_logger_init(const char *log_file);
ovoid ov_logger_cleanup(void);

/* Logging functions */
void ov_log(ov_log_level_t level, const char *format, ...);
void ov_log_debug(const char *format, ...);
void ov_log_info(const char *format, ...);
void ov_log_warn(const char *format, ...);
void ov_log_error(const char *format, ...);

/* File output */
const char* ov_logger_get_filename(void);

#endif /* OV_LOGGER_H */
