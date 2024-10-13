dependencies = [
    dependency('libpng'),
    dependency('libavutil'),
    dependency('threads')
]

fractal = library('fractal',
    'fractal.c',
    link_with: [video],
    dependencies: dependencies
)
