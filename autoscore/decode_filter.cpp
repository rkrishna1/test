/*
 * decode_filter.cpp
 *
 *  Created on: 21-Dec-2016
 *      Author: rkrishna
 */


#include "decode_filter.h"

/*
 *	Initilize FFMPEG Decoder and Filter
 *
 *
 *
 *
 */
FFMPEG_Decode_Filter::FFMPEG_Decode_Filter(std::string video_file_name, int64_t byte_pos, int start_sec, int duration):
	video_file_path(video_file_name), byte_pos(byte_pos), start_sec(start_sec), filter_graph(NULL),
	frame_number(0), n_frame_rate(30), buffersrc_ctx(NULL), buffersink_ctx(NULL), flush_decode_pipe(false), dframe(false),
	done_decode(false), filter_err(false)
{

	av_register_all();
	// Allocate FFMPEG Format Context Open video file
	if((format_ctx = avformat_alloc_context()) &&
			(avformat_open_input(&(format_ctx), video_file_path.c_str(), NULL, NULL) < 0)){
		return;
	}
	format_ctx->max_analyze_duration = 12 * AV_TIME_BASE;
	if(avformat_find_stream_info(format_ctx, NULL) < 0){
		return;
	}
	for(unsigned int i = 0; i < format_ctx->nb_streams; i++){
		video_stream_index = -1;
		if(format_ctx->streams[i]->codec->codec_type != AVMEDIA_TYPE_VIDEO){
			continue;
		}

		if(!format_ctx->streams[i]->codec->height ||
				!format_ctx->streams[i]->codec->width ||
				format_ctx->streams[i]->codec->pix_fmt == AV_PIX_FMT_NONE){
			continue;
		}
		// Get a pointer to the codec context for the audio stream
		video_stream_index = i;
		video_width = format_ctx->streams[video_stream_index]->codec->width;
		video_height = format_ctx->streams[video_stream_index]->codec->height;
		pix_fmt = format_ctx->streams[video_stream_index]->codec->pix_fmt;
		// Find the decoder for the audio stream
		AVCodec *video_codec = avcodec_find_decoder((enum AVCodecID) format_ctx->streams[i]->codec->codec_id);
		if(video_codec == NULL){
			continue;
		}
		// Open codec to decode Video
		if(avcodec_open2(format_ctx->streams[i]->codec, video_codec, NULL) < 0){
			continue;
		}
		break;
	}
	if(byte_pos)
		seek_ok = avformat_seek_file(format_ctx, -1, byte_pos, byte_pos, byte_pos, AVSEEK_FLAG_BYTE);
	else if(start_sec)
		seek_ok = av_seek_frame(format_ctx, video_stream_index, start_sec*1000, 0);
	else
		seek_ok = 1;
	if(Init_Deinterlace_filter() < 0)
		cleanfilter();
	n_frame_rate = CalcFrameRate(format_ctx, video_stream_index);
	return;
}


int FFMPEG_Decode_Filter::CalcFrameRate(AVFormatContext *format_ctx, int stream_id){

	AVStream *st = format_ctx->streams[stream_id];
	float actual_fps;
	if(st->codec->codec_type != AVMEDIA_TYPE_VIDEO)
		return(30);
	int fps = st->avg_frame_rate.den && st->avg_frame_rate.num;
	int tbr = st->r_frame_rate.den && st->r_frame_rate.num;
	if (fps)
		actual_fps = av_q2d(st->avg_frame_rate);
	else if (tbr)
		actual_fps = av_q2d(st->r_frame_rate);
	return(ceil(actual_fps));
}


