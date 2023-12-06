#include "app_ui_utils.h"

#define MB_DEFAULT_PHOTO_NAME "photo.png"
#define MB_DEFAULT_VIDEO_NAME "video.mp4"

#define STATE_PROGRESS_SIZE 128

static void btn_save_photo_clicked(GtkWidget *btn);
static void on_photo_dialog_response(GObject *fileDialog, GAsyncResult *res, gpointer data);

static void btn_save_video_clicked(GtkWidget *btn);
static void on_video_dialog_response(GObject *fileDialog, GAsyncResult *res, gpointer data);

static void btn_stop_video_clicked();

static gboolean ui_on_update(UpdateData *data);

static fractal_config_t get_final_config(fractal_config_t *old, int width, int height);

static gchar *mb_photo_filename, *mb_video_filename;
static char *stateProgressStr;
static int mb_width, mb_height, mb_framerate = VIDEO_FRAMERATE;

static GtkProgressBar *progressBar;
static GtkLabel *statusLabel;

GtkWidget *create_home_layout() {
	GtkBuilder *builder = gtk_builder_new();
	GError *error = NULL;
	if (!gtk_builder_add_from_file(builder, DATA_PATH"ui/app_ui_home.ui", &error)) {
		debug_printerr(" [EE] Cannot create home layout: %s\n", error->message);
		g_error_free(error);
		return NULL;
	}

	GObject *btn = gtk_builder_get_object(builder, "photoBtn");
	g_signal_connect(btn, "clicked", G_CALLBACK(btn_save_photo_clicked), NULL);
	btn = gtk_builder_get_object(builder, "videoBtn");
	g_signal_connect(btn, "clicked", G_CALLBACK(btn_save_video_clicked), NULL);
	stateProgressStr = malloc(STATE_PROGRESS_SIZE);
	
	btn = gtk_builder_get_object(builder, "stopBtn");
	g_signal_connect(btn, "clicked", G_CALLBACK(btn_stop_video_clicked), NULL);

	btn = gtk_builder_get_object(builder, "juliaBtn");
	g_signal_connect(btn, "clicked", G_CALLBACK(switch_fractal), NULL);

	progressBar = GTK_PROGRESS_BAR(gtk_builder_get_object(builder, "progressBar"));
	statusLabel = GTK_LABEL(gtk_builder_get_object(builder, "statusLabel"));

	GObject *homeBox = gtk_builder_get_object(builder, "homeBox");
	g_object_ref(homeBox);
	g_object_unref(builder);

	return GTK_WIDGET(homeBox);
}

void btn_save_photo_clicked(GtkWidget *btn) {
	if (ui_blocked) return; // TODO: mutex

	guint size_selected = gtk_drop_down_get_selected(GTK_DROP_DOWN(frameSizeDropDown));
	if (size_selected == GTK_INVALID_LIST_POSITION) return;

	mb_width = frameResolutions[size_selected].width;
	mb_height = frameResolutions[size_selected].height;

	GtkRoot *root = gtk_widget_get_root(btn);
	GtkFileDialog *fileDialog = gtk_file_dialog_new();

	g_object_set(fileDialog, "title", _("Photo"), "initial-name",
	             mb_photo_filename ? mb_photo_filename : MB_DEFAULT_PHOTO_NAME, NULL);

	gtk_file_dialog_save(fileDialog, GTK_WINDOW(root), NULL, on_photo_dialog_response, NULL);
}

void on_photo_dialog_response(GObject *fileDialog, GAsyncResult *res, __attribute__((unused)) gpointer data) {
	GFile *file = gtk_file_dialog_save_finish(GTK_FILE_DIALOG(fileDialog), res, NULL);
	if (file) {
		ui_blocked = 1; // TODO: mutex
		g_free(mb_photo_filename);
		mb_photo_filename = g_file_get_path(file);

		// create new configuration for photo (another resolution)
		fractal_config_t new_config = get_final_config(&mb_tmp_config, mb_width, mb_height);
		fractal_begin_photo(&new_config, mb_photo_filename, mb_on_photo_progress, image_on_save);
	} else {
		debug_printerr(" [DD] Photo dialog cancelled\n");
	}

	g_object_unref(fileDialog);
}

