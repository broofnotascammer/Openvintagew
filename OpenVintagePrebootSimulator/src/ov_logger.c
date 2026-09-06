#include "ov_logger.h"
#include <stdarg.h>
#include <time.h>

static FILE *log_file = NULL;
static char log_filename[512] = {0};

ov_status_t ov_logger_init(const char *filename) {
    if (!filename) return OV_ERROR_INVALID_PARAM;
    
    strncpy(log_filename, filename, sizeof(log_filename) - 1);
    log_file = fopen(filename, "w");
    
    if (!log_file) {
        fprintf(stderr, "Failed to open log file: %s\n", filename);
        return OV_ERROR_INIT;
    }
    
    fprintf(log_file, "=== OpenVintage PreBoot Simulator Log ===\n");
    fprintf(log_file, "Started at: ");
    
    time_t now = time(NULL);
    fprintf(log_file, "%s\n\n", ctime(&now));
    fflush(log_file);
    
    return OV_SUCCESS;
}

void ov_log(ov_log_level_t level, const char *format, ...) {
    va_list args;
    const char *level_str[] = {"DEBUG", "INFO", "WARN", "ERROR"};
    
    if (level < 0 || level > 3) level = OV_LOG_INFO;
    
    if (log_file) {
        fprintf(log_file, "[%s] ", level_str[level]);
        va_start(args, format);
        vfprintf(log_file, format, args);
        va_end(args);
        fprintf(log_file, "\n");
        fflush(log_file);
    }
    
    fprintf(stdout, "[%s] ", level_str[level]);
    va_start(args, format);
    vfprintf(stdout, format, args);
    va_end(args);
    fprintf(stdout, "\n");
}

void ov_log_debug(const char *format, ...) {
    va_list args;
    va_start(args, format);
    if (log_file) {
        fprintf(log_file, "[DEBUG] ");
        vfprintf(log_file, format, args);
        fprintf(log_file, "\n");
    }
    va_end(args);
}

void ov_log_info(const char *format, ...) {
    va_list args;
    va_start(args, format);
    if (log_file) {
        fprintf(log_file, "[INFO] ");
        vfprintf(log_file, format, args);
        fprintf(log_file, "\n");
        fflush(log_file);
    }
    fprintf(stdout, "[INFO] ");
    vfprintf(stdout, format, args);
    fprintf(stdout, "\n");
    va_end(args);
}

void ov_log_warn(const char *format, ...) {
    va_list args;
    va_start(args, format);
    if (log_file) {
        fprintf(log_file, "[WARN] ");
        vfprintf(log_file, format, args);
        fprintf(log_file, "\n");
    }
    fprintf(stdout, "[WARN] ");
    vfprintf(stdout, format, args);
    fprintf(stdout, "\n");
    va_end(args);
}

void ov_log_error(const char *format, ...) {
    va_list args;
    va_start(args, format);
    if (log_file) {
        fprintf(log_file, "[ERROR] ");
        vfprintf(log_file, format, args);
        fprintf(log_file, "\n");
        fflush(log_file);
    }
    fprintf(stderr, "[ERROR] ");
    vfprintf(stderr, format, args);
    fprintf(stderr, "\n");
    va_end(args);
}

const char* ov_logger_get_filename(void) {
    return log_filename;
}

void ov_logger_cleanup(void) {
    if (log_file) {
        fprintf(log_file, "\n=== Log Ended ===\n");
        fclose(log_file);
        log_file = NULL;
    }
}
