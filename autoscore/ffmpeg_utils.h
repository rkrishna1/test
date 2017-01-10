/*
 * utils.cpp
 *
 *  Created on: 21-Dec-2016
 *      Author: rkrishna
 */


#include <iostream>
#include <fstream>
#include <string>
#include <map>

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

#include "opencv2/imgproc/imgproc.hpp"
#include "opencv2/highgui/highgui.hpp"
#include "opencv2/ml.hpp"


class ImageConvertCV{

public:
	ImageConvertCV(AVFormatContext *fmt_ctx, int video_st);
	int ffmpegimage_to_matimage(AVFrame *deinterlaced_frame, cv::Mat *ocv_image);
private:
	struct SwsContext *convert_ctx;
	int video_width, video_height;
	enum AVPixelFormat pix_fmt;
	int init_swscale();

};

ImageConvertCV::ImageConvertCV(AVFormatContext *fmt_ctx, int video_st):convert_ctx(NULL)
{

	video_width = fmt_ctx->streams[video_st]->codec->width;
	video_height = fmt_ctx->streams[video_st]->codec->height;
	pix_fmt = fmt_ctx->streams[video_st]->codec->pix_fmt;
	init_swscale();
}


int ImageConvertCV::init_swscale(){

	if(convert_ctx)
		sws_freeContext(convert_ctx);
	convert_ctx = sws_getContext(video_width, video_height,
				pix_fmt, video_width, video_height, AV_PIX_FMT_GRAY8, SWS_BICUBIC, NULL, NULL, NULL);
	if(!convert_ctx)
		return(-1);
	return(0);
}

int ImageConvertCV::ffmpegimage_to_matimage(AVFrame *deinterlaced_frame, cv::Mat *ocv_image){

	int ret;
	AVFrame *ccov_frame = av_frame_alloc();

	if(video_width != deinterlaced_frame->width || video_height != deinterlaced_frame->height || pix_fmt != deinterlaced_frame->format){
		video_width = deinterlaced_frame->width;
		video_height = deinterlaced_frame->height;
		pix_fmt = (enum AVPixelFormat) deinterlaced_frame->format;
		if(init_swscale() < 0){
			std::cout << "Cannot allocate Image Conversion...Unable to process";
			return(-1);
		}
	}
	ccov_frame->width = deinterlaced_frame->width;
	ccov_frame->height = deinterlaced_frame->height;
	ccov_frame->format = AV_PIX_FMT_GRAY8;
	ret = av_frame_get_buffer(ccov_frame, 1);
	if(ret < 0 || !ccov_frame)
		return(ret);
	sws_scale(convert_ctx, deinterlaced_frame->data,
			deinterlaced_frame->linesize, 0,
			deinterlaced_frame->height,
			ccov_frame->data, ccov_frame->linesize);
	ocv_image->create(ccov_frame->height, ccov_frame->width, CV_8UC1);
	std::memcpy(ocv_image->data, ccov_frame->data[0], ccov_frame->height * ccov_frame->width);
	av_frame_free(&ccov_frame);
	return(1);
}


//int ocr_text(cv::Mat original_img, Rect roi){
//	cv::Mat in_img, out_img;
//
//	if(CV_SHOW_IMAGE)
//			cv::imshow("TEST1", original_img);
//
//	in_img = original_img(roi);
//	cv::resize(in_img, out_img, cv::Size(0, 0), 4, 4, INTER_CUBIC);
//	if(CV_SHOW_IMAGE){
//		cv::imshow("TEST2", out_img);
//		cv::waitKey(5);
//	}
//	PIX *pixS = pixCreate(out_img.size().width, out_img.size().height, 8);
//	for(int p=0; p<out_img.rows; p++)
//		for(int j=0; j<out_img.cols; j++)
//			pixSetPixel(pixS, j,p, (l_uint32) out_img.at<uchar>(p,j));
//	int ocr_number = imageDetect(pixS);
//	return(ocr_number);
//}
