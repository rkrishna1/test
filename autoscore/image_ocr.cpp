/*
 * image_ocr.cpp
 *
 *  Created on: 21-Dec-2016
 *      Author: rkrishna
 */

#include "image_ocr.h"
#include "opencv2/imgproc/imgproc.hpp"


ImgOCR::ImgOCR(ProcBoxes cords, int frame_rate, int pr_rate, void *log):api(NULL), video_frame_rate(30), score_update_interval(5),
processing_rate(pr_rate), frame_number(0), change_vector_size(15),
Change_threshold(20), ocr_boxes(cords), log_cb(log)
{
	api = new tesseract::TessBaseAPI();
	api->SetVariable("tessedit_char_whitelist", "0123456789");
	// Initialize tesseract-ocr with English, without specifying tessdata path
	if (api->Init("/usr/share/tesseract-ocr", "eng")) {
		std::cout << "Errro" << std::endl;
	}
	cv::namedWindow("Test", CV_WINDOW_NORMAL);
	return;
}

float ImgOCR::calcRunningAvg(float previousAverage, int currentNumber, int index) {
    // [ avg' * (n-1) + x ] / n
    return ( previousAverage * (index - 1) + currentNumber ) / index;
}


cv::Mat ImgOCR::image_preproc(cv::Mat image, cv::Rect score_box){

	cv::Mat in_img = image(score_box);
	cv::Mat out_img;
	if((!in_img.rows || !in_img.cols) && (in_img.channels() != 1)){
		return out_img;
	}
	//Minimum height of digit
	int std_min_height = 120;
	if(in_img.rows < std_min_height){
		float aspect = (float) in_img.rows/in_img.cols;
		cv::resize(in_img, out_img, cv::Size(round((1/aspect)*std_min_height), std_min_height), 0, 0, cv::INTER_CUBIC);
	}
	cv::GaussianBlur(out_img, out_img, cv::Size(3, 3), 0, 0);
	in_img.release();
	return out_img;
}

int ImgOCR::process_image(cv::Mat image, int64_t frame_number){

	int err = 0;
	cv::Rect tmp_box;
	std::string tmp_tag;
	PIX *tmp_tess;

	//Tracking score changes if we
	for(ProcBoxes::iterator itr = ocr_boxes.begin(); itr != ocr_boxes.end(); itr++){
		tmp_box = itr->second;
		tmp_tag = itr->first;
		cv::Mat target = image_preproc(image, tmp_box);
		if(target.empty())
			return -1;
		tmp_tess = cvtoTes(target);
		int ocr_number = TesOCR(tmp_tess);
		if(dscores.find(tmp_tag) == dscores.end()){
			dscores.insert(std::pair<std::string, Scores> (tmp_tag, Scores()));
		}
		Scores &_scr = dscores.find(tmp_tag)->second;
		if(ocr_number >= 0){
			//Find if element is present
			ScoreFnCn &_sfc = _scr.first;
			DeltaFn &_dfn = _scr.second;
			if(_sfc.find(ocr_number) == _sfc.end()){
				//Score not registered
				std::cout << "[" << frame_number << "]" << "NEWSCORE[" << ocr_number << "]" << std::endl;
				_sfc.insert(std::pair<int, std::pair<int, int> >(ocr_number, std::pair<int, int> (frame_number, 1)));
				_dfn.insert(std::pair<int, float>(ocr_number, 0));
				scorefirst.insert(std::pair<int, int> (ocr_number, frame_number));
			}
			else{
				_dfn[ocr_number] =
						calcRunningAvg(_dfn[ocr_number], frame_number-_sfc[ocr_number].first, _sfc[ocr_number].second);
				_sfc[ocr_number].first = frame_number;
				_sfc[ocr_number].second++;
				std::cout << "Score " << tmp_tag << " " << ocr_number << " Weight " << _sfc[ocr_number].second/_dfn[ocr_number] << std::endl;
			}
		}

	}
	clean_scores(frame_number);
	return(err);
}


void ImgOCR::clean_scores(int64_t frame_number){
	std::map< std::string, Scores>::iterator sr_it;
	std::map<int, int>::iterator _itr;
	std::string dat = "Iam calling a callback";
	log_cb(dat);
	for(sr_it = dscores.begin(); sr_it != dscores.end(); sr_it++){
		ScoreFnCn::iterator _it;
		for(_it = sr_it->second.first.begin(); _it != sr_it->second.first.end(); _it++){
			if(_it->second.first - frame_number > 1800){
				sr_it->second.first.erase(_it->first);
				sr_it->second.second.erase(_it->first);
				scorefirst.erase(_it->first);
			}
		}
	}
}


PIX *ImgOCR::cvtoTes(cv::Mat in_img){
	cv::imshow("Test" , in_img);
	cv::waitKey(20);
	PIX *pixS = pixCreate(in_img.size().width, in_img.size().height, 8);
	for(int p=0; p<in_img.rows; p++)
		for(int j=0; j<in_img.cols; j++)
			pixSetPixel(pixS, j,p, (l_uint32) in_img.at<uchar>(p,j));
	in_img.release();
	return(pixS);
}



int ImgOCR::TesOCR(PIX *pix){

	char *outText, *dat = new char[128];
	api->SetImage(pix);
	// Get OCR result
	outText = api->GetUTF8Text();
	pixDestroy(&pix);
	int i= 0, j = 0;
	while(outText[i] != '\0'){
		if(isdigit((int) outText[i])){
			dat[j] = (int) outText[i];
			j++;
		}
		i++;
	}
	dat[j] = '\0';
	delete outText;
	int64_t number  = -1;
	if(j)
	 number = atoll(dat);
	delete dat;
	return number;
}
