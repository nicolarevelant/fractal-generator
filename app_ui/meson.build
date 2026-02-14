files = files(
    'app_ui.c',
    'app_ui_gl_area.c',
    'app_ui_home.c',
    'app_ui_settings.c',
    'app_ui_utils.c',
    'app_ui_utils.h',
)

dependencies = [
    dependency('libadwaita-1'),
    dependency('gl'),
    dependency('epoxy'),
]

app_ui = static_library(
    'app_ui',
    sources: files,
    dependencies: dependencies,
    link_with: [fractal],
    include_directories: include_directories('..'),
)