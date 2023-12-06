#ifndef APP_UI_UTILS_H
#define APP_UI_UTILS_H

#include <libintl.h>
#include "adwaita.h"
#include "../fractal/fractal.h"
#include "project_variables.h"

#ifdef DEBUG
	#define debug_printerr(x, ...) g_printerr(x, ##__VA_ARGS__)
#else
	#define debug_printerr(x, ...) {}
#endif

#define DATA_PATH "../"

#define _(String) gettext (String)

#define GIO_SETTINGS_SCHEMA "com.nicolarevelant.fractal-generator"

// max iterations for preview
#define PV_MAX_ITERATION_LIMIT 500
#define VIDEO_FRAMERATE 60

extern fractal_config_t mb_tmp_config;
extern int ui_blocked;

typedef enum {
	IDLE, PHOTO_PROGRESS, VIDEO_PROGRESS, ERR_SAVE
} update_state_t;

/**
 * Data to be passed from the work thread to the main thread to update UI
*/
typedef struct {
	update_state_t state;
	float progress;
} UpdateData;

typedef struct {
	int width;
	int height;
} Size;

extern const Size frameResolutions[];

// settings UI Widgets
extern GtkSpinButton *translateStep_spinBtn;
extern GtkSpinButton *zoomStep_spinBtn;
extern GtkSpinButton *video_init_zoom;
extern GtkSpinButton *video_zoom_speed;
extern GtkDropDown *frameSizeDropDown;

// home UI
extern GtkWidget *create_home_layout();

// settings UI
extern GtkWidget *create_settings_layout();

/**
 * Create GL area
 * @return GL area
 */
extern GtkWidget *create_gl_area();

/**
 * Update GL area
 */
extern void glAreaUpdate();

// Main UI
extern void mb_on_photo_progress(float p);
extern void mb_on_video_progress(float p);
extern void image_on_save(int is_success);
extern void video_on_save(int is_success);

extern void switch_fractal();

#endif /* APP_UI_UTILS_H */
