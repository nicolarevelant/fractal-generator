project('fractal-generator', 'c',
    license: 'GNU General Public License, version 3 or later',
    version: '1.0.0'
)

subdir('po')
subdir('data')
subdir('video')
subdir('fractal')
subdir('glad')
subdir('app_ui')

files = files(
    'main.c'
)

conf_data = configuration_data()
conf_data.set_quoted('PROJECT_NAME', meson.project_name())
conf_data.set_quoted('PROJECT_DESCRIPTION', 'Mandelbrot and Julia fractal generator')
conf_data.set_quoted('PROJECT_VERSION', meson.project_version())
conf_data.set_quoted('PROJECT_LICENSE', meson.project_license())
conf_data.set_quoted('PROJECT_COPYRIGHT', 'Copyright (C) 2024 Nicola Revelant')
conf_data.set_quoted('ICON_NAME', meson.project_name())
conf_data.set_quoted('DATADIR', get_option('prefix') / get_option('datadir') / meson.project_name())
conf_data.set_quoted('LOCALEDIR', get_option('prefix') / get_option('localedir'))
conf_data.set('DEBUG', get_option('buildtype') == 'debug')

configure_file(output: 'project_variables.h',
    configuration: conf_data
)

executable(meson.project_name(),
    sources: files,
    link_with: [app_ui],
    link_args: '-lm',
    include_directories: include_directories('.'),
    install: true
)

install_data('gschemas/com.nicolarevelant.fractal-generator.gschema.xml',
    install_dir: get_option('datadir') / 'glib-2.0' / 'schemas'
)
