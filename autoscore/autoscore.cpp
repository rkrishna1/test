/*
 * autoscore.cpp
 *
 *  Created on: 21-Dec-2016
 *      Author: rkrishna
 */



#include <iostream>
#include <sstream>
#include <boost/program_options.hpp>

#include "decode_filter.h"
#include "ffmpeg_utils.h"
#include "image_ocr.h"

int64_t parse_start_time(std::string time_str){
	int64_t start_sec = 0;
	if(time_str.size()){
		std::cout << "From Start" + time_str << std::endl;
		if(time_str.find(":") > 0){
			int h, m, s;
			sscanf(time_str.c_str(), "%d:%d:%d", &h, &m, &s);
			start_sec += h* 60 * 60;
			start_sec += m * 60;
			start_sec += s;
		}
		else
			start_sec = atoi(time_str.c_str());

	}
	return(start_sec);
}

//std::istringstream b(blocks);
//		std::string block;
//		while(std::getline(ss, block, ':')){
//			int c, x, y, w, h;
//			std::istringstream b(block);
//			std::string block;
//		}


//Taged_Pos parse_coordinates(std::string score_coord){
//	Taged_Pos co_data;
//	std::map<std::string, cv::Rect> coord;
//
//	if(!score_coord.size())
//		return co_data;
//	std::istringstream ss(score_coord);
//	std::string blocks;
//	while(std::getline(ss, blocks, ';')){
//		std::cout << blocks << std::endl;
//	}
//	return co_data;
//}


void cbfunc(std::string scores)
{
	std::cout << scores << std::endl;
}

int main(int argc, char **argv){

	std::string video_file_path, start_time;
	std::string scoreboard_templete_path;
	std::string game_name;
	std::string score_board_position_string;
	std::string score_position_string;
	std::string outfile_path;
	int rate = 0, duration = -1;
	int64_t start_byte = 0, start_sec = 0;

	std::map<std::string, cv::Rect > data;
//	data.insert( std::pair<std::string, cv::Rect>("Score-1", cv::Rect(260, 354, 40, 16)));
	data.insert( std::pair<std::string, cv::Rect>("Score-1", cv::Rect(275, 350, 30, 25)));

//	data.insert( std::pair<std::string, cv::Rect>("Score-2", cv::Rect(450, 354, 40, 11)));
//	data.insert( std::pair<std::string, cv::Rect>("Score-3", cv::Rect(278, 380, 40, 11)));
//	data.insert( std::pair<std::string, cv::Rect>("Score-4", cv::Rect(468, 380, 40, 11)));


	//	boost::program_options::options_description autoscore_options("Allowed Options for AutoScore");
	//	namespace po = boost::program_options;
	//
	//	autoscore_options.add_options()
	//		("help,h", "Print Help")
	//		("infile,i", po::value<std::string>(&video_file_path)->required(), "Video file to process")
	//		("start_t,g", po::value<std::string>(&start_time), "Game Start Time for processing")
	//		("start_b,b", po::value<int64_t>(&start_byte), "Byte position to start for processing")
	//		("template,t", po::value<std::string>(&scoreboard_templete_path), "Scoreboard Template Image path")
	//		("sport,s", po::value<std::string>(&game_name), "Sport Name Code (Not Implemented)")
	//		("sbloc,l", po::value<std::string>(&score_board_position_string), "Scoreboard Location Ex: -l x,y,w,h")
	//		("ocr,o", po::value<std::string>(&score_position_string), "Multiple Regions to OCR tag_name:x,y,w,h Ex: score1:x,y,w,h score2:x,y,w,h")
	//		("outfile,O", po::value<std::string>(&outfile_path), "Output log file")
	//		("rate,r", po::value<int>(&rate), "Frames per second to process max is 30; Default Full FrameRate")
	//		("duration,d", po::value<int>(&duration));
	//
	//		po::variables_map vm;
	//		po::store(po::parse_command_line(argc, argv, autoscore_options), vm);
	//		po::notify(vm);
	//
	//		if(vm.count("help")){
	//			std::cout << autoscore_options << std::endl;
	//			return(0);
	//		}
	//
	//
	//		if(video_file_path.size())
	//			std::cout << "Processing Video file " + video_file_path << std::endl;
	//		if(start_time.size())
	//			start_sec = parse_start_time(start_time);
	//		if(rate == 0)
	//			rate = 30;
	//
	//		std::cout << score_position_string.size() + " " + score_position_string << std::endl;
//	video_file_path = "/home/rkrishna/MPEGFiles/1038_20161124193000.mpg";
	video_file_path = "/home/rkrishna/MPEGFiles/1038_20161119010000.mpg";
	void (*fp)(std::string)	= &cbfunc;
	FFMPEG_Decode_Filter *ff_decoder = new FFMPEG_Decode_Filter(video_file_path, 500000000, 0, duration);
	ImageConvertCV *img_converter = new ImageConvertCV(ff_decoder->getFormatCtx(), ff_decoder->getVideoStream());
	ImgOCR *tess_ocr = new ImgOCR(data, ff_decoder->getVideoFramerate(), 30, fp);

	int err = 0;
	AVFrame dec_frame;
	float frame_time = 0;
	cv::Mat cvimage;
	while(err >= 0){
		err = ff_decoder->getNextFrame(&dec_frame, &frame_time);
		if(err >= 1){
			img_converter->ffmpegimage_to_matimage(&dec_frame, &cvimage);
			if(cvimage.empty())
				err = -1;
			err = tess_ocr->process_image(cvimage, (int64_t) (frame_time*30));
			av_frame_unref(&dec_frame);
			cvimage.release();
		}
	}
	return(0);
}


