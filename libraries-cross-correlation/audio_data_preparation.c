#include "audio_data_preparation.h"

#include "return_codes.h"
#include <libavcodec/avcodec.h>
#include <libavformat/avformat.h>
#include <libavutil/frame.h>
#include <libavutil/samplefmt.h>

#include <stdio.h>
#include <stdlib.h>

typedef struct
{
	AVFormatContext *format_context;
	AVCodecContext *codec_context;
	size_t audio_stream_index;
	double *raw_data1;
	double *raw_data2;
	size_t sample_rate;
	size_t data_length;
	size_t data2_length;
	size_t nb_channels;
	size_t channel1_index;
	size_t channel2_index;
} AudioProcessingContext;

#define CHECK_SUCCESS(expr)                                                                                            \
	do                                                                                                                 \
	{                                                                                                                  \
		int result = (expr);                                                                                           \
		if (result != SUCCESS)                                                                                         \
			return result;                                                                                             \
	} while (0)

static int open_audio_file(const char *path, AudioProcessingContext *audio_ctx)
{
	AVFormatContext *context = NULL;
	int output = avformat_open_input(&context, path, NULL, NULL);
	if (output < 0)
	{
		switch (output)
		{
		case AVERROR(EIO):
			fprintf(stderr, "Failed to open input file %s: I/O error\n", path);
			return ERROR_CANNOT_OPEN_FILE;
		case AVERROR(ENOENT):
			fprintf(stderr, "Failed to open input file %s: File not found\n", path);
			return ERROR_CANNOT_OPEN_FILE;
		case AVERROR(ENOMEM):
			fprintf(stderr, "Failed to open input file %s: Not enough memory\n", path);
			return ERROR_NOTENOUGH_MEMORY;
		case AVERROR_INVALIDDATA:
			fprintf(stderr, "Failed to open input file %s: Invalid data\n", path);
			return ERROR_DATA_INVALID;
		case AVERROR(EAGAIN):
			fprintf(stderr, "Failed to open input file %s: Resource temporarily unavailable\n", path);
			return ERROR_UNSUPPORTED;
		case AVERROR(ENOSYS):
			fprintf(stderr, "Failed to open input file %s: Unsupported functionality\n", path);
			return ERROR_UNSUPPORTED;
		default:
			fprintf(stderr, "Failed to open input file %s: Unknown error %d\n", path, output);
			avformat_free_context(context);
			return ERROR_UNKNOWN;
		}
	}

	if (avformat_find_stream_info(context, NULL) < 0)
	{
		fprintf(stderr, "Failed to find any stream info for file %s\n", path);
		goto clean_up;
	}

	audio_ctx->audio_stream_index = av_find_best_stream(context, AVMEDIA_TYPE_AUDIO, -1, -1, NULL, 0);

	if (audio_ctx->audio_stream_index < 0)
	{
		fprintf(stderr, "Failed to find any audio stream in file %s\n", path);
		goto clean_up;
	}

	AVCodecParameters *codec_parameters = context->streams[audio_ctx->audio_stream_index]->codecpar;
	enum AVCodecID codec_id = codec_parameters->codec_id;
	switch (codec_id)
	{
	case AV_CODEC_ID_FLAC:
	case AV_CODEC_ID_MP2:
	case AV_CODEC_ID_MP3:
	case AV_CODEC_ID_OPUS:
	case AV_CODEC_ID_AAC:
		break;
	default:
		fprintf(stderr, "Codec %s is unsupported in file %s\n", avcodec_get_name(codec_id), path);
		goto clean_up;
	}

	audio_ctx->format_context = context;
	return SUCCESS;

clean_up:
	if (context)
	{
		avformat_close_input(&context);
		avformat_free_context(context);
	}
	if (audio_ctx->format_context)
	{
		avformat_close_input(&audio_ctx->format_context);
		avformat_free_context(audio_ctx->format_context);
	}
	return ERROR_DATA_INVALID;
}

