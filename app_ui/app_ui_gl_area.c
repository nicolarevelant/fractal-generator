#include "app_ui_utils.h"
#include "../glad/glad.h"

#define VERTEX_SHADER_PATH DATA_PATH"shader/vertex.glsl"
#define FRAGMENT_SHADER_PATH DATA_PATH"shader/fragment.glsl"
#define BUF_SIZE 2048

static const GLfloat vertices[] = {
	-1.0f, -1.0f, 0.0f,
	-1.0f, 1.0f, 0.0f,
	1.0f, 1.0f, 0.0f,
	1.0f, 1.0f, 0.0f,
	1.0f, -1.0f, 0.0f,
	-1.0f, -1.0f, 0.0f
};

void on_window_left_pressed(GtkGestureClick*, gint, gdouble x, gdouble y, gpointer);
void on_window_right_pressed(GtkGestureClick*, gint, gdouble x, gdouble y, gpointer);

static void on_realize();
static gboolean on_render();
static void on_resize(GtkGLArea *, int width, int height);
static void init_program();
static void loadPalette(int maxItr);
static char *readShaderFromFile(char *filename);

static GtkGLArea *glArea;
static GLint mbLocation, juliaLocation, ratioLocation, maxItrLocation, paletteLocation;
static GLuint shaderProgram;
static float gl_palette[3 * (PV_MAX_ITERATION_LIMIT + 1)];

GtkWidget *create_gl_area() {
	glArea = GTK_GL_AREA(gtk_gl_area_new());
	g_object_set(glArea, "width-request", 300, "height-request", 300, NULL);
	gtk_gl_area_set_required_version(glArea, 4, 0);

	GtkGesture *gestureController = gtk_gesture_click_new();
	g_object_set(gestureController, "propagation-phase", GTK_PHASE_CAPTURE, "button", GDK_BUTTON_PRIMARY, NULL);
	g_signal_connect(gestureController, "pressed", G_CALLBACK(on_window_left_pressed), NULL);
	gtk_widget_add_controller(GTK_WIDGET(glArea), GTK_EVENT_CONTROLLER(gestureController));

	gestureController = gtk_gesture_click_new();
	g_object_set(gestureController, "propagation-phase", GTK_PHASE_CAPTURE, "button", GDK_BUTTON_SECONDARY, NULL);
	g_signal_connect(gestureController, "pressed", G_CALLBACK(on_window_right_pressed), NULL);
	gtk_widget_add_controller(GTK_WIDGET(glArea), GTK_EVENT_CONTROLLER(gestureController));

	g_signal_connect(glArea, "realize", G_CALLBACK(on_realize), NULL);
	g_signal_connect(glArea, "render", G_CALLBACK(on_render), NULL);
	g_signal_connect(glArea, "resize", G_CALLBACK(on_resize), NULL);

	return GTK_WIDGET(glArea);
}

void on_window_left_pressed(GtkGestureClick*, gint, gdouble x, gdouble y, gpointer) {
	GSettings *settings = g_settings_new(GIO_SETTINGS_SCHEMA);
	if (!settings) {
		return;
	}
	int width = gtk_widget_get_width(GTK_WIDGET(glArea));
	int height = gtk_widget_get_height(GTK_WIDGET(glArea));
	g_printerr("Left: %lf %lf\n", x / width, y / height);
	double zoom = g_settings_get_double(settings, "mouse-zoom-step");

	double relx = (x - width / 2.0);
	double rely = (y - height / 2.0);
	if (mb_tmp_config.use_julia) {
		double hzoom = mb_tmp_config.julia_zoom * height;
		mb_tmp_config.julia_x += 2.0 * ( relx - relx / zoom ) / hzoom;
		mb_tmp_config.julia_y -=  2.0 * ( rely - rely / zoom ) / hzoom;
		mb_tmp_config.julia_zoom *= zoom;
	} else {
		double hzoom = mb_tmp_config.zoom * height;
		mb_tmp_config.x += 2.0 * ( relx - relx / zoom ) / hzoom;
		mb_tmp_config.y -= 2.0 * ( rely - rely / zoom ) / hzoom;
		mb_tmp_config.zoom *= zoom;
	}

	glAreaUpdate();
}

