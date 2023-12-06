#ifndef VIDEO_H
#define VIDEO_H

#include <stdio.h>
#include <stdlib.h>
#include <libavformat/avformat.h>
#include <libavcodec/avcodec.h>
#include <libavutil/timestamp.h>
#include <libavutil/pixdesc.h>
#include <libavutil/opt.h>
#include <libswscale/swscale.h>
#include <libswresample/swresample.h>

#define VIDEO_CODEC_ID AV_CODEC_ID_HEVC
#define VIDEO_PIX_FMT AV_PIX_FMT_YUV420P
#define VIDEO_CRF 28

/**
 * Main struct used to encode and mux videos
*/
typedef struct VideoCtx {
    AVFormatContext *mux_ctx;

    // video properties
    AVCodecContext *video_ctx;
    AVStream *video_stream;
    AVFrame *video_frame;
    int64_t video_pts;
    struct SwsContext *sws_ctx;

    // audio properties
    AVCodecContext *audio_ctx;
    AVStream *audio_stream;
    AVFrame *audio_frame;
    int64_t audio_pts;
    struct SwrContext *swr_ctx;
    uint16_t *src_audio_data;
} VideoCtx;

/**
 * Initialize the muxer
 * result: if not NULL the operation result
 * filename: The name of the file where save the video
 * w, h, framerate: Width, Height and video framerate
 * pix_fmt_src: Source pixel format
 * metadata: Muxer metadata
 * Returns the video context with result = 0, or NULL if error with result != 0
 */
extern VideoCtx *video_ctx_new(int *result, char *filename, int w, int h, int framerate, enum AVPixelFormat pix_fmt_src, AVDictionary *metadata);

/**
 * Send a NULL frame to video_send_frame and free ctx
 */
extern void video_ctx_free(VideoCtx *ctx);

/**
 * Send a frame to encoder, NULL to stop and save
 */
extern int video_send_frame(VideoCtx *ctx, const uint8_t *data, int stride);

#endif /* VIDEO_H */

