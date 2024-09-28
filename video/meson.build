dependencies = [
    dependency('libavcodec'),
    dependency('libavformat'),
    dependency('libavutil'),
    dependency('libswresample'),
    dependency('libswscale')
]

video = library('video', 'video.c',
    dependencies: dependencies,
    link_args: '-lm'
)
