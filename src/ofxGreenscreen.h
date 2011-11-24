#ifndef OFXGREENSCREEN_H
#define OFXGREENSCREEN_H

#include "ofMain.h"
#include <opencv2/opencv.hpp>

typedef cv::Ptr<cv::Mat> cvImg;

class ofxGreenscreen: public ofImage {

public:
	ofxGreenscreen();
	~ofxGreenscreen();
	void setPixels(ofPixelsRef pixels);
	void setPixels(unsigned char*, int w, int h);
	void setBgColor(ofColor col);
	ofColor getBgColor();
	void drawBgColor(int x=0, int y=0, int w=10, int h=10);
	void draw(int x, int y, int w, int h, bool checkers=true);
	void learnBgColor(ofPixelsRef pixels);
	void learnBgColor(ofPixelsRef pixels, int x, int y, int w, int h);

	ofPixels getBaseMask();
	ofPixels getDetailMask();
	ofPixels getMask();

	ofPixels getRedSub();
	ofPixels getBlueSub();
	ofPixels getGreenSub();

	//ALL THE FOLLOWING VALUES ARE IN A RANGE 0 - 1
	float clipBlackBaseMask, clipWhiteBaseMask;
	float clipBlackDetailMask, clipWhiteDetailMask;
	float clipBlackEndMask, clipWhiteEndMask;
	float strengthBaseMask;
	float strengthGreenSpill;

private:
	void update();
	void drawCheckers(int x, int y, int w, int h);

	//private
	int width;
	int height;

	//cv Mats
	cv::Mat mask;
	cv::Mat maskDetail;
	cv::Mat maskBase;
	cv::Mat redSub;
	cv::Mat greenSub;
	cv::Mat blueSub;

	//greenkey color
	ofColor bgColor;

	//raw input data
	cv::Mat input;
};

#endif // OFXGREENSCREEN_H