static int setup_audio_codec(AudioProcessingContext *audio_ctx)
{
	AVStream *audio_stream = audio_ctx->format_context->streams[audio_ctx->audio_stream_index];
	AVCodecParameters *codec_parameters = audio_stream->codecpar;
	const AVCodec *codec = avcodec_find_decoder(codec_parameters->codec_id);

	if (!codec)
	{
		avformat_close_input(&audio_ctx->format_context);
		fprintf(stderr, "Unsupported codec\n");
		return ERROR_DATA_INVALID;
	}

	audio_ctx->codec_context = avcodec_alloc_context3(codec);
	if (!audio_ctx->codec_context)
		goto clean_up;

	audio_ctx->codec_context->pkt_timebase = audio_ctx->format_context->streams[audio_ctx->audio_stream_index]->time_base;

	if (avcodec_parameters_to_context(audio_ctx->codec_context, codec_parameters) < 0)
	{
		fprintf(stderr, "Failed to set up codec context\n");
		goto clean_up;
	}

	if (avcodec_open2(audio_ctx->codec_context, codec, NULL) < 0)
	{
		fprintf(stderr, "Failed to open codec\n");
		goto clean_up;
	}

	audio_ctx->nb_channels = audio_ctx->codec_context->ch_layout.nb_channels;
	audio_ctx->sample_rate = audio_ctx->codec_context->sample_rate;
	return SUCCESS;

clean_up:
	if (audio_ctx->format_context)
	{
		avformat_close_input(&audio_ctx->format_context);
		avformat_free_context(audio_ctx->format_context);
	}
	return ERROR_UNKNOWN;
}

static int setup_channels(AudioProcessingContext *audio_ctx)
{
	audio_ctx->channel1_index = -1;
	audio_ctx->channel2_index = -1;

	for (size_t i = 0; i < audio_ctx->nb_channels; i++)
	{
		if (audio_ctx->channel1_index == -1)
		{
			audio_ctx->channel1_index = i;
		}
		else if (audio_ctx->channel2_index == -1)
		{
			audio_ctx->channel2_index = i;
		}
		else
		{
			break;
		}
	}

	if (audio_ctx->channel1_index == -1)
	{
		fprintf(stderr, "No channels available\n");
		avformat_close_input(&audio_ctx->format_context);
		avcodec_free_context(&audio_ctx->codec_context);
		return ERROR_DATA_INVALID;
	}
	return SUCCESS;
}

static int sample_reformat(AVFrame *frame, AudioProcessingContext *audio_ctx, size_t channel, size_t length, double **decoded_data)
{
	for (size_t i = 0; i < frame->nb_samples; i++)
	{
		switch (audio_ctx->codec_context->sample_fmt)
		{
		case AV_SAMPLE_FMT_FLTP:
		{
			(*decoded_data)[length + i] = (double)((float *)frame->data[channel])[i];
			break;
		}

		case AV_SAMPLE_FMT_FLT:
		{
			(*decoded_data)[length + i] = (double)((float *)frame->data[0])[i * audio_ctx->nb_channels + channel];
			break;
		}

		case AV_SAMPLE_FMT_S16P:
		{
			(*decoded_data)[length + i] = ((int16_t *)frame->data[channel])[i] / (double)INT16_MAX;
			break;
		}

		case AV_SAMPLE_FMT_S16:
		{
			(*decoded_data)[length + i] = ((int16_t *)frame->data[0])[i * audio_ctx->nb_channels + channel] / (double)INT16_MAX;
			break;
		}

		case AV_SAMPLE_FMT_S32P:
		{
			(*decoded_data)[length + i] = ((int32_t *)frame->data[channel])[i] / (double)INT32_MAX;
			break;
		}

		case AV_SAMPLE_FMT_S32:
		{
			(*decoded_data)[length + i] = ((int32_t *)frame->data[0])[i * audio_ctx->nb_channels + channel] / (double)INT32_MAX;
			break;
		}

		case AV_SAMPLE_FMT_DBLP:
		{
			(*decoded_data)[length + i] = ((double *)frame->data[channel])[i];
			break;
		}

		case AV_SAMPLE_FMT_DBL:
		{
			(*decoded_data)[length + i] = ((double *)frame->data[0])[i * audio_ctx->nb_channels + channel];
			break;
		}

		case AV_SAMPLE_FMT_U8P:
		{
			(*decoded_data)[length + i] = ((uint8_t *)frame->data[channel])[i] / (double)UINT8_MAX;
			break;
		}

		case AV_SAMPLE_FMT_U8:
		{
			(*decoded_data)[length + i] = ((uint8_t *)frame->data[0])[i * audio_ctx->nb_channels + channel] / (double)UINT8_MAX;
			break;
		}

		case AV_SAMPLE_FMT_S64P:
		{
			(*decoded_data)[length + i] = (double)((int64_t *)frame->data[channel])[i] / (double)INT64_MAX;
			break;
		}

		case AV_SAMPLE_FMT_S64:
		{
			(*decoded_data)[length + i] = (double)((int64_t *)frame->data[0])[i * audio_ctx->nb_channels + channel] / (double)INT64_MAX;
			break;
		}

		default:
			fprintf(stderr, "Unsupported sample format\n");
			free(*decoded_data);
			av_frame_unref(frame);
			return ERROR_DATA_INVALID;
		}
	}
	return SUCCESS;
}

