dependencies = [
    dependency('libpng'),
    dependency('libavutil')
]

fractal = library('fractal',
    'fractal.c',
    link_with: [video],
    link_args: '-lm',
    dependencies: dependencies
)
