#include "video.h"
#include <math.h>
#include <libavutil/opt.h>
#include <libswscale/swscale.h>
#include <libswresample/swresample.h>

// private functions
static int close_all(VideoCtx *ctx);
static int video_stream_init(VideoCtx *ctx, int w, int h, int framerate, enum AVPixelFormat pix_fmt_src);
static int audio_stream_init(VideoCtx *ctx);
static int send_frame(VideoCtx *ctx, AVCodecContext *codec_ctx, AVStream *stream, AVFrame *frame);
static void fill_test_audio_data(VideoCtx *ctx);

VideoCtx *video_ctx_new(int *result, char *filename, int w, int h, int framerate, enum AVPixelFormat pix_fmt_src, AVDictionary *metadata) {
	VideoCtx *ctx = malloc(sizeof(VideoCtx));
	AVFormatContext *mux_ctx = NULL;

#ifndef DEBUG
	av_log_set_callback(NULL); // hide debug output
#endif

	int ret = avformat_alloc_output_context2(&mux_ctx, NULL, NULL, filename);
	if (!mux_ctx) {
		free(ctx);
		if (result)
			*result = ret;
		return NULL;
	}
	mux_ctx->metadata = metadata;


	ctx->mux_ctx = mux_ctx;
	ret = video_stream_init(ctx, w, h, framerate, pix_fmt_src);
	if (ret < 0) {
		free(ctx);
		free(mux_ctx);
		if (result)
			*result = ret;
		return NULL;
	}

	ret = audio_stream_init(ctx);
	if (ret < 0) {
		free(ctx);
		free(mux_ctx);
		if (result)
			*result = ret;
		return NULL;
	}

#ifdef DEBUG
	av_dump_format(mux_ctx, 0, filename, 1); // info about streams, for debugging
#endif

	const AVOutputFormat *fmt = mux_ctx->oformat;
	if (!(fmt->flags & AVFMT_NOFILE)) {
		ret = avio_open(&mux_ctx->pb, filename, AVIO_FLAG_WRITE);
		if (ret < 0) {
			free(ctx);
			free(mux_ctx);
			if (result)
				*result = ret;
			return NULL;
		}
	}

	AVDictionary *opt = NULL;
	ret = avformat_write_header(mux_ctx, &opt);
	av_dict_free(&opt);
	if (ret < 0) {
		if (result)
			*result = ret;
		return NULL;
	}

	if (result)
		*result = 0;
	return ctx;
}

void video_ctx_free(VideoCtx *ctx) {
	int res = video_send_frame(ctx, NULL, -1);
	if (res) {
		free(ctx);
		exit(res);
	}
	
	free(ctx);
}

int video_send_frame(VideoCtx *ctx, const uint8_t *data, int stride) {
	int ret;
	if (!ctx || (data && stride < 1))
		return -1;

	AVCodecContext *video_ctx = ctx->video_ctx, *audio_ctx = ctx->audio_ctx;
	AVFrame *frame;

	if (data) {
		while (av_compare_ts(ctx->audio_pts, audio_ctx->time_base,
				ctx->video_pts, video_ctx->time_base) < 0) {			
			// Write audio frame while muxer requires audio frames instead of video frames

			fill_test_audio_data(ctx); // TODO: read from audio file

			frame = ctx->audio_frame;
			if (av_frame_make_writable(frame) < 0)
				return -1;

			ret = swr_convert(ctx->swr_ctx, frame->data, frame->nb_samples,
					(const uint8_t **)&ctx->src_audio_data, frame->nb_samples);
			if (ret < 0) return ret;

			// increment the presentation timestamp by the number of audio samples (per channel)
			frame->pts = ctx->audio_pts;
			ctx->audio_pts += frame->nb_samples;

			ret = send_frame(ctx, audio_ctx, ctx->audio_stream, frame);
			if (ret == -1) return ret;
		}

		frame = ctx->video_frame;
		if (av_frame_make_writable(frame) < 0)
			return -1;

		ret = sws_scale(ctx->sws_ctx, &data, &stride, 0, ctx->video_ctx->height,
				frame->data, frame->linesize);
		if (ret < 0) return ret;

		frame->pts = ctx->video_pts++;
	} else {
		// flush audio frames
		ret = send_frame(ctx, audio_ctx, ctx->audio_stream, NULL);
		if (ret != AVERROR_EOF)
			return ret; // should be EOF, otherwise it is an error
		frame = NULL;
	}

	// if data send_frame otherwise flush video frames
	ret = send_frame(ctx, video_ctx, ctx->video_stream, frame);
	if (ret == -1) return -1;

	if (ret == AVERROR_EOF) {
		close_all(ctx);
		return 0;
	}

	return ctx->video_pts;
}

