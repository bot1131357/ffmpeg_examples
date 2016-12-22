/** Example writing to video file
* best up-to-date implementation of video encoding so far 
* For some reason, webm files are a little shorter than the full length. Oh the mysteries of life. 	
*/

#include <algorithm>
#include <iostream>

 // FFmpeg C library in C++ programs requires extern "C" in order to inhibit the name mangling that C++ symbols gothrough
extern "C"
{
// #include <libavutil/opt.h>
#include <libavCodec/avCodec.h>
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

	AVCodec *Codec = NULL;
	AVCodecParameters* Codecpar = NULL;
	AVCodecContext *CodecCtx = NULL;
	AVFormatContext *FormatCtx = NULL;
	AVOutputFormat *OutputFormat = NULL;
	AVStream * VideoStream = NULL;;
	AVFrame *ImageFrame = NULL;;
	
	// char filename[] = "tmp/delme.webm";
	// char filename[] = "tmp/delme.avi";
	char filename[] = "tmp/delme.mov";

	// Setting up AVFormatContext's oformat by guessing on the filename
	OutputFormat = av_guess_format(NULL, filename, NULL);
	if (NULL == OutputFormat) {
		cerr << "Could not guess output format" << endl;
		return -1;
	}
	FormatCtx = avformat_alloc_context();
	FormatCtx->oformat = OutputFormat;
	memcpy(FormatCtx->filename, filename,
		min(strlen(filename), sizeof(FormatCtx->filename)));

	// Add a video stream to FormatCtx
	VideoStream = avformat_new_stream(FormatCtx, 0);
	if (!VideoStream)
	{
		printf("Cannot add new vidoe stream\n");
		return -1;
	}

	VideoStream->time_base.den = FRAMERATE;
	VideoStream->time_base.num = 1;

	// Allocate and set Codec context
	CodecCtx = avCodec_alloc_context3(Codec);
	CodecCtx->time_base.den = FRAMERATE;
	CodecCtx->time_base.num = 1;
	CodecCtx->Codec_type = AVMEDIA_TYPE_VIDEO;
	CodecCtx->bit_rate = 300000;
	CodecCtx->width = 320;
	CodecCtx->height = 240;
	CodecCtx->pix_fmt = AV_PIX_FMT_YUV420P;

	// Needed in FormatCtx. Not working otherwise.
	Codecpar = VideoStream->Codecpar;
	Codecpar->Codec_id = (AVCodecID)OutputFormat->video_Codec;
	Codecpar->Codec_type = AVMEDIA_TYPE_VIDEO;
	Codecpar->bit_rate = 300000;
	Codecpar->width = 320;
	Codecpar->height = 240;
	Codecpar->format = AV_PIX_FMT_YUV420P;

	/** 
	* Find and open the Codec.
	*/
	Codec = avCodec_find_encoder((AVCodecID)OutputFormat->video_Codec);
	if (Codec == NULL) {
		fprintf(stderr, "Codec not found\n");
		return -1;
	}
	
	if (avCodec_open2(CodecCtx, Codec, NULL) < 0)
	{
		printf("Cannot open video Codec\n");
		return -1;
	}

	// Open output file
	if (avio_open(&FormatCtx->pb, filename, AVIO_FLAG_WRITE) < 0)
	{
		printf("Cannot open file\n");
		return -1;
	}

	// Write file header.
	ret = avformat_write_header(FormatCtx, NULL);

	// Create frame
	ImageFrame = av_frame_alloc();
	ImageFrame->format = CodecCtx->pix_fmt;
	ImageFrame->width = CodecCtx->width;
	ImageFrame->height = CodecCtx->height;

	// What's the size of a frame?
	int bufferImgSize = av_image_get_buffer_size(CodecCtx->pix_fmt, CodecCtx->width,
		CodecCtx->height, 1);
	cout << "Size of a single frame: " << bufferImgSize << endl;

	// Allocate memory for the frame
	av_image_alloc(ImageFrame->data, ImageFrame->linesize, CodecCtx->width, CodecCtx->height, CodecCtx->pix_fmt, 32);

	// av_frame_get_buffer(ImageFrame, 32);

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
		for (y = 0; y<CodecCtx->height; y++) {
			for (x = 0; x<CodecCtx->width; x++) {
				ImageFrame->data[0][y * ImageFrame->linesize[0] + x] = x + y + i * 3;
			}
		}

		/* U and V */
		for (y = 0; y<CodecCtx->height / 2; y++) {
			for (x = 0; x<CodecCtx->width / 2; x++) {
				ImageFrame->data[1][y * ImageFrame->linesize[1] + x] = 128 + y + i * 2;
				ImageFrame->data[2][y * ImageFrame->linesize[2] + x] = 64 + x + i * 5;
			}
		}
		ImageFrame->pts = i;

		 
		// Encoder input: Supply a raw (uncompressed) frame to the encoder  		
		if (0 == avCodec_send_frame(CodecCtx, ImageFrame))
		{
			// Encoder output: Receive the packet from the encoder 
			if (0 == avCodec_receive_packet(CodecCtx, &avpkt)) // Packet available
			{
				/** 
				* Time stamps! Tricky little bastards. Still figuring it out myself.
				* DTS - Decoding Time Stamp
				* PTS - Presentation Time Stamp
				* DTS and PTS are usually the same unless we have B frames. This is 
				* because B-frames depend on information of frames before and after 
				* them and maybe be decoded in a sequence different from PTS.
				* 
				* av_rescale_q_rnd(a,bq,cq,AVRounding) supposedly gives a*(bq/cq)
				*
				*/
				avpkt.dts = av_rescale_q_rnd(avpkt.dts,
					CodecCtx->time_base,
					FormatCtx->streams[0]->time_base,
					(AVRounding)(AV_ROUND_NEAR_INF | AV_ROUND_PASS_MINMAX));
				avpkt.pts = av_rescale_q_rnd(avpkt.pts,
					CodecCtx->time_base,
					FormatCtx->streams[0]->time_base,
					(AVRounding)(AV_ROUND_NEAR_INF | AV_ROUND_PASS_MINMAX));
				avpkt.duration = av_rescale_q(1,
					CodecCtx->time_base,
					FormatCtx->streams[0]->time_base);

				// cout << "avpkt.dts: " << avpkt.dts << "\t"
				// << "avpkt.pts: " << avpkt.pts << "\t"
				// << "avpkt.duration: " << avpkt.duration << "\n";

				// If all goes well, we write the packet to output
				if (0 == av_interleaved_write_frame(FormatCtx, &avpkt))
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
	* We will check continuously until avCodec_receive_packet() returns AVERROR_EOF
	*/
	while (1)
	{
		av_init_packet(&avpkt);
		err = avCodec_send_frame(CodecCtx, NULL);
		if (0 == err) // Packet available
		{
			if (0 == avCodec_receive_packet(CodecCtx, &avpkt))
			{
				avpkt.dts = av_rescale_q_rnd(avpkt.dts,
					CodecCtx->time_base,
					FormatCtx->streams[0]->time_base,
					(AVRounding)(AV_ROUND_NEAR_INF | AV_ROUND_PASS_MINMAX));
				avpkt.pts = av_rescale_q_rnd(avpkt.pts,
					CodecCtx->time_base,
					FormatCtx->streams[0]->time_base,
					(AVRounding)(AV_ROUND_NEAR_INF | AV_ROUND_PASS_MINMAX));
				avpkt.duration = av_rescale_q(1,
					CodecCtx->time_base,
					FormatCtx->streams[0]->time_base);

				// cout << "avpkt.dts: " << avpkt.dts << "\t"
				// << "avpkt.pts: " << avpkt.pts << "\t"
				// << "avpkt.duration: " << avpkt.duration << "\n";

				if (0 == av_interleaved_write_frame(FormatCtx, &avpkt))
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
	av_write_trailer(FormatCtx);
	avCodec_close(CodecCtx);
	avio_close(FormatCtx->pb);
	av_free(CodecCtx);
	avformat_free_context(FormatCtx);
	av_freep(&ImageFrame->data[0]);
	av_frame_free(&ImageFrame);
		

    return 0;
}

