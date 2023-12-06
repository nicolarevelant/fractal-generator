#ifndef APP_UI_H
#define APP_UI_H

#include <stdio.h>
#include <stdlib.h>

#include "app_ui_utils.h"

#define APPLICATION_ID "com.nicolarevelant.fractal-generator"

/**
 * Callback called when the GtkApplication activate the app
 * @param app The GtkApplication of the project
 */
extern void on_app_activate(GtkApplication *app);

#endif /* APP_UI_H */