//      Private functions

int close_all(VideoCtx *ctx) {
	int ret = av_write_trailer(ctx->mux_ctx);
	if (ret < 0) {
		return ret;
	}
	AVFormatContext *mux_ctx = ctx->mux_ctx;

	// close streams
	avcodec_free_context(&ctx->video_ctx);
	avcodec_free_context(&ctx->audio_ctx);
	av_frame_free(&ctx->video_frame);
	av_frame_free(&ctx->audio_frame);
	sws_freeContext(ctx->sws_ctx);
	swr_free(&ctx->swr_ctx);

	// close I/O
	if (!(mux_ctx->oformat->flags & AVFMT_NOFILE))
		avio_closep(&mux_ctx->pb);

	// close muxer
	avformat_free_context(mux_ctx);
	return 0;
}

int video_stream_init(VideoCtx *ctx, int width, int height, int framerate, enum AVPixelFormat pix_fmt_src) {
	AVFormatContext *mux_ctx = ctx->mux_ctx;
	ctx->video_pts = 0;

	// init stream
	AVCodecContext *video_ctx;
	const AVCodec *codec = avcodec_find_encoder(VIDEO_CODEC_ID);
	if (!codec) {
		return -1;
	}

	AVStream *video_stream = avformat_new_stream(mux_ctx, NULL);

	if (!video_stream) {
		return -1;
	}
	ctx->video_stream = video_stream;

	video_ctx = avcodec_alloc_context3(codec);

	if (!video_ctx) {
		return -1;
	}

	video_ctx->width = width;
	video_ctx->height = height;
	video_ctx->time_base.num = 1;
	video_ctx->time_base.den = framerate;
	
	/*if (av_opt_set(video_ctx->priv_data, "crf", VIDEO_CRF, 0)) {
		return -1;
	}*/

	// pixel format of video codec: chroma 4:2:0
	video_ctx->pix_fmt = VIDEO_PIX_FMT;

#ifdef DEBUG
	const enum AVPixelFormat *tmp = codec->pix_fmts;
	fprintf(stderr, " [DD] Pixel formats:");
	while (*tmp != AV_PIX_FMT_NONE) {
		fprintf(stderr, " %d", *tmp++);
	}
	fprintf(stderr, "\n");
#endif

	// keyframe every quarter second
	video_ctx->gop_size = framerate/4;

	// global header compatibility
	if (mux_ctx->oformat->flags & AVFMT_GLOBALHEADER)
		video_ctx->flags |= AV_CODEC_FLAG_GLOBAL_HEADER;
	ctx->video_ctx = video_ctx;

	// open stream
	int ret;
	AVDictionary *opt = NULL;
	av_dict_set_int(&opt, "crf", VIDEO_CRF, 0);
	ret = avcodec_open2(video_ctx, codec, &opt);
	av_dict_free(&opt);
	if (ret < 0) {
		return ret;
	}

	ret = avcodec_parameters_from_context(video_stream->codecpar, video_ctx);
	if (ret < 0) {
		return ret;
	}

	AVFrame *frame = av_frame_alloc();
	if (!frame) {
		return -1;
	}
	frame->format = video_ctx->pix_fmt;
	frame->width = width;
	frame->height = height;

	ret = av_frame_get_buffer(frame, 0);
	if (ret < 0) {
		return ret;
	}

	ctx->video_frame = frame;

	// crete swscale context
	ctx->sws_ctx = sws_getContext(width, height, pix_fmt_src,
			width, height, video_ctx->pix_fmt, 0, 0, 0, 0);

	return 0;
}

