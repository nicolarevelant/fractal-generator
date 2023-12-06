# Mandelbrot and Julia fractal generator

Explore Mandelbrot and Julia fractals.

A button allows you to switch between Mandelbrot and Julia views.
When you switch from Mandelbrot to Julia, this program renders Julia using the current cartesian coordinates of Mandelbrot.

## Building

### At build-time dependencies

- CMake
- Ninja (or make)

### At runtime and build-time dependencies

- Libadwaita \>= 1.4
- OpenGL \>= 4

To install into ``/usr/local``, run:

```bash
cmake -B build -GNinja -DCMAKE_BUILD_TYPE=Release
cmake --build build
sudo cmake --install build
```

CMake cache variables:

| Variable name        | Description       | Type   | Default value  |
| -------------------- | ----------------- | ------ | -------------- |
| CMAKE_INSTALL_PREFIX | Installation path | String | ``/usr/local`` |

## Installation

> Note: system installation is not supported (yet)

Copy ``gschemas/*`` to ``/usr/share/glib-2.0/schemas/`` and compile them:

```console
# cp gschemas/* /usr/share/glib-2.0/schemas/
# glib-compile-schemas /usr/share/glib-2.0/schemas/
```

then run the program from the ``build`` directory:

```bash
cd build
./fractal-generator
```

## Usage

### Keybindings

- q, ``+`` - Zoom in
- z, ``-`` - Zoom out
- ``wasd``, ``hjkl``, ``Arrows`` - Translate the current fractal
- Ctrl+``wasd``, Ctrl+``hjkl``, Ctrl+``Arrows`` - Translate Mandelbrot \(even on Julia\)

### Mouse (on the preview)

- Left click - Zoom in
- Right click - Zoom out

# License

Copyright (C) 2023 Nicola Revelant

This program is free software: you can redistribute it and/or modify
it under the terms of the GNU General Public License as published by
the Free Software Foundation, either version 3 of the License, or
(at your option) any later version.

This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with this program.  If not, see <https://www.gnu.org/licenses/>.
