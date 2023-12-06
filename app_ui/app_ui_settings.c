#include "app_ui_utils.h"

static void maxi_widget_onchange(GtkSpinButton *source);

GtkWidget *create_settings_layout() {
	GtkBuilder *builder = gtk_builder_new();
	GError *error = NULL;
	if (!gtk_builder_add_from_file(builder, DATA_PATH"ui/app_ui_settings.ui", &error)) {
		debug_printerr(" [EE] Cannot create settings layout: %s\n", error->message);
		g_error_free(error);
		return NULL;
	}

	GSettings *settings = g_settings_new(GIO_SETTINGS_SCHEMA);
	if (!settings) {
		return NULL;
	}
	
	GtkSpinButton *spinMaxIter = GTK_SPIN_BUTTON(gtk_builder_get_object(builder, "spinMaxIter"));
	g_settings_bind(settings, "max-iterations", spinMaxIter, "value", G_SETTINGS_BIND_DEFAULT);
	g_signal_connect(spinMaxIter, "value-changed", G_CALLBACK(maxi_widget_onchange), NULL);
	mb_tmp_config.max_iterations = (int)(gtk_spin_button_get_value(spinMaxIter));

	translateStep_spinBtn = GTK_SPIN_BUTTON(gtk_builder_get_object(builder, "spinTranslateStep"));
	g_settings_bind(settings, "translate-step", translateStep_spinBtn, "value", G_SETTINGS_BIND_DEFAULT);

	zoomStep_spinBtn = GTK_SPIN_BUTTON(gtk_builder_get_object(builder, "spinZoomStep"));
	g_settings_bind(settings, "zoom-step", zoomStep_spinBtn, "value", G_SETTINGS_BIND_DEFAULT);

	video_init_zoom = GTK_SPIN_BUTTON(gtk_builder_get_object(builder, "spinVideoInitZoom"));
	g_settings_bind(settings, "video-init-zoom", video_init_zoom, "value", G_SETTINGS_BIND_DEFAULT);

	video_zoom_speed = GTK_SPIN_BUTTON(gtk_builder_get_object(builder, "spinVideoZoomSpeed"));
	g_settings_bind(settings, "video-zoom-speed", video_zoom_speed, "value", G_SETTINGS_BIND_DEFAULT);

	frameSizeDropDown = GTK_DROP_DOWN(gtk_builder_get_object(builder, "frameSize"));

	GObject *settingsGrid = gtk_builder_get_object(builder, "settingsGrid");
	g_object_ref(settingsGrid);
	g_object_unref(builder);

	return GTK_WIDGET(settingsGrid);
}

void maxi_widget_onchange(GtkSpinButton *source) {
	mb_tmp_config.max_iterations = (int)(gtk_spin_button_get_value(source));
    glAreaUpdate();
}
