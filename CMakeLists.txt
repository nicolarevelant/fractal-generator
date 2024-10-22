cmake_minimum_required(VERSION 3.20)
project(fractal-generator
		DESCRIPTION "Mandelbrot and Julia fractal generator"
		VERSION 1.0.0
		LANGUAGES C)

include(GNUInstallDirs)

set(PROJECT_COPYRIGHT "Copyright (C) 2023 Nicola Revelant")
set(PROJECT_LICENSE "GNU General Public License, version 3 or later")
set(ICON_NAME "fractal-generator")
set(DATADIR "${CMAKE_INSTALL_FULL_DATADIR}/${PROJECT_NAME}")
set(LOCALEDIR "${CMAKE_INSTALL_FULL_LOCALEDIR}")

set(CMAKE_C_EXTENSIONS OFF)
set(CMAKE_C_STANDARD 99)
set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -Wall -Wextra -Werror -DGDK_DISABLE_DEPRECATED -DGTK_DISABLE_DEPRECATED")
set(CMAKE_C_FLAGS_DEBUG "${CMAKE_C_FLAGS_DEBUG} -DDEBUG")

find_package(PkgConfig REQUIRED)
pkg_check_modules(LIBS REQUIRED libadwaita-1 gl libpng libavcodec libavformat libavutil libswresample libswscale)
include_directories(${LIBS_INCLUDE_DIRS} ${CMAKE_BINARY_DIR})
add_definitions(${LIBS_CFLAGS_OTHER})

add_subdirectory(app_ui)
add_subdirectory(fractal)
add_subdirectory(video)
add_subdirectory(glad)

add_executable(${PROJECT_NAME} main.c)
target_link_libraries(${PROJECT_NAME} app_ui fractal video glad)
target_link_libraries(${PROJECT_NAME} ${LIBS_LIBRARIES} m)

configure_file(project_variables.h.in project_variables.h)

install(FILES "gschemas/com.nicolarevelant.fractal-generator.gschema.xml"
		DESTINATION "${CMAKE_INSTALL_DATADIR}/glib-2.0/schemas")