static int process_frames(AudioProcessingContext *audio_ctx, AVFrame *frame, size_t *length, size_t *buffer_size, double **decoded_data, size_t channel)
{
	while (1)
	{
		int output = avcodec_receive_frame(audio_ctx->codec_context, frame);
		switch (output)
		{
		case 0:
			break;
		case AVERROR(EAGAIN):
		case AVERROR_EOF:
			return SUCCESS;
		default:
			fprintf(stderr, "Failed to receive frame from decoder\n");
			return ERROR_UNKNOWN;
		}

		size_t frame_sample_count = frame->nb_samples;
		size_t required_size = *length + frame_sample_count;

		if (required_size > *buffer_size)
		{
			*buffer_size *= 2;
			double *new_data = realloc(*decoded_data, *buffer_size * sizeof(double));
			if (!new_data)
			{
				fprintf(stderr, "Failed to reallocate memory for audio data\n");
				return ERROR_UNKNOWN;
			}
			*decoded_data = new_data;
		}

		int sample_result = sample_reformat(frame, audio_ctx, channel, *length, decoded_data);
		if (sample_result != SUCCESS)
		{
			return sample_result;
		}
		*length += frame_sample_count;
	}
}

static int handle_packet_error(int error_code)
{
	switch (error_code)
	{
	case AVERROR(EINVAL):
		fprintf(stderr, "Invalid parameters when sending packet\n");
		return ERROR_ARGUMENTS_INVALID;
	case AVERROR(ENOSYS):
		fprintf(stderr, "Operation not supported by codec\n");
		return ERROR_UNSUPPORTED;
	case AVERROR_INVALIDDATA:
		fprintf(stderr, "Corrupted data in packet\n");
		return ERROR_DATA_INVALID;
	default:
		fprintf(stderr, "Unknown error code %d\n", error_code);
		return ERROR_UNKNOWN;
	}
}

static int decode_audio_data(AudioProcessingContext *audio_ctx, size_t channel, size_t *data_length, double **decoded_data)
{
	AVFrame *frame = av_frame_alloc();
	AVPacket *packet = av_packet_alloc();
	*decoded_data = NULL;
	int decoding_output = SUCCESS;

	if (!packet || !frame)
	{
		fprintf(stderr, "Failed to allocate memory for packet or frame\n");
		decoding_output = ERROR_UNKNOWN;
		goto clean_up;
	}

	size_t buffer_size = 4096;
	*decoded_data = malloc(buffer_size * sizeof(double));
	if (!*decoded_data)
	{
		fprintf(stderr, "Failed to allocate memory for decoded data\n");
		decoding_output = ERROR_UNKNOWN;
		goto clean_up;
	}

	size_t length = 0;

	while (av_read_frame(audio_ctx->format_context, packet) >= 0)
	{
		if (packet->stream_index != audio_ctx->audio_stream_index)
		{
			av_packet_unref(packet);
			continue;
		}

		decoding_output = avcodec_send_packet(audio_ctx->codec_context, packet);
		if (decoding_output != SUCCESS)
		{
			decoding_output = handle_packet_error(decoding_output);
			goto clean_up;
		}

		decoding_output = process_frames(audio_ctx, frame, &length, &buffer_size, decoded_data, channel);
		if (decoding_output != SUCCESS)
		{
			goto clean_up;
		}
		av_packet_unref(packet);
	}

	av_packet_free(&packet);
	av_frame_free(&frame);
	*data_length = length;
	return SUCCESS;

clean_up:
	av_packet_unref(packet);
	av_frame_unref(frame);
	if (*decoded_data)
		free(*decoded_data);
	return decoding_output;
}