void on_window_right_pressed(GtkGestureClick*, gint, gdouble x, gdouble y, gpointer) {
	GSettings *settings = g_settings_new(GIO_SETTINGS_SCHEMA);
	if (!settings) {
		return;
	}
	int width = gtk_widget_get_width(GTK_WIDGET(glArea));
	int height = gtk_widget_get_height(GTK_WIDGET(glArea));
	g_printerr("Right: %lf %lf\n", x / width, y / height);
	double zoom = g_settings_get_double(settings, "mouse-zoom-step");

	double relx = (x - width / 2.0);
	double rely = (y - height / 2.0);
	if (mb_tmp_config.use_julia) {
		mb_tmp_config.julia_zoom /= zoom;
		double hzoom = mb_tmp_config.julia_zoom * height;
		mb_tmp_config.julia_x -= 2.0 * ( relx - relx / zoom ) / hzoom;
		mb_tmp_config.julia_y += 2.0 * ( rely - rely / zoom ) / hzoom;
	} else {
		mb_tmp_config.zoom /= zoom;
		double hzoom = mb_tmp_config.zoom * height;
		mb_tmp_config.x -= 2.0 * ( relx - relx / zoom ) / hzoom;
		mb_tmp_config.y += 2.0 * ( rely - rely / zoom ) / hzoom;
	}

	glAreaUpdate();
}

void on_realize() {
	if (!glArea) {
		debug_printerr(" [WW] glArea.on_realize: glArea is NULL\n");
		return;
	}
    gtk_gl_area_make_current(glArea);
    if (!gladLoadGL()) {
		debug_printerr(" [EE] OpenGL not supported\n");
        return;
    }

	// Print version info
    debug_printerr(" [DD] Renderer: %s\n", glGetString(GL_RENDERER));
	debug_printerr(" [DD] OpenGL version supported: %s\n", glGetString(GL_VERSION));

	debug_printerr(" [DD] OpenGL %d.%d\n", GLVersion.major, GLVersion.minor);

    init_program();
}

gboolean on_render() {
	if (!glArea) {
		g_printerr(" [WW] glArea is NULL\n");
		return TRUE;
	}
	glDrawArrays(GL_TRIANGLES, 0, 6);
	return TRUE;
}

void on_resize(GtkGLArea *, int width, int height) {
    glUniform1f(ratioLocation, (float) width / height);
}

void glAreaUpdate() {
	if (!glArea) {
		debug_printerr(" [EE] glArea is NULL\n");
		return;
	}

	gtk_gl_area_make_current(glArea);

	GLdouble x = mb_tmp_config.x;
	GLdouble y = mb_tmp_config.y;
	GLdouble zoom = mb_tmp_config.zoom;

	GLdouble julia_x = mb_tmp_config.julia_x;
	GLdouble julia_y = mb_tmp_config.julia_y;
	GLdouble julia_zoom = mb_tmp_config.use_julia ? mb_tmp_config.julia_zoom : 0.0;

	glUniform3d(mbLocation, x, y, zoom);
	glUniform3d(juliaLocation, julia_x, julia_y, julia_zoom);

	glUniform1i(maxItrLocation, mb_tmp_config.max_iterations);
	loadPalette(mb_tmp_config.max_iterations);

	gtk_gl_area_queue_render(glArea);
}

