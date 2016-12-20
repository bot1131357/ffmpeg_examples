/** Example writing to video file
* best up-to-date implementation of video encoding so far 
* For some reason, webm files are a little shorter than the full length. Oh the mysteries of life. 	
*/
// #include <stdlib.h>
// #include <stdio.h>
// #include <string.h>
// #include <ctime>
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
// #include <libavutil/samplefmt.h>
#include <libavformat/avformat.h>
}


using namespace std;

#define FRAMERATE 30

int main(int argc, char** argv)
{
	int i, x, y, ret;
	av_register_all();

	AVCodec *codec = NULL;
	AVCodecParameters* codecpar = NULL;
	AVCodecContext *codecCtx = NULL;
	AVFormatContext *pFormatCtx = NULL;
	AVOutputFormat *pOutFormat = NULL;
	AVStream * pVideoStream = NULL;;
	AVFrame *picture = NULL;;
	
	// char filename[] = "tmp/delme.webm";
	// char filename[] = "tmp/delme.avi";
	char filename[] = "tmp/delme.mov";

	// Setting up AVFormatContext's oformat by guessing on the filename
	pOutFormat = av_guess_format(NULL, filename, NULL);
	if (NULL == pOutFormat) {
		cerr << "Could not guess output format" << endl;
		return -1;
	}
	pFormatCtx = avformat_alloc_context();
	pFormatCtx->oformat = pOutFormat;
	memcpy(pFormatCtx->filename, filename,
		min(strlen(filename), sizeof(pFormatCtx->filename)));

	// Add a video stream to pFormatCtx
	pVideoStream = avformat_new_stream(pFormatCtx, 0);
	if (!pVideoStream)
	{
		printf("Cannot add new vidoe stream\n");
		return -1;
	}

	pVideoStream->time_base.den = FRAMERATE;
	pVideoStream->time_base.num = 1;

	// Allocate and set codec context
	codecCtx = avcodec_alloc_context3(codec);
	codecCtx->time_base.den = FRAMERATE;
	codecCtx->time_base.num = 1;
	codecCtx->codec_type = AVMEDIA_TYPE_VIDEO;
	codecCtx->bit_rate = 300000;
	codecCtx->width = 320;
	codecCtx->height = 240;
	codecCtx->pix_fmt = AV_PIX_FMT_YUV420P;

	// Needed in pFormatCtx. Not working otherwise.
	codecpar = pVideoStream->codecpar;
	codecpar->codec_id = (AVCodecID)pOutFormat->video_codec;
	codecpar->codec_type = AVMEDIA_TYPE_VIDEO;
	codecpar->bit_rate = 300000;
	codecpar->width = 320;
	codecpar->height = 240;
	codecpar->format = AV_PIX_FMT_YUV420P;

	/** 
	* Find and open the codec.
	*/
	codec = avcodec_find_encoder((AVCodecID)pOutFormat->video_codec);
	if (codec == NULL) {
		fprintf(stderr, "Codec not found\n");
		return -1;
	}
	
	if (avcodec_open2(codecCtx, codec, NULL) < 0)
	{
		printf("Cannot open video codec\n");
		return -1;
	}

	// Open output file
	if (avio_open(&pFormatCtx->pb, filename, AVIO_FLAG_WRITE) < 0)
	{
		printf("Cannot open file\n");
		return -1;
	}

	// Write file header.
	ret = avformat_write_header(pFormatCtx, NULL);

	// Create frame
	picture = av_frame_alloc();
	picture->format = codecCtx->pix_fmt;
	picture->width = codecCtx->width;
	picture->height = codecCtx->height;

	// What's the size of a frame?
	int bufferImgSize = av_image_get_buffer_size(codecCtx->pix_fmt, codecCtx->width,
		codecCtx->height, 1);
	cout << "Size of a single frame: " << bufferImgSize << endl;

	// Allocate memory for the frame
	av_image_alloc(picture->data, picture->linesize, codecCtx->width, codecCtx->height, codecCtx->pix_fmt, 32);

	AVPacket avpkt;

	/** 
	* Using a while loop, we will encode 10 seconds of video 
	*/
	for (i = 0; i<10*FRAMERATE; i++) {
		av_init_packet(&avpkt);

		/** 
		* Prepare a dummy image 
		* We are using YUV 420P pixel format, which can be visualised as 3 matrix arrays Y, U and V. 
		* Y array has the same dimension as the frame, whereas U and V are only half the dimensions (therefore quarter of the area) 
		*/
		/* Y */
		for (y = 0; y<codecCtx->height; y++) {
			for (x = 0; x<codecCtx->width; x++) {
				picture->data[0][y * picture->linesize[0] + x] = x + y + i * 3;
			}
		}

		/* U and V */
		for (y = 0; y<codecCtx->height / 2; y++) {
			for (x = 0; x<codecCtx->width / 2; x++) {
				picture->data[1][y * picture->linesize[1] + x] = 128 + y + i * 2;
				picture->data[2][y * picture->linesize[2] + x] = 64 + x + i * 5;
			}
		}
		picture->pts = i;

		 
		// Encoder input: Supply a raw (uncompressed) frame to the encoder  		
		if (0 == avcodec_send_frame(codecCtx, picture))
		{
			// Encoder output: Receive the packet from the encoder 
			if (0 == avcodec_receive_packet(codecCtx, &avpkt)) // Packet available
			{
				/** 
				* Time stamps! Tricky little bastards. Still figuring it out myself.
				*/
				avpkt.dts = av_rescale_q_rnd(avpkt.dts,
					codecCtx->time_base,
					pFormatCtx->streams[0]->time_base,
					(AVRounding)(AV_ROUND_NEAR_INF | AV_ROUND_PASS_MINMAX));
				avpkt.pts = av_rescale_q_rnd(avpkt.pts,
					codecCtx->time_base,
					pFormatCtx->streams[0]->time_base,
					(AVRounding)(AV_ROUND_NEAR_INF | AV_ROUND_PASS_MINMAX));
				avpkt.duration = av_rescale_q(1,
					codecCtx->time_base,
					pFormatCtx->streams[0]->time_base);

				// cout << "avpkt.dts: " << avpkt.dts << "\t"
				// << "avpkt.pts: " << avpkt.pts << "\t"
				// << "avpkt.duration: " << avpkt.duration << "\n";

				// If all goes well, we write the packet to output
				if (0 == av_interleaved_write_frame(pFormatCtx, &avpkt))
				{
					av_packet_unref(&avpkt);
				}
			}

		}		
		cout << ".";
	}
	int err = 0;
	/** 
	* Flush remaining data from the encoder after the previous loop
	* We will check continuously until avcodec_receive_packet() returns AVERROR_EOF
	*/
	while (1)
	{
		av_init_packet(&avpkt);
		err = avcodec_send_frame(codecCtx, NULL);
		if (0 == err) // Packet available
		{
			if (0 == avcodec_receive_packet(codecCtx, &avpkt))
			{
				avpkt.dts = av_rescale_q_rnd(avpkt.dts,
					codecCtx->time_base,
					pFormatCtx->streams[0]->time_base,
					(AVRounding)(AV_ROUND_NEAR_INF | AV_ROUND_PASS_MINMAX));
				avpkt.pts = av_rescale_q_rnd(avpkt.pts,
					codecCtx->time_base,
					pFormatCtx->streams[0]->time_base,
					(AVRounding)(AV_ROUND_NEAR_INF | AV_ROUND_PASS_MINMAX));
				avpkt.duration = av_rescale_q(1,
					codecCtx->time_base,
					pFormatCtx->streams[0]->time_base);

				// cout << "avpkt.dts: " << avpkt.dts << "\t"
				// << "avpkt.pts: " << avpkt.pts << "\t"
				// << "avpkt.duration: " << avpkt.duration << "\n";

				if (0 == av_interleaved_write_frame(pFormatCtx, &avpkt))
				{
					av_packet_unref(&avpkt);
				}
			}

		}
		else if (AVERROR_EOF == err) // No more packets
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

	// Remember to clean up
	av_write_trailer(pFormatCtx);
	avcodec_close(codecCtx);
	avio_close(pFormatCtx->pb);
	av_free(codecCtx);
	avformat_free_context(pFormatCtx);
	av_freep(&picture->data[0]);
	av_frame_free(&picture);
		

    return 0;
}