static void cleanup_audio_context(AudioProcessingContext *audio_ctx)
{
	if (audio_ctx->format_context)
	{
		avformat_close_input(&audio_ctx->format_context);
		avformat_free_context(audio_ctx->format_context);
	}
	if (audio_ctx->codec_context)
	{
		avcodec_free_context(&audio_ctx->codec_context);
	}
	if (audio_ctx->raw_data1)
	{
		free(audio_ctx->raw_data1);
	}
	if (audio_ctx->raw_data2)
	{
		free(audio_ctx->raw_data2);
	}
}

static int
	process_audio_file(const char *file_path, AudioProcessingContext *audio_ctx, double **channel1_data, double **channel2_data, size_t *data1_len, size_t *data2_len, uint8_t one_file_input)
{
	CHECK_SUCCESS(open_audio_file(file_path, audio_ctx));
	CHECK_SUCCESS(setup_audio_codec(audio_ctx));
	CHECK_SUCCESS(setup_channels(audio_ctx));

	if (one_file_input && audio_ctx->nb_channels != 2)
	{
		fprintf(stderr, "Audio file must have two channels\n");
		cleanup_audio_context(audio_ctx);
		return ERROR_FORMAT_INVALID;
	}
	else if (one_file_input && audio_ctx->nb_channels == 2)
	{
		CHECK_SUCCESS(decode_audio_data(audio_ctx, audio_ctx->channel2_index, data2_len, channel2_data));
	}

	CHECK_SUCCESS(decode_audio_data(audio_ctx, audio_ctx->channel1_index, data1_len, channel1_data));

	return SUCCESS;
}

int prepare_audio_data(int argc, char **argv, double **prepared_data1, double **prepared_data2, size_t *data1_len, size_t *data2_len, size_t *sample_rate)
{
	AudioProcessingContext audio_file1_ctx = {
		.format_context = NULL,
		.codec_context = NULL,
		.audio_stream_index = 0,
		.raw_data1 = NULL,
		.raw_data2 = NULL,
		.sample_rate = 0,
		.data_length = 0,
		.data2_length = 0,
		.nb_channels = 0,
		.channel1_index = 0,
		.channel2_index = 0
	};

	AudioProcessingContext audio_file2_ctx = audio_file1_ctx;
	int output = SUCCESS;

	if (argc == 2)
		CHECK_SUCCESS(process_audio_file(argv[1], &audio_file1_ctx, prepared_data1, prepared_data2, data1_len, data2_len, 1));

	else if (argc == 3)
	{
		CHECK_SUCCESS(process_audio_file(argv[1], &audio_file1_ctx, prepared_data1, NULL, data1_len, NULL, 0));
		CHECK_SUCCESS(process_audio_file(argv[2], &audio_file2_ctx, prepared_data2, NULL, data2_len, NULL, 0));

		if (audio_file1_ctx.sample_rate != audio_file2_ctx.sample_rate)
		{
			fprintf(stderr, "Sample rates do not match\n");
			output = ERROR_DATA_INVALID;
			goto clean_up;
		}
	}

	*sample_rate = audio_file1_ctx.sample_rate;
	goto clean_up;

clean_up:
	cleanup_audio_context(&audio_file1_ctx);
	cleanup_audio_context(&audio_file2_ctx);
	return output;
}
