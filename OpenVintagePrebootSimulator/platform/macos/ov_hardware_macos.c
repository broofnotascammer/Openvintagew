#include "ov_macos_native.h"

#include <CoreFoundation/CoreFoundation.h>
#include <IOKit/IOKitLib.h>
#include <IOKit/graphics/IOGraphicsLib.h>
#include <sys/sysctl.h>
#include <mach/mach.h>
#include <mach/machine.h>
#include <cpuid.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>

/* Native macOS hardware detection backend. */

/*
 * ...
 *
 * NOTE: This file is intentionally updated only at the affected sites below:
 * Penryn is already represented by OV_CPU_INTEL_CORE2_DUO in ov_types.h, so
 * native detection must use that existing enum rather than inventing a second
 * CPU-family identifier.
 */

