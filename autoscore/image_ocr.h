/*
 * image_ocr.h
 *
 *  Created on: 21-Dec-2016
 *      Author: rkrishna
 */

#ifndef IMAGE_OCR_H_
#define IMAGE_OCR_H_

#include "opencv2/ml.hpp"
#include "opencv2/opencv.hpp"
#include "opencv2/imgproc/imgproc.hpp"
#include "opencv2/highgui/highgui.hpp"
#include <tesseract/baseapi.h>
#include <leptonica/allheaders.h>

typedef std::map<std::string, cv::Rect> ProcBoxes;

typedef std::map<int, std::pair<int , int > > ScoreFnCn;
typedef std::map<int, float> DeltaFn;
typedef std::pair<ScoreFnCn, DeltaFn> Scores;


class ImgOCR
{
public:
	/**
	 * Arguments to initialize SB processing for score change detection
	 * @sb_template_path Scoreboard Image Template path: Used to detect accurate/presense of Scoreboard
	 * @sb_loc comma separated scoreboard location x,y,w,h
	 * @ocr_loc comma separated OCR regions tag:x,y,w,h
	 */
	ImgOCR(ProcBoxes cords, int frame_rate, int pr_rate, void *log_cb);
	int process_image(cv::Mat image, int64_t time_stamp);
	void setScoreUpdate(int up_sec){
		score_update_interval = up_sec;
	}
private:
	tesseract::TessBaseAPI *api;
	int video_frame_rate;
	int score_update_interval;
	int processing_rate;
	int64_t frame_number;
	cv::Mat template_image;
	int change_vector_size;
	float Change_threshold;
	ProcBoxes ocr_boxes;
	int TesOCR(PIX *pix);
	PIX *cvtoTes(cv::Mat in_img);
	float calcRunningAvg(float previousAverage, int currentNumber, int index);
	void clean_scores(int64_t frame_number);
	cv::Mat image_preproc(cv::Mat image, cv::Rect score_box);
	std::map<int, int> scorefirst; //Score and First occurrence
	std::map< std::string, Scores>  dscores;
	std::vector<float> sad;
	cv::Mat previous_image;
	void *log_cb;

};



#endif /* IMAGE_OCR_H_ */