void init_program() {
	int success;
	char infoLog[BUF_SIZE];
	char *shaderSource;

	shaderSource = readShaderFromFile(VERTEX_SHADER_PATH);
	if (!shaderSource) {
		debug_printerr(" [EE] Vertex shader not found\n");
		exit(1);
	}

	GLuint vertexShader = glCreateShader(GL_VERTEX_SHADER);
	glShaderSource(vertexShader, 1, (const GLchar *const *) &shaderSource, NULL);
	free(shaderSource);

	glCompileShader(vertexShader);
	glGetShaderiv(vertexShader, GL_COMPILE_STATUS, &success);
	if (!success) {
		glGetShaderInfoLog(vertexShader, BUF_SIZE, NULL, infoLog);
		debug_printerr(" [EE] OpenGL compile error for the vertex shader\n%s\n", infoLog);
		exit(1);
	}

	shaderSource = readShaderFromFile(FRAGMENT_SHADER_PATH);
	if (!shaderSource) {
		debug_printerr(" [EE] OpenGL fragment shader not found\n");
		exit(1);
	}

	GLuint fragmentShader = glCreateShader(GL_FRAGMENT_SHADER);
	glShaderSource(fragmentShader, 1, (const GLchar *const *) &shaderSource, NULL);
	free(shaderSource);

	glCompileShader(fragmentShader);
	glGetShaderiv(fragmentShader, GL_COMPILE_STATUS, &success);
	if (!success) {
		glGetShaderInfoLog(fragmentShader, BUF_SIZE, NULL, infoLog);
		debug_printerr(" [EE] OpenGL compile error for the fragment shader\n%s\n", infoLog);
		exit(1);
	}

	shaderProgram = glCreateProgram();
	glAttachShader(shaderProgram, vertexShader);
	glAttachShader(shaderProgram, fragmentShader);
	glLinkProgram(shaderProgram);
	glGetProgramiv(shaderProgram, GL_LINK_STATUS, &success);
	if(!success) {
		glGetProgramInfoLog(shaderProgram, BUF_SIZE, NULL, infoLog);
		debug_printerr(" [EE] OpenGL program linking error\n%s\n", infoLog);
		exit(1);
	}

	GLuint VAO;
	glGenVertexArrays(1, &VAO);
	glBindVertexArray(VAO);

	GLuint VBO;
	glGenBuffers(1, &VBO);
	glBindBuffer(GL_ARRAY_BUFFER, VBO);
	glBufferData(GL_ARRAY_BUFFER, sizeof(vertices), vertices, GL_STATIC_DRAW);

	mbLocation = glGetUniformLocation(shaderProgram, "mb");
    juliaLocation = glGetUniformLocation(shaderProgram, "julia");
    ratioLocation = glGetUniformLocation(shaderProgram, "ratio");
    maxItrLocation = glGetUniformLocation(shaderProgram, "maxItr");
	paletteLocation = glGetUniformLocation(shaderProgram, "palette");

	glUseProgram(shaderProgram);
	glDeleteShader(vertexShader);
	glDeleteShader(fragmentShader);

	glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 3 * sizeof(float), (void *)0);
	glEnableVertexAttribArray(0);

    glAreaUpdate();
}

void loadPalette(int maxItr) {
	double t;
	for (int i = 0; i <= maxItr; i++) {
		t = (double) i / maxItr;

		// color algorithm (default 9.4 ; 15.9 ; 9.4)
		gl_palette[3*i] = 9.4 * (1.0 - t) * t * t * t;
		gl_palette[3*i + 1] = 15.9 * (1.0 - t)*(1.0 - t) * t * t;
		gl_palette[3*i + 2] = 9.4 * (1.0 - t)*(1.0 - t)*(1.0 - t) * t;
	}

	glUniform3fv(paletteLocation, PV_MAX_ITERATION_LIMIT + 1, gl_palette);
}

char *readShaderFromFile(char *filename) {
	FILE *file = fopen(filename, "rb");
	if (!file) {
		debug_printerr(" [EE] Could not read file %s: %s\n", filename, strerror(errno));
        return NULL;
    }

	fseek(file, 0, SEEK_END);
	long int len = ftell(file);
	if (len < 0)
		return NULL;

	char *content = malloc(len + 1);
	rewind(file);
	if (fread(content, 1, len, file) != (size_t)len)
		return NULL;

	content[len] = '\0'; // null terminating character
	return content;
}