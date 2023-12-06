#include "app_ui_utils.h"

fractal_config_t mb_tmp_config = {
		//0.3983541193151654, // x
		//0.19514111158909933, // y
		0.285, 0, 100.0, // x, y, zoom

		0, 0, 1.0, // julia_x, julia_y, julia_zoom

		0, // use_julia

		0, 0, // width, height
		0, // max_iterations
		8, // threads
};

int ui_blocked = 0;

const Size frameResolutions[] = {
		{ 1024,   576    }, // "576p SD (16:9)"
		{ 1920,   1080   }, // "1080p FHD (16:9)"
		{ 1800,   1200   }, // "2.16 MP (3:2)"
		{ 1920*2, 1080*2 }, // "4K UHD (16:9)"
		{ 3264,   2448   }, // "8.0 MP (4:3)"
		{ 6000,   4000   }, // "24 MP (3:2)"
		{ 1920*4, 1080*4 }, // "8K UHD (16:9)"
		{0}
};

const char *videoItems[] = {
		NULL
};

const Size videoResolutions[] = {
		{0}
};

// settings UI Widgets
GtkSpinButton *translateStep_spinBtn;
GtkSpinButton *zoomStep_spinBtn;
GtkSpinButton *video_init_zoom;
GtkSpinButton *video_zoom_speed;
GtkDropDown *frameSizeDropDown;
