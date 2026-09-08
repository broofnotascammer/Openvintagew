/**
 * OpenVintage Pre-Boot Simulator - GTK4 Native User Interface
 * Multi-tab graphical interface for Zorin OS / Linux desktops.
 */

#ifndef OV_UI_H
#define OV_UI_H

#include "ov_types.h"

#ifdef OV_ENABLE_GTK
#include <gtk/gtk.h>
#endif

/* Run GTK4 Application */
int ov_ui_run(int argc, char *argv[]);

/* Helper to check if GUI display is available */
bool ov_ui_is_display_available(void);

#endif /* OV_UI_H */
