dependencies = [
    dependency('libpng'),
    dependency('libavutil'),
    dependency('threads')
]

fractal = static_library('fractal',
    'fractal.c',
    link_with: [video],
    dependencies: dependencies
)
