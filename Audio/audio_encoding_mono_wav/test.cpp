
#include <algorithm>
#include <iostream>

// FFmpeg C library in C++ programs requires extern "C" in order to inhibit the name mangling that C++ symbols gothrough
extern "C"
{
	// #include <libavutil/opt.h>
#include <libavcodec/avcodec.h>
	// #include <libavutil/channel_layout.h>
	// #include <libavutil/common.h>
#include <libavutil/imgutils.h>
	// #include <libavutil/mathematics.h>
#include <libavutil/samplefmt.h>
#include <libavformat/avformat.h>
}

#define ERRBUFFLEN 200
char errbuf[ERRBUFFLEN];
inline const char* err2str(int ret)
{
	av_strerror(ret, errbuf, ERRBUFFLEN);
	return errbuf	;
} 

using namespace std;

#define CH_LAYOUT AV_CH_LAYOUT_MONO
#define SAMPLE_FMT AV_SAMPLE_FMT_S16
#define CODEC_ID AV_CODEC_ID_PCM_S16LE
#define CHANNELS 1
#define SAMPLERATE 44100
char filename[] = "tmp/delme.wav";	

int main(int argc, char** argv)
{
	int i, j, x, y, ret;

	av_register_all();

	AVCodec *Codec = NULL;
	AVCodecParameters* Codecpar = NULL;
	AVCodecContext *CodecCtx = NULL;
	AVFormatContext *FormatCtx = NULL;
	AVOutputFormat *OutputFormat = NULL;
	AVStream * AudioStream = NULL;;
	AVFrame *AudioFrame = NULL;


	ret = avformat_alloc_output_context2(&FormatCtx, NULL, NULL, filename);
	if (!FormatCtx || ret < 0)
	{
		fprintf(stderr, "Could not allocate output context: %s\n",err2str(ret));
		exit(1);
	}

	Codec = avcodec_find_encoder(FormatCtx->oformat->audio_codec);
	if (!Codec) {
		fprintf(stderr, "Codec not found\n");
		exit(1);
	}

	AudioStream = avformat_new_stream(FormatCtx, 0);
	if (!AudioStream)
	{
		fprintf(stderr, "Cannot add new audio stream\n");
		exit(1);
	}

	// Needed in pFormatCtx. Not working otherwise.
	Codecpar = AudioStream->codecpar;
	Codecpar->channel_layout = CH_LAYOUT;
	Codecpar->codec_id = CODEC_ID;
	Codecpar->channels = CHANNELS;
	Codecpar->format= SAMPLE_FMT;
	Codecpar->sample_rate = SAMPLERATE;
	Codecpar->codec_type = AVMEDIA_TYPE_AUDIO;

	CodecCtx = avcodec_alloc_context3(Codec);
	ret = avcodec_parameters_to_context(CodecCtx, Codecpar);

	// Check sample format is supported
	const enum AVSampleFormat *p = Codec->sample_fmts;
	bool samplefmt_not_supported = true;
	while (*p != AV_SAMPLE_FMT_NONE) {
		if (*p == CodecCtx->sample_fmt)
		{
			samplefmt_not_supported = false;
			break;
		}
		p++;
	}
	if (samplefmt_not_supported)
	{
		fprintf(stderr, "Sample format %d is not supported for this encoder.\n", CodecCtx->sample_fmt);
		exit(1);
	}

	/* open codec */
	if (avcodec_open2(CodecCtx, Codec, NULL) < 0) {
		fprintf(stderr, "Could not open codec: %s\n",err2str(ret));
		exit(1);
	}

	// avcodec_open2 seems to overwrite frame_size to 0
	CodecCtx->frame_size = 512;

	av_dump_format(FormatCtx, 0, filename, 1);

	ret = avio_open2(&FormatCtx->pb, filename, AVIO_FLAG_WRITE, NULL, NULL);
	ret = avformat_write_header(FormatCtx, NULL);
	if (ret <0)
	{
		printf("ret = %d: %s\n", ret, err2str(ret));
		exit(1);
	}

	/* frame containing input audio */
	AudioFrame = av_frame_alloc();
	if (!AudioFrame) {
		fprintf(stderr, "Could not allocate audio frame: %s\n",err2str(ret));
		exit(1);
	}

	AudioFrame->nb_samples = CodecCtx->frame_size;
	AudioFrame->format = CodecCtx->sample_fmt;
	AudioFrame->channel_layout = CodecCtx->channel_layout;

	/* the codec gives us the frame size, in samples,
	* we calculate the size of the samples buffer in bytes */
	int buffer_size = av_samples_get_buffer_size(NULL, CodecCtx->channels, CodecCtx->frame_size,
		CodecCtx->sample_fmt, 0);
	if (buffer_size < 0) {
		fprintf(stderr, "Could not get sample buffer size\n");
		exit(1);
	}

	/** Allocate memory for audio frame
	*	We could also use av_malloc, which requires av_freep at the end 
	*	avcodec_fill_audio_frame is useful if you want to encode directly 
	*	from an input buffer without making a copy
	*/
	av_frame_get_buffer(AudioFrame, 32);

	//samples = (uint16_t*)av_malloc(buffer_size);
	//if (!samples) {
	//	fprintf(stderr, "Could not allocate %d bytes for samples buffer\n",
	//		buffer_size);
	//	exit(1);
	//}
	///* setup the data pointers in the AVFrame */
	//ret = avcodec_fill_audio_frame(audio_frame, CodecCtx->channels, CodecCtx->sample_fmt,
	//	(const uint8_t*)samples, buffer_size, 0);
	//if (ret < 0) {
	//	fprintf(stderr, "Could not setup audio frame\n");
	//	exit(1);
	//}

	AVPacket avpkt;
	/** 
	* Using a while loop, we will encode 5 seconds of audio 
	* We generate a tone with the function y = 10000*sin(2*pi*440*t)
	*/

	int next_pts = 0;
	float theta = 0;
	uint16_t *samples = (uint16_t*)AudioFrame->data[0];
	float theta_incr = 2 * M_PI * 440.0 / CodecCtx->sample_rate;
	for (i = 0; i < 5* CodecCtx->sample_rate/ CodecCtx->frame_size; i++) {
		av_init_packet(&avpkt);

		for (j = 0; j < CodecCtx->frame_size; j++) {
			samples[j] = (uint16_t)(sin(theta) * 10000);
			theta += theta_incr;
		}

		// Sets time stamp
		next_pts += AudioFrame->nb_samples;
		AudioFrame->pts = next_pts;

		/* encode the samples */
		if (0 == avcodec_send_frame(CodecCtx, AudioFrame))
		{
			// Encoder output: Receive the packet from the encoder 
			if (0 == avcodec_receive_packet(CodecCtx, &avpkt)) // Packet available
			{
				// If all goes well, we write the packet to output
				if (0 == av_interleaved_write_frame(FormatCtx, &avpkt))
				{
					av_packet_unref(&avpkt);
				}
			}
		}
	}
	while (1)
	{
		av_init_packet(&avpkt);
		ret = avcodec_send_frame(CodecCtx, NULL);
		if (0 == ret) // Packet available
		{
			if (0 == avcodec_receive_packet(CodecCtx, &avpkt))
			{
				// Encoder output: Receive the packet from the encoder 
				if (0 == avcodec_receive_packet(CodecCtx, &avpkt)) // Packet available
				{
					// If all goes well, we write the packet to output
					if (0 == av_interleaved_write_frame(FormatCtx, &avpkt))
					{
						av_packet_unref(&avpkt);
					}
				}
			}
		}
		else if (AVERROR_EOF == ret) // No more packets
		{
			cout << "End of file reached\n";
			break;
		}
		else // How did it even get here?
		{
			cout << "Error\n";
		}

		av_packet_unref(&avpkt);
	}

	av_write_trailer(FormatCtx);
	avcodec_close(CodecCtx);
	avio_close(FormatCtx->pb);
	av_free(CodecCtx);
	avformat_free_context(FormatCtx);
	//av_freep(&audio_frame->data[0]);
	av_frame_free(&AudioFrame);


	return 0;
}