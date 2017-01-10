/*
 * decode_filter.h
 *
 *  Created on: 21-Dec-2016
 *      Author: rkrishna
 */

#ifndef DECODE_FILTER_H_
#define DECODE_FILTER_H_


/*
 * FFMPEG Decoder and Filter Class
 * Read Input file
 * Take StartTime OR BytePosition
 * Processing FrameRate
 * Processing Duration
 */

#ifdef __cplusplus
extern "C" {
#endif
#include "libavformat/avformat.h"
#include "libavcodec/avcodec.h"
#include "libavutil/avutil.h"
#include "libswscale/swscale.h"
#include "libavfilter/avfilter.h"
#include <libavfilter/buffersink.h>
#include <libavfilter/buffersrc.h>
#include <libavutil/opt.h>
#include <libswscale/swscale.h>
#ifdef __cplusplus
}
#endif
#include <iostream>

class FFMPEG_Decode_Filter{

public:
	FFMPEG_Decode_Filter(std::string video_file_name, int64_t byte_pos, int start_sec, int duration);
	~FFMPEG_Decode_Filter();
	int getNextFrame(AVFrame *dec_frame, float *frame_time);
	AVFormatContext * getFormatCtx(){
		return(format_ctx);
	}
	int getVideoStream(){
		return(video_stream_index);
	}
	int getVideoFramerate(){
		return(n_frame_rate);
	}

private:
	std::string video_file_path;
	int64_t byte_pos;
	int start_sec;
	int duration;
	int64_t frame_number;
	int n_frame_rate;
	bool flush_decode_pipe;
	int video_width, video_height;
	enum AVPixelFormat pix_fmt;
	bool done_decode, filter_err;
	/*
	 * FFMPEG variables
	 */
	AVFormatContext *format_ctx;
	int video_stream_index;
	int seek_ok;
	AVFrame decoded_picture;
	bool dframe;
	int ffmpegDecodeFromPacket(void);
	int CalcFrameRate(AVFormatContext *, int );

	/*
	 * Filter variables
	 */
    AVFilterGraph *filter_graph;
    AVFilterContext *buffersrc_ctx;
    AVFilterContext *deinterlace_ctx;
    AVFilterContext *buffersink_ctx;
	int Init_Deinterlace_filter(void);
	int DeinterlaceImage(void);
	void cleanfilter(void);
};





#endif /* DECODE_FILTER_H_ */
