#include "ofxGreenscreen.h"
#include <opencv2/highgui/highgui.hpp>

using namespace cv;

ofxGreenscreen::ofxGreenscreen():width(0), height(0) {
	input = Mat::zeros(5, 5, CV_8UC3);
	bgColor.set(20, 200, 20);

	clipBlackBaseMask = .2;
	clipWhiteBaseMask = .6;
	strengthBaseMask = .3;

	clipBlackDetailMask = .1;
	clipWhiteDetailMask = .6;

	clipBlackEndMask = .1;
	clipWhiteEndMask = .6;

	strengthGreenSpill = .4;
}

ofxGreenscreen::~ofxGreenscreen() {
}

void ofxGreenscreen::learnBgColor(ofPixelsRef pixelSource) {
	learnBgColor(pixelSource, 0, 0, pixelSource.getWidth(), pixelSource.getHeight());
}

void ofxGreenscreen::learnBgColor(ofPixelsRef pixelSource, int x, int y, int w, int h) {
	int wh = w * h;
	int r,g,b;
	r=g=b=0;
	for(int iy=0; iy<h; iy++) {
		for(int ix=0; ix<w; ix++) {
			int i = pixelSource.getPixelIndex(ix+x, iy+y);
			r+=pixelSource[i];
			g+=pixelSource[i+1];
			b+=pixelSource[i+2];
		}
	}
	r/=wh;
	g/=wh;
	b/=wh;
	bgColor.set(r, g, b);
	update();
}

void ofxGreenscreen::setBgColor(ofColor col) {
	bgColor = col;
}

void ofxGreenscreen::setPixels(ofPixelsRef pixels) {
	setPixels(pixels.getPixels(), pixels.getWidth(), pixels.getHeight());
}

void ofxGreenscreen::setPixels(unsigned char* pixels, int w, int h) {
	width = w;
	height = h;
	input = Mat(height, width, CV_8UC3, pixels);
	update();
}

void mapImage(const Mat& src, CV_OUT Mat& dst, float min, float max) {
	//Mat::
	int dim(256);
	Mat lookup(1, &dim, CV_8U);
	int mi = min * 255;
	int ma = max * 255;
	for(int i=0; i<256; i++) {
		lookup.at<unsigned char>(i) = ofMap(i, mi, ma, 0, 255, true);
	}
	LUT(src,lookup,dst);
}

void ofxGreenscreen::update() {
	if(width == 0 || height == 0)
		return;

	// THE FOLLOWING KEYING METHOD RELIES HEAVILY ON THIS ARTICLE
	// http://www.blendedplanet.com/?Planet_Blog:Greenscreen_Keying

	//split the rgb into individual channels
	std::vector<Mat> rgbInput;
	split(input, rgbInput);

	//subtract the background form each channel and invert the green
	redSub = rgbInput[0] - bgColor.r;
	bitwise_not(rgbInput[1], greenSub);
	greenSub -= 255 - bgColor.g;
	blueSub = rgbInput[0] - bgColor.b;

	//create the detail mask
	maskDetail = redSub + greenSub + blueSub;
	Mat maskDetailSpill = maskDetail;
	mapImage(maskDetail, maskDetail, clipBlackDetailMask, clipWhiteDetailMask);

	//create the mask green minus red, invert it, darken & erode
	maskBase = rgbInput[1] - rgbInput[0];
	bitwise_not(maskBase, maskBase);
	maskBase -= (1-strengthBaseMask)*255;
	mapImage(maskBase, maskBase, clipBlackBaseMask, clipWhiteBaseMask);
	blur(maskBase, maskBase, Size(5, 5));
	dilate(maskBase, maskBase, Mat());
	erode(maskBase, maskBase, Mat());

	//create the final mask
	mask = maskDetail + maskBase;
	mapImage(mask, mask, clipBlackEndMask, clipWhiteEndMask);

	//REMOVE GREEN SPILL with a multiply filter
	//blur(maskDetailSpill, maskDetailSpill, Size(5, 5));
	float opac = strengthGreenSpill;
	for( int i=0; i<width*height; i++){
		//rgbInput[1].data[i] = maskDetail.data[i] * (rgbInput[1].data[i])  / 255 ;
		rgbInput[1].data[i] = (rgbInput[1].data[i] * (1 - opac)) + (rgbInput[1].data[i] * (maskDetailSpill.data[i] / 255) * opac);
		if(rgbInput[1].data[i]>255)
			rgbInput[1].data[i] = 255;
		if(rgbInput[1].data[i]<0)
			rgbInput[1].data[i] = 0;
	}


	//MERGE IT ALL
	Mat composition;
	std::vector<Mat> rgbaOutput;
	rgbaOutput.push_back(rgbInput[0]);
	rgbaOutput.push_back(rgbInput[1]);
	rgbaOutput.push_back(rgbInput[2]);
	rgbaOutput.push_back(mask);
	merge(rgbaOutput, composition);

	setFromPixels((unsigned char*)composition.data, width, height, OF_IMAGE_COLOR_ALPHA);

	//setFromPixels((unsigned char*)mask.data, width, height, OF_IMAGE_GRAYSCALE);
}

void ofxGreenscreen::drawBgColor(int x, int y, int w, int h) {
	ofFill();
	ofSetColor(bgColor);
	ofRect(x, y, w, h);
}

void ofxGreenscreen::drawCheckers(int x, int y, int w, int h) {
	int rectSize = 10;
	ofColor a(30);
	ofColor b(255);
	ofFill();
	int maxH=h/rectSize;
	int maxW=w/rectSize;
	for(int iy=0; iy<maxH; iy++) {
		for(int ix=0; ix<maxW; ix++) {
			if(iy%2==0)
				ix%2==0?ofSetColor(a):ofSetColor(b);
			else
				ix%2==0?ofSetColor(b):ofSetColor(a);
			ofRect(x+ix*rectSize, y+iy*rectSize, rectSize, rectSize);
		}
	}
}

void ofxGreenscreen::draw(int x, int y, int w, int h, bool checkers) {
	if(width == 0 ||  height == 0)
		return;
	ofEnableAlphaBlending();
	if(checkers)
		drawCheckers(x, y, w, h);
	ofSetColor(255);
	ofImage::draw(x, y, w, h);
}

ofColor ofxGreenscreen::getBgColor() {
	return bgColor;
}

ofPixels matToPixRef(Mat* m) {
	ofPixels ret;
	ofImageType type = OF_IMAGE_GRAYSCALE;
	if(m->channels() == 3)
		type = OF_IMAGE_COLOR;
	//ret.allocate(m->size[0], m->size[1], channel);
	ret.setFromPixels(m->data, m->size[1], m->size[0], type);
	return ret;
}

ofPixels ofxGreenscreen::getBaseMask() {
	return matToPixRef(&maskBase);
}

ofPixels ofxGreenscreen::getBlueSub() {
	return matToPixRef(&blueSub);
}

ofPixels ofxGreenscreen::getDetailMask() {
	return matToPixRef(&maskDetail);
}

ofPixels ofxGreenscreen::getGreenSub() {
	return matToPixRef(&greenSub);
}

ofPixels ofxGreenscreen::getMask() {
	return matToPixRef(&mask);
}

ofPixels ofxGreenscreen::getRedSub() {
	return matToPixRef(&redSub);
}
