#include "ov_core.h"
#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>

int main(int argc, char *argv[]) {
    const char *log_file = "openvintage_preboot.log";
    const char *report_file = "openvintage_report.txt";
    
    /* Parse command line arguments */
    for (int i = 1; i < argc; i++) {
        if (!strcmp(argv[i], "-l") && i + 1 < argc) {
            log_file = argv[++i];
        } else if (!strcmp(argv[i], "-r") && i + 1 < argc) {
            report_file = argv[++i];
        } else if (!strcmp(argv[i], "-h") || !strcmp(argv[i], "--help")) {
            printf("Usage: %s [options]\n", argv[0]);
            printf("Options:\n");
            printf("  -l FILE   Log file (default: openvintage_preboot.log)\n");
            printf("  -r FILE   Report file (default: openvintage_report.txt)\n");
            printf("  -h        Show this help\n");
            return 0;
        }
    }
    
    /* Initialize logger */
    if (ov_logger_init(log_file) != OV_SUCCESS) {
        fprintf(stderr, "Failed to initialize logger\n");
        return 1;
    }
    
    /* Initialize core */
    if (ov_core_init() != OV_SUCCESS) {
        ov_log_error("Failed to initialize core");
        ov_logger_cleanup();
        return 1;
    }
    
    /* Execute pre-boot simulation */
    ov_status_t status = ov_core_execute_preboot();
    
    /* Generate report */
    if (status == OV_SUCCESS) {
        ov_core_generate_report(report_file);
    }
    
    /* Cleanup */
    ov_core_cleanup();
    
    printf("\nLog file: %s\n", log_file);
    printf("Report file: %s\n", report_file);
    
    return status == OV_SUCCESS ? 0 : 1;
}
