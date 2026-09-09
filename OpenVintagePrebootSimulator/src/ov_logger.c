/**
 * OpenVintage Pre-Boot Simulator - Logging System Implementation
 */

#include "ov_logger.h"
#include <stdarg.h>
#include <time.h>
#include <string.h>

#define LOG_RING_BUFFER_SIZE 32768

const char* ov_status_to_string(ov_status_t status) {
    switch (status) {
        case OV_SUCCESS: return "Success";
        case OV_ERROR_INIT: return "Initialization Error";
        case OV_ERROR_HARDWARE: return "Hardware Error";
        case OV_ERROR_MEMORY: return "Memory Error";
        case OV_ERROR_CONFIG: return "Configuration Error";
        case OV_ERROR_INVALID_PARAM: return "Invalid Parameter";
        case OV_ERROR_NOT_FOUND: return "Not Found";
        case OV_ERROR_OUT_OF_RESOURCES: return "Out of Resources";
        case OV_ERROR_UNSUPPORTED: return "Unsupported";
        case OV_ERROR_INTEGRITY: return "Integrity Failure";
        default: return "Unknown Error";
    }
}

static FILE *log_file = NULL;
static char log_filename[512] = {0};
static char recent_logs_buffer[LOG_RING_BUFFER_SIZE] = {0};
static size_t recent_logs_pos = 0;

static void append_to_recent(const char *prefix, const char *msg) {
    char entry[1024];
    int len = snprintf(entry, sizeof(entry), "%s%s\n", prefix, msg);
    if (len <= 0) return;

    if (recent_logs_pos + len >= sizeof(recent_logs_buffer) - 1) {
        /* Shift buffer down to keep most recent */
        size_t half = sizeof(recent_logs_buffer) / 2;
        memmove(recent_logs_buffer, recent_logs_buffer + half, sizeof(recent_logs_buffer) - half);
        recent_logs_pos = strlen(recent_logs_buffer);
    }

    strcat(recent_logs_buffer + recent_logs_pos, entry);
    recent_logs_pos += len;
}

ov_status_t ov_logger_init(const char *filename) {
    if (!filename) return OV_ERROR_INVALID_PARAM;
    
    strncpy(log_filename, filename, sizeof(log_filename) - 1);
    log_file = fopen(filename, "w");
    
    recent_logs_buffer[0] = '\0';
    recent_logs_pos = 0;

    if (!log_file) {
        fprintf(stderr, "Failed to open log file: %s\n", filename);
        return OV_ERROR_INIT;
    }
    
    fprintf(log_file, "=== OpenVintage PreBoot Simulator Log ===\n");
    fprintf(log_file, "Started at: ");
    
    time_t now = time(NULL);
    fprintf(log_file, "%s\n", ctime(&now));
    fflush(log_file);

    append_to_recent("[INIT] ", "OpenVintage PreBoot Simulator Logger Initialized");
    
    return OV_SUCCESS;
}

void ov_log(ov_log_level_t level, const char *format, ...) {
    va_list args;
    const char *level_str[] = {"DEBUG", "INFO", "WARN", "ERROR"};
    char formatted[1024];
    
    if (level < 0 || level > 3) level = OV_LOG_INFO;
    
    va_start(args, format);
    vsnprintf(formatted, sizeof(formatted), format, args);
    va_end(args);

    char prefix[32];
    snprintf(prefix, sizeof(prefix), "[%s] ", level_str[level]);

    if (log_file) {
        fprintf(log_file, "%s%s\n", prefix, formatted);
        fflush(log_file);
    }
    
    fprintf(stdout, "%s%s\n", prefix, formatted);
    append_to_recent(prefix, formatted);
}

void ov_log_debug(const char *format, ...) {
    va_list args;
    char formatted[1024];
    va_start(args, format);
    vsnprintf(formatted, sizeof(formatted), format, args);
    va_end(args);

    if (log_file) {
        fprintf(log_file, "[DEBUG] %s\n", formatted);
        fflush(log_file);
    }
    append_to_recent("[DEBUG] ", formatted);
}

void ov_log_info(const char *format, ...) {
    va_list args;
    char formatted[1024];
    va_start(args, format);
    vsnprintf(formatted, sizeof(formatted), format, args);
    va_end(args);

    if (log_file) {
        fprintf(log_file, "[INFO] %s\n", formatted);
        fflush(log_file);
    }
    fprintf(stdout, "[INFO] %s\n", formatted);
    append_to_recent("[INFO] ", formatted);
}

void ov_log_warn(const char *format, ...) {
    va_list args;
    char formatted[1024];
    va_start(args, format);
    vsnprintf(formatted, sizeof(formatted), format, args);
    va_end(args);

    if (log_file) {
        fprintf(log_file, "[WARN] %s\n", formatted);
        fflush(log_file);
    }
    fprintf(stdout, "[WARN] %s\n", formatted);
    append_to_recent("[WARN] ", formatted);
}

void ov_log_error(const char *format, ...) {
    va_list args;
    char formatted[1024];
    va_start(args, format);
    vsnprintf(formatted, sizeof(formatted), format, args);
    va_end(args);

    if (log_file) {
        fprintf(log_file, "[ERROR] %s\n", formatted);
        fflush(log_file);
    }
    fprintf(stderr, "[ERROR] %s\n", formatted);
    append_to_recent("[ERROR] ", formatted);
}

const char* ov_logger_get_filename(void) {
    return log_filename;
}

void ov_logger_get_recent_logs(char *out_buffer, size_t max_bytes) {
    if (!out_buffer || max_bytes == 0) return;
    strncpy(out_buffer, recent_logs_buffer, max_bytes - 1);
    out_buffer[max_bytes - 1] = '\0';
}

void ov_logger_cleanup(void) {
    if (log_file) {
        fprintf(log_file, "\n=== Log Ended ===\n");
        fclose(log_file);
        log_file = NULL;
    }
}