int FFMPEG_Decode_Filter::Init_Deinterlace_filter()
{
    int ret = 0;
    AVCodecContext *dec_ctx = format_ctx->streams[video_stream_index]->codec;
    buffersink_ctx = new AVFilterContext;
    buffersrc_ctx = new AVFilterContext;
    deinterlace_ctx = new AVFilterContext;

    avfilter_register_all();

    AVFilter *buffersrc  = avfilter_get_by_name("buffer");
    AVFilter *buffersink = avfilter_get_by_name("buffersink");
    AVFilter *deinterlace = avfilter_get_by_name("yadif");
    AVRational time_base = format_ctx->streams[video_stream_index]->time_base;


    enum AVPixelFormat pix_fmts[] = { dec_ctx->pix_fmt , AV_PIX_FMT_NONE };

    filter_graph = avfilter_graph_alloc();
    if (!filter_graph) {
        ret = AVERROR(ENOMEM);
        return(ret);
    }

    buffersrc_ctx = avfilter_graph_alloc_filter(filter_graph, buffersrc, "Src");
    deinterlace_ctx = avfilter_graph_alloc_filter(filter_graph, deinterlace, "Yadif");
    buffersink_ctx = avfilter_graph_alloc_filter(filter_graph, buffersink, "Sink");

    if(!buffersrc_ctx || !deinterlace_ctx || !buffersink_ctx){
    	av_freep(filter_graph);
    	return(AVERROR(ENOMEM));
    }

    ret = av_opt_set_q(buffersrc_ctx, "frame_rate", dec_ctx->framerate, AV_OPT_SEARCH_CHILDREN);
    if(ret ==  AVERROR_OPTION_NOT_FOUND){
    	std::cout << "error setting rate" << std::endl;
    }
    ret = av_opt_set_int(buffersrc_ctx, "width", dec_ctx->width, AV_OPT_SEARCH_CHILDREN);
    if(ret ==  AVERROR_OPTION_NOT_FOUND){
    	std::cout << "error setting width" << std::endl;
    	return(ret);
    }
    ret = av_opt_set_int(buffersrc_ctx, "height", dec_ctx->height, AV_OPT_SEARCH_CHILDREN);
    if(ret ==  AVERROR_OPTION_NOT_FOUND){
    	std::cout << "error setting height" << std::endl;
    	std::cout << "error setting width" << std::endl;
    }
    ret = av_opt_set_int(buffersrc_ctx, "pix_fmt", dec_ctx->pix_fmt, AV_OPT_SEARCH_CHILDREN);
    if(ret ==  AVERROR_OPTION_NOT_FOUND){
    	std::cout << "error setting pix_fmt" << std::endl;
    	return(ret);
    }
    ret = av_opt_set_q(buffersrc_ctx, "time_base", time_base, AV_OPT_SEARCH_CHILDREN);
    if(ret ==  AVERROR_OPTION_NOT_FOUND){
    	std::cout << "error setting Time Base" << std::endl;
    	return(ret);
    }
    ret = av_opt_set_q(buffersrc_ctx, "pixel_aspect", dec_ctx->sample_aspect_ratio, AV_OPT_SEARCH_CHILDREN);
    if(ret ==  AVERROR_OPTION_NOT_FOUND){
    	std::cout << "error setting pix aspect" << std::endl;
    	return(ret);
    }

    ret = avfilter_init_str(buffersrc_ctx, NULL);
    if (ret < 0) {
        return(ret);
    }
    ret = avfilter_init_str(deinterlace_ctx, NULL);
    if (ret < 0) {
    	return(ret);
    }
    ret = av_opt_set_int_list(buffersink_ctx, "pix_fmts", pix_fmts, AV_PIX_FMT_NONE, AV_OPT_SEARCH_CHILDREN);
    if (ret < 0) {
    	return(ret);
    }
    ret = avfilter_init_str(buffersink_ctx, NULL);
    if (ret < 0) {
    	return(ret);
    }

    //Now link the filters we crates to form a pipeline
    ret = avfilter_link(buffersrc_ctx, 0, deinterlace_ctx, 0);
    if (ret >= 0)
    	ret = avfilter_link(deinterlace_ctx, 0, buffersink_ctx, 0);
    if(ret < 0){
    	std::cout << "Error Linking Filters" << std::endl;
    	return(ret);
    }
    if ((ret = avfilter_graph_config(filter_graph, NULL)) < 0){
    	std::cout << "Unable to config Filter" << std::endl;
    	return(ret);
    }
    return(1);
}

FFMPEG_Decode_Filter::~FFMPEG_Decode_Filter(){
	avcodec_close(format_ctx->streams[video_stream_index]->codec);
	avformat_free_context(format_ctx);
	cleanfilter();
}


void FFMPEG_Decode_Filter::cleanfilter(){
	if(buffersink_ctx)
		avfilter_free(buffersink_ctx);
	if(buffersrc_ctx)
		avfilter_free(buffersrc_ctx);
	if(deinterlace_ctx)
		avfilter_free(deinterlace_ctx);
	if(filter_graph)
		avfilter_graph_free(&filter_graph);
	filter_graph = NULL;
}


int FFMPEG_Decode_Filter::getNextFrame(AVFrame *dec_frame, float *frame_time){

	int err, ret = 0;

	*frame_time = (float)frame_number/n_frame_rate;

	if(!done_decode){
		err = ffmpegDecodeFromPacket();
		if(err < 0){
			std::cout << "Error Decoding Video frame Exiting....." << std::endl;
			done_decode = true;
		}
	}

	if(dframe){
		bool frame_change = decoded_picture.width != video_width
				|| decoded_picture.height != video_height
				|| decoded_picture.format != pix_fmt;
		//Regular processing
		if(filter_graph && !frame_change && !filter_err){
			err = DeinterlaceImage();
			if(err < 0){
				std::cout << "Unable De-interlace Image";
				filter_err = true;
			}
		}
	}
	if(dframe){
		av_frame_move_ref(dec_frame, &decoded_picture);
		av_frame_unref(&decoded_picture);
		dframe = false;
		ret = 1;
	}
	if(done_decode)
		return(-1);
	return (ret);
}

int FFMPEG_Decode_Filter::ffmpegDecodeFromPacket(void){

	AVPacket pkt;
	int ret = 0, got_frame = 0, process_frame = 0;
	av_init_packet(&pkt);
	dframe = false;

	while(!process_frame){
		if(!flush_decode_pipe){
			ret = av_read_frame(format_ctx, &pkt);
			if( ret == AVERROR(EAGAIN))
				continue;
			if(ret < 0){
				flush_decode_pipe = true;
			}
		}
		if(flush_decode_pipe){
			pkt.size = 0;
			pkt.data = NULL;
		}
		if(flush_decode_pipe || (pkt.size && pkt.stream_index == video_stream_index)){
			ret = avcodec_decode_video2(format_ctx->streams[video_stream_index]->codec, &decoded_picture,
					&got_frame, &pkt);
			if(ret < 0){
				return(ret);
			}
			if(flush_decode_pipe && !got_frame)
				return(-1);
		}
		if(!flush_decode_pipe)
			av_packet_unref(&pkt);

		if(got_frame){
			++frame_number;
			process_frame = 1;
		}
	}
	if(process_frame){
		dframe = true;
	}
	return(0);
}

int FFMPEG_Decode_Filter::DeinterlaceImage(void){
	int err;
	if(dframe){
		err = av_buffersrc_add_frame(buffersrc_ctx, &decoded_picture);
		av_frame_unref(&decoded_picture);
		dframe = false;
		if(err < 0){
			std::cout << "Unable Push frame";
			return(err);
		}
	}
	err = av_buffersink_get_frame(buffersink_ctx, &decoded_picture);
	if(err < 0 && err != AVERROR(EAGAIN)){
		std::cout << "Error reading Frame";
		return(err);
	}
	if(err >= 0)
		dframe = true;
	return 0;
}