int audio_stream_init(VideoCtx *ctx) {
	AVFormatContext *mux_ctx = ctx->mux_ctx;
	ctx->audio_pts = 0;

	// init stream
	AVCodecContext *audio_ctx;
	const AVCodec *codec = avcodec_find_encoder(AV_CODEC_ID_AAC);
	if (!codec) {
		return -1;
	}

	AVStream *audio_stream = avformat_new_stream(mux_ctx, NULL);
	if (!audio_stream) {
		return -1;
	}

	ctx->audio_stream = audio_stream;
	audio_ctx = avcodec_alloc_context3(codec);
	if (!audio_ctx) {
		return -1;
	}

	audio_ctx->sample_fmt = codec->sample_fmts ? *codec->sample_fmts : AV_SAMPLE_FMT_FLTP;

#ifdef DEBUG
	const enum AVSampleFormat *tmp = codec->sample_fmts;
	fprintf(stderr, " [DD] Sample formats:");
	while (*tmp != AV_SAMPLE_FMT_NONE) {
		fprintf(stderr, " %d", *tmp++);
	}
	fprintf(stderr, "\n");

	const int *tmp2 = codec->supported_samplerates;
	fprintf(stderr, " [DD] Sample rates:");
	while (*tmp2) {
		fprintf(stderr, " %d", *tmp2++);
	}
	fprintf(stderr, "\n");
#endif

	audio_ctx->sample_rate = 44100;
	audio_ctx->ch_layout = (AVChannelLayout)AV_CHANNEL_LAYOUT_STEREO;
	audio_stream->time_base = (AVRational){1, audio_ctx->sample_rate};

	// global header compatibility
	if (mux_ctx->oformat->flags & AVFMT_GLOBALHEADER)
		audio_ctx->flags |= AV_CODEC_FLAG_GLOBAL_HEADER;
	ctx->audio_ctx = audio_ctx;
	
	// open stream
	int ret;
	AVDictionary *opt = NULL;
	ret = avcodec_open2(audio_ctx, codec, &opt);
	av_dict_free(&opt);
	if (ret < 0) {
		return ret;
	}

	ret = avcodec_parameters_from_context(audio_stream->codecpar, audio_ctx);
	if (ret < 0) {
		return ret;
	}

	AVFrame *frame = av_frame_alloc();
	if (!frame) {
		return -1;
	}
	frame->format = audio_ctx->sample_fmt;
	frame->ch_layout = audio_ctx->ch_layout;
	frame->sample_rate = audio_ctx->sample_rate;
	frame->nb_samples = audio_ctx->frame_size;

	ret = av_frame_get_buffer(frame, 0);
	if (ret < 0) {
		return ret;
	}

	ctx->audio_frame = frame;

	// create resample context
	SwrContext *swr_ctx = swr_alloc();
	if (!swr_ctx) {
		fprintf(stderr, " [EE] Could not allocate resample context\n");
		return -1;
	}

	// set options
	av_opt_set_chlayout(swr_ctx, "in_chlayout", &audio_ctx->ch_layout, 0);
	av_opt_set_int(swr_ctx, "in_sample_rate", audio_ctx->sample_rate, 0);
	av_opt_set_sample_fmt(swr_ctx, "in_sample_fmt", AV_SAMPLE_FMT_S16, 0);
	
	av_opt_set_chlayout(swr_ctx, "out_chlayout", &audio_ctx->ch_layout, 0);
	av_opt_set_int(swr_ctx, "out_sample_rate", audio_ctx->sample_rate, 0);
	av_opt_set_sample_fmt(swr_ctx, "out_sample_fmt", audio_ctx->sample_fmt, 0);

	// initialize sw resample context
	if (swr_init(swr_ctx) < 0) {
		fprintf(stderr, " [EE] Failed to initialize the SW resample context\n");
		return -1;
	}
	ctx->swr_ctx = swr_ctx;
	ctx->src_audio_data = malloc(sizeof(int16_t) * audio_ctx->frame_size * audio_ctx->ch_layout.nb_channels);
	
	return 0;
}

int send_frame(VideoCtx *ctx, AVCodecContext *codec_ctx, AVStream *stream, AVFrame *frame) {
	// send the frame to the encoder
	int ret = avcodec_send_frame(codec_ctx, frame);
	if (ret < 0) {
		return -1;
	}

	AVPacket pkt = {0};
	while (1) {
		ret = avcodec_receive_packet(codec_ctx, &pkt);
		if (ret == AVERROR(EAGAIN) || ret == AVERROR_EOF)
			break;
		if (ret < 0) {
			return -1;
		}

		av_packet_rescale_ts(&pkt, codec_ctx->time_base, stream->time_base);
		pkt.stream_index = stream->index;

		ret = av_interleaved_write_frame(ctx->mux_ctx, &pkt);
		av_packet_unref(&pkt);
		if (ret < 0) {
			return -1;
		}
	}
	
	return ret;
}

void fill_test_audio_data(VideoCtx *ctx) {
	uint16_t *data = ctx->src_audio_data, tmp_value;

	AVCodecContext *audio_ctx = ctx->audio_ctx;
	int frame_size = audio_ctx->frame_size;
	int sample_rate = audio_ctx->sample_rate;
	int channels = audio_ctx->ch_layout.nb_channels;
	
	double freq = 200.0;
	double volume = 0.0;
	double const_factor_inside = 2.0*M_PI*freq;
	double const_factor_outside = SHRT_MAX * volume;

	for (int i = 0; i < frame_size; i++) {
		double relative_time = (i+ctx->audio_pts) % sample_rate / (double)sample_rate;
		tmp_value = sin(const_factor_inside * relative_time) * const_factor_outside;
		for (int c = 0; c < channels; c++)
			*data++ = tmp_value;
	}
}
