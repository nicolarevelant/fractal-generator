#include "app_ui.h"

static const char *authors[] = {
        "Nicola Revelant <nicolarevelant@outlook.com>", NULL
};
static const char *designers[] = {
        "Nicola Revelant <nicolarevelant@outlook.com>", NULL
};

static void show_about_dialog(GtkWindow *window);

static void on_window_destroy();
static gboolean on_window_key_pressed(GtkEventControllerKey *, guint key, guint, GdkModifierType state, gpointer);

// public functions

extern void on_app_activate(GtkApplication *app) {
	GtkBuilder *builder = gtk_builder_new();
	GError *error = NULL;
	if (!gtk_builder_add_from_file(builder, DATA_PATH"ui/app_ui.ui", &error)) {
		debug_printerr(" [EE] Cannot create UI: %s\n", error->message);
		g_printerr("Application error, closing\n");
		g_error_free(error);
		g_object_unref(builder);
		return;
	}

	GtkWindow *window = GTK_WINDOW(gtk_builder_get_object(builder, "window"));
	g_signal_connect(window, "destroy", G_CALLBACK(on_window_destroy), NULL);
	gtk_window_set_application(window, app);

	GObject *aboutBtn = gtk_builder_get_object(builder, "aboutBtn");
	g_signal_connect_swapped(aboutBtn, "clicked", G_CALLBACK(show_about_dialog), window);

	GtkBox *mainBox = GTK_BOX( gtk_builder_get_object(builder, "mainBox"));
	GtkWidget *tmpWidget = create_home_layout();
	if (!tmpWidget) {
		debug_printerr(" [EE] Cannot create main layout: Home layout is NULL\n");
		g_object_unref(builder);
		gtk_window_destroy(window);
		return;
	}
	gtk_box_append(mainBox, tmpWidget);
	tmpWidget = create_settings_layout();
	if (!tmpWidget) {
		debug_printerr(" [EE] Cannot create main layout: Settings layout is NULL\n");
		g_object_unref(builder);
		gtk_window_destroy(window);
		return;
	}
	gtk_box_append(mainBox, tmpWidget);

	tmpWidget = create_gl_area();
	if (!tmpWidget) {
		debug_printerr(" [EE] Cannot create main layout: GLArea is NULL\n");
	}

	GObject *splitView = gtk_builder_get_object(builder, "split_view");
	g_object_set(splitView, "content", tmpWidget, NULL);

	GtkEventController *eventController = gtk_event_controller_key_new();
	g_object_set(eventController, "propagation-phase", GTK_PHASE_CAPTURE, NULL);
	g_signal_connect(eventController, "key-pressed", G_CALLBACK(on_window_key_pressed), NULL);
	gtk_widget_add_controller(GTK_WIDGET(window), eventController);

	gtk_window_present(window);
	g_object_unref(builder);
}

// end public functions

static void show_about_dialog(GtkWindow *window) {
	adw_show_about_window(window,
						  "application-name", PROJECT_NAME,
						  "application-icon", ICON_NAME,
						  "developer-name", "Nicola Revelant",
						  "version", PROJECT_VERSION,
						  "copyright", PROJECT_COPYRIGHT,
						  "comments", PROJECT_DESCRIPTION,
						  "license", PROJECT_LICENSE,
						  "developers", authors,
						  "designers", designers,
						  NULL);
}


/*        --- EVENTS ---               */

void on_window_destroy() {
	debug_printerr(" [DD] On destroy\n");
}

gboolean on_window_key_pressed(GtkEventControllerKey *, guint key, guint, GdkModifierType state, gpointer) {
	debug_printerr(" [DD] On key pressed, keyval: %X\n", key);

	if (ui_blocked) return TRUE;

	gboolean configChanged = FALSE;
	double translate_step = gtk_spin_button_get_value(GTK_SPIN_BUTTON(translateStep_spinBtn));
	double zoom_step = gtk_spin_button_get_value(GTK_SPIN_BUTTON(zoomStep_spinBtn));

	double translate_x = 0.0;
	double translate_y = 0.0;
	switch (key) {
		case GDK_KEY_F3:
			// print debug info
			g_print("--- DEBUG INFO ---\n");

			g_print("Mandelbrot X:    %lf\n", mb_tmp_config.x);
            g_print("Mandelbrot Y:    %lf\n", mb_tmp_config.y);
			g_print("Mandelbrot Zoom: %lf\n", mb_tmp_config.zoom);

			g_print("Julia X:    %lf\n", mb_tmp_config.julia_x);
            g_print("Julia Y:    %lf\n", mb_tmp_config.julia_y);
			g_print("Julia Zoom: %lf\n", mb_tmp_config.julia_zoom);

			g_print("--- END DEBUG ---\n");
			break;
		case GDK_KEY_w: // UP
		case GDK_KEY_k:
		case GDK_KEY_Up:
			translate_y = translate_step;
			configChanged = TRUE;
			break;
		case GDK_KEY_s: // DOWN
		case GDK_KEY_j:
		case GDK_KEY_Down:
			translate_y = -translate_step;
			configChanged = TRUE;
			break;
		case GDK_KEY_a: // LEFT
		case GDK_KEY_h:
		case GDK_KEY_Left:
			translate_x = -translate_step;
			configChanged = TRUE;
			break;
		case GDK_KEY_d: // RIGHT
		case GDK_KEY_l:
		case GDK_KEY_Right:
			translate_x = translate_step;
			configChanged = TRUE;
			break;
		case GDK_KEY_z: // ZOOM OUT
		case GDK_KEY_minus:
			if (mb_tmp_config.use_julia)
				mb_tmp_config.julia_zoom /= zoom_step;
			else
				mb_tmp_config.zoom /= zoom_step;
			configChanged = TRUE;
			break;
		case GDK_KEY_q: // ZOOM IN
		case GDK_KEY_plus:
			if (mb_tmp_config.use_julia)
				mb_tmp_config.julia_zoom *= zoom_step;
			else
				mb_tmp_config.zoom *= zoom_step;
			configChanged = TRUE;
			break;
		default:
			configChanged = FALSE;
	}

	if (translate_x || translate_y) {
		if (mb_tmp_config.use_julia) {
			if (state & GDK_CONTROL_MASK) {
				mb_tmp_config.x += translate_x / ((state & GDK_ALT_MASK) ? mb_tmp_config.julia_zoom : mb_tmp_config.zoom);
				mb_tmp_config.y += translate_y / ((state & GDK_ALT_MASK) ? mb_tmp_config.julia_zoom : mb_tmp_config.zoom);
			} else {
				mb_tmp_config.julia_x += translate_x / mb_tmp_config.julia_zoom;
				mb_tmp_config.julia_y += translate_y / mb_tmp_config.julia_zoom;
			}
		} else {
			mb_tmp_config.x += translate_x / mb_tmp_config.zoom;
			mb_tmp_config.y += translate_y / mb_tmp_config.zoom;
		}
	}

	if (configChanged) glAreaUpdate();
	return configChanged; // if TRUE it blocks event propagation
}

void switch_fractal() {
	if (mb_tmp_config.use_julia)
		mb_tmp_config.use_julia = 0;
	else
		mb_tmp_config.use_julia = 1;
	
	glAreaUpdate();
}