void btn_save_video_clicked(GtkWidget *btn) {
	if (ui_blocked) return; // TODO: mutex

	guint size_selected = gtk_drop_down_get_selected(GTK_DROP_DOWN(frameSizeDropDown));
	if (size_selected == GTK_INVALID_LIST_POSITION) return;

	mb_width = frameResolutions[size_selected].width;
	mb_height = frameResolutions[size_selected].height;

	GtkRoot *root = gtk_widget_get_root(btn);
	GtkFileDialog *fileDialog = gtk_file_dialog_new();

	g_object_set(fileDialog, "title", _("Video"), "initial-name",
	             mb_video_filename ? mb_video_filename : MB_DEFAULT_VIDEO_NAME, NULL);

	gtk_file_dialog_save(fileDialog, GTK_WINDOW(root), NULL, on_video_dialog_response, NULL);
}

void on_video_dialog_response(GObject *fileDialog, GAsyncResult *res, __attribute__((unused)) gpointer data) {
	GFile *file = gtk_file_dialog_save_finish(GTK_FILE_DIALOG(fileDialog), res, NULL);
	if (file) {
		ui_blocked = 1;
		g_free(mb_video_filename);
		mb_video_filename = g_file_get_path(file);

		// create new configuration for video (another resolution)
		fractal_config_t new_config = get_final_config(&mb_tmp_config, mb_width, mb_height);

		mb_video_config_t video_config = {
				gtk_spin_button_get_value(GTK_SPIN_BUTTON(video_init_zoom)), // initial zoom value
				gtk_spin_button_get_value(GTK_SPIN_BUTTON(video_zoom_speed)), // zoom step
				mb_framerate
		};

		fractal_begin_video(&new_config, &video_config, mb_video_filename, mb_on_video_progress, video_on_save);
	} else {
		debug_printerr(" [DD] Video dialog cancelled\n");
	}

	g_object_unref(fileDialog);
}


static void btn_stop_video_clicked() {
	mb_video_stop();
}

gboolean ui_on_update(UpdateData *data) {
	update_state_t state = data->state;
	float progress = data->progress;
	free(data);

	switch (state) {
		case IDLE:
			gtk_progress_bar_set_fraction(progressBar, 0);
			gtk_label_set_text(statusLabel, _("Idle"));
			ui_blocked = 0;
			break;
		case PHOTO_PROGRESS:
			snprintf(stateProgressStr, STATE_PROGRESS_SIZE, _("Progress: %.2f %%"), progress * 100.0f);
			gtk_label_set_text(statusLabel, stateProgressStr);
			gtk_progress_bar_set_fraction(progressBar, progress);
			break;
		case VIDEO_PROGRESS:
			snprintf(stateProgressStr, STATE_PROGRESS_SIZE, _("Progress: %02d:%02d.%02d"),
					(int) progress / 60,  // minutes
					(int) progress % 60,  // seconds
					(int)((progress - (int)progress) * mb_framerate));
			gtk_label_set_text(statusLabel, stateProgressStr);
			//gtk_progress_bar_set_fraction(progressBar, progress);
			break;
		case ERR_SAVE:
			gtk_progress_bar_set_fraction(progressBar, 0);
			gtk_label_set_text(statusLabel, _("Save error"));
			ui_blocked = 0;
			break;
		default:
			return G_SOURCE_REMOVE;
	}

	return G_SOURCE_REMOVE;
}

//              ASYNC CALLBACKS (from other threads)

void mb_on_photo_progress(float p) {
	UpdateData *data = malloc(sizeof(UpdateData));
	data->state = PHOTO_PROGRESS;
	data->progress = p;

	g_idle_add(G_SOURCE_FUNC(ui_on_update), data);
}

void mb_on_video_progress(float p) {
	UpdateData *data = malloc(sizeof(UpdateData));
	data->state = VIDEO_PROGRESS;
	data->progress = p;

	g_idle_add(G_SOURCE_FUNC(ui_on_update), data);
}

void image_on_save(int is_success) {
	UpdateData *data = malloc(sizeof(UpdateData));
	data->state = is_success ? IDLE : ERR_SAVE;
	g_idle_add(G_SOURCE_FUNC(ui_on_update), data);
}

void video_on_save(int is_success) {
	UpdateData *data = malloc(sizeof(UpdateData));
	data->state = is_success ? IDLE : ERR_SAVE;
	g_idle_add(G_SOURCE_FUNC(ui_on_update), data);
}

static fractal_config_t get_final_config(fractal_config_t *old, int width, int height) {
	fractal_config_t new_config = *old;
	new_config.width = width;
	new_config.height = height;
	return new_config;
}