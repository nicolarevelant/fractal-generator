#include "fractal.h"
#include <signal.h>
#include <unistd.h>
#include <libpng16/png.h>
#include <pthread.h>
#include <semaphore.h>

// for gen photos
static int mb_gen_status;
static int mb_gen_rows; // generated rows, for progress
static sem_t mb_add_semaphore;
static int use_sem; // if true mb_thread will count the generated rows
static uint8_t *image_data; // image data used by both libpng and ffmpeg
static volatile int generate_more_frames;

// attributes
static int mb_width, mb_height, mb_max_iterations, use_julia;
static double fractal_tx, fractal_ty, julia_x0, julia_y0, fractal_zoom;
static png_color *mb_palette;

typedef struct {
	int start_row;
	int row_step;
} fractal_thread_args;

typedef struct {
	mb_on_progress_t on_progress;
	mb_on_save_t on_save;

	char *filename;
	int threads;
} mb_photo_args;

typedef struct {
	mb_on_progress_t on_progress;
	mb_on_save_t on_save;

	char *filename;
	int threads;
	int framerate;
	double zoom_step;
} mb_video_args;

// private methods
static void *photo_thread(void *void_args);
static void *video_thread(void *void_args);
static void *fractal_thread(void *void_args);
/**
 * 'xc' and 'yc' are the coordinates of the cartesian plane
*/
static png_color mb_get_color_from_pos(double xc, double yc);

/**
 * 'x0' and 'y0' are the starting point
 * 'xc' and 'yc' are the coordinates of the cartesian plane
*/
static png_color julia_get_color_from_pos(double x0, double y0, double xc, double yc);
static void mb_prepare(fractal_config_t *config);

fractal_error_t mb_video_stop() {
	generate_more_frames = 0;
	return MB_OK;
}

extern fractal_error_t fractal_begin_photo(fractal_config_t *config,
                          char *filename, mb_on_progress_t on_progress, mb_on_save_t on_save) {
	if (!config || !filename || !on_save)
		return MB_ERROR;

	if (mb_gen_status)
		return MB_EXEC;
	mb_gen_status = 1;
	mb_prepare(config);

	pthread_t pid;
	mb_photo_args *mb_args = malloc(sizeof (mb_photo_args));
	mb_args->on_progress = on_progress;
	mb_args->on_save = on_save;
	mb_args->filename = filename;
	mb_args->threads = config->threads;
    pthread_create(&pid, NULL, photo_thread, mb_args);
	on_progress(0);

	return MB_OK;
}

static void *photo_thread(void *void_args) {
	mb_photo_args *mb_args = void_args;
	mb_on_progress_t on_progress = mb_args->on_progress;
	mb_on_save_t on_save = mb_args->on_save;
	char *filename = mb_args->filename;
	int mb_threads = mb_args->threads;
	free(mb_args);

	fractal_thread_args **thread_args = malloc(sizeof (void *) * mb_threads);
	pthread_t *thread_ids = malloc(sizeof (pthread_t) * mb_threads);
	use_sem = 0;
	if (on_progress) {
		sem_init(&mb_add_semaphore, 0, 1);
		mb_gen_rows = 0;
		use_sem = 1;
	}

	png_image image = {0};
	image.format = PNG_FORMAT_RGB;
	image.version = PNG_IMAGE_VERSION;
	image.width = mb_width;
	image.height = mb_height;
	image_data = malloc(PNG_IMAGE_SIZE(image));

    int creation_result;
	for (int i = 0; i < mb_threads; i++) {
        thread_args[i] = malloc(sizeof (fractal_thread_args));
        thread_args[i]->start_row = i;
        thread_args[i]->row_step = mb_threads;
        creation_result = pthread_create(thread_ids + i, NULL, fractal_thread, thread_args[i]);
        if (creation_result) {
            fprintf(stderr, "ERROR: Cannot create threads\n");
            exit(creation_result);
        }
	}

	if (on_progress) {
		const struct timespec delay = {0, PROGRESS_DELAY_NANOSECONDS};
		while (mb_gen_rows < mb_height) {
			on_progress((float) mb_gen_rows / mb_height);
			nanosleep(&delay, NULL);
		}
	}

	for (int i = 0; i < mb_threads; i++) {
		pthread_join(thread_ids[i], NULL);
		free(thread_args[i]);
	}

	free(thread_ids);
	free(thread_args);
	if (on_progress)
		sem_destroy(&mb_add_semaphore);

	int success = png_image_write_to_file(&image, filename, 0, image_data, 0, NULL);
	if (!success) {
		fprintf(stderr, "Libpng error: %s\n", image.message);
	}
	png_image_free(&image);
	free(image_data);

	mb_gen_status = 0;
	on_save(success);

    return NULL;
}

fractal_error_t fractal_begin_video(fractal_config_t *config, mb_video_config_t *video_config,
		char *filename, mb_on_progress_t on_progress, mb_on_save_t on_save) {
	if (!config || !video_config || !filename || !on_progress || !on_save)
		return MB_ERROR;

	if (mb_gen_status)
		return MB_EXEC;
	mb_gen_status = 1;
	mb_prepare(config);

	fractal_zoom = video_config->zoom_start * config->height;
	generate_more_frames = 1;

	pthread_t pid;
	mb_video_args *mb_args = malloc(sizeof (mb_video_args));
	mb_args->on_progress = on_progress;
	mb_args->on_save = on_save;
	mb_args->filename = filename;
	mb_args->threads = config->threads;
	mb_args->framerate = video_config->frame_rate;
	mb_args->zoom_step = video_config->zoom_step;
	pthread_create(&pid, NULL, video_thread, mb_args);
	on_progress(0);

	return MB_OK;
}

static void *video_thread(void *void_args) {
	mb_video_args *mb_args = void_args;
	mb_on_progress_t on_progress = mb_args->on_progress;
	mb_on_save_t on_save = mb_args->on_save;
	char *filename = mb_args->filename;
	int mb_threads = mb_args->threads;
	int mb_framerate = mb_args->framerate;
	double mb_zoom_step = mb_args->zoom_step;
	free(mb_args);
	
	char *video_title = malloc(1024);
	snprintf(video_title, 1024, "Fractal cartesian coordinates: (%.4lf , %.4lf)", fractal_tx, fractal_ty);
	AVDictionary *metadata = NULL;
	av_dict_set(&metadata, "title", video_title, 0);
	av_dict_set(&metadata, "copyright", "Nicola Revelant", 0);
	free(video_title);

	VideoCtx *video_ctx = video_ctx_new(NULL, filename, mb_width, mb_height,
			mb_framerate, AV_PIX_FMT_RGB24, metadata);
	
	if (!video_ctx) {
		on_save(0);
		return NULL;
	}
	image_data = malloc(mb_width * mb_height * 3);

	int stride = mb_width * 3;
	fractal_thread_args **thread_args = malloc(sizeof (void *) * mb_threads);
	pthread_t *thread_ids = malloc(sizeof (pthread_t) * mb_threads);

	for (int i = 0; i < mb_threads; i++) {
        thread_args[i] = malloc(sizeof (fractal_thread_args));
	}
	
	use_sem = 0;
    int creation_result;
	while (generate_more_frames) {
		for (int i = 0; i < mb_threads; i++) {
            thread_args[i]->start_row = i;
            thread_args[i]->row_step = mb_threads;
			creation_result = pthread_create(thread_ids + i, NULL, fractal_thread, thread_args[i]);
            if (creation_result) {
                fprintf(stderr, "ERROR: Cannot create threads\n");
                exit(creation_result);
            }
		}

		for (int i = 0; i < mb_threads; i++) {
			pthread_join(thread_ids[i], NULL);
		}

		int pts = video_send_frame(video_ctx, image_data, stride);
		if (pts < 0) {
			on_save(0);
			return NULL;
		}

		on_progress((float) pts / mb_framerate);
		fractal_zoom *= mb_zoom_step;
	}

	// flush the stream and save the file
	video_ctx_free(video_ctx);

	for (int i = 0; i < mb_threads; i++) {
		free(thread_args[i]);
	}

	free(thread_ids);
	free(thread_args);
	free(image_data);

	mb_gen_status = 0;
	on_save(1);

    return NULL;
}

static void *fractal_thread(void *void_args) {
	fractal_thread_args *tArgs = void_args;
	int start_row = tArgs->start_row, row_step = tArgs->row_step;
	double xc, yc;
	double halfWidth = mb_width / 2.0, halfHeight = mb_height / 2.0;
	int data_index;
	png_color color;

	// scan pixels
	for (int row = start_row, col; row < mb_height; row += row_step) {
		for (col = 0; col < mb_width; col++) {

			xc = fractal_tx + (col - halfWidth) / fractal_zoom;
			yc = fractal_ty - (row - halfHeight) / fractal_zoom;

			if (use_julia)
				color = julia_get_color_from_pos(julia_x0, julia_y0, xc, yc);
			else
				color = mb_get_color_from_pos(xc, yc);
			data_index = 3 * (row * mb_width + col);
			image_data[data_index] = color.red;
			image_data[data_index + 1] = color.green;
			image_data[data_index + 2] = color.blue;
		}
		if (use_sem) {
			sem_wait(&mb_add_semaphore);
			mb_gen_rows++;
			sem_post(&mb_add_semaphore);
		}
	}

    return NULL;
}

static png_color mb_get_color_from_pos(double xc, double yc) {
	// 'x', 'y', 'xx' and 'yy' are calculation variables
	double x = xc, y = yc, xx, yy;
	int iterations = 0;

	while (iterations < mb_max_iterations) {
		xx = x * x;
		yy = y * y;
		if (xx + yy > 4.0) break;
		y = 2.0 * x * y + yc;
		x = xx - yy + xc;
		iterations++;
	}
	
	return mb_palette[iterations];
}

static png_color julia_get_color_from_pos(double x0, double y0, double xc, double yc) {
	// 'x', 'y', 'xx' and 'yy' are calculation variables
	double x = xc, y = yc, xx, yy;
	int iterations = 0;

	while (iterations < mb_max_iterations) {
		xx = x * x;
		yy = y * y;
		if (xx + yy > 4.0) break;
		y = 2.0 * x * y + y0;
		x = xx - yy + x0;
		iterations++;
	}
	
	return mb_palette[iterations];
}

static void mb_prepare(fractal_config_t *config) {
	fractal_config_t c = *config;

	if (c.use_julia) {
		// use Julia
		use_julia = 1;
		julia_x0 = c.x;
		julia_y0 = c.y;
		fractal_zoom = c.julia_zoom * c.height / 2.0;

		fractal_tx = c.julia_x;
		fractal_ty = c.julia_y;
	} else {
		// use Mandelbrot
		use_julia = 0;
		fractal_tx = c.x;
		fractal_ty = c.y;
		fractal_zoom = c.zoom * c.height / 2.0;
	}
	
	mb_width = c.width;
	mb_height = c.height;
	
	if (mb_max_iterations != c.max_iterations) {
        mb_max_iterations = c.max_iterations;
		free(mb_palette);
		mb_palette = malloc(3 * (mb_max_iterations + 1));
		
		double t;
		for (int i = 0; i <= mb_max_iterations; i++) {
			t = (double) i / mb_max_iterations;

			// color algorithm (default 9.4 ; 15.9 ; 9.4)
			mb_palette[i].red = (png_byte)(256.0 * 9.4 * (1.0 - t) * t * t * t);
			mb_palette[i].green = (png_byte)(256.0 * 15.9 * (1.0 - t)*(1.0 - t) * t * t);
			mb_palette[i].blue = (png_byte)(256.0 * 9.4 * (1.0 - t)*(1.0 - t)*(1.0 - t) * t);
		}
	}
}
