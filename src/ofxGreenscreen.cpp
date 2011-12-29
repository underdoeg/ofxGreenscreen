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

	clipBlackChromaMask = 0.05;
	clipWhiteChromaMask = .95;

	strengthGreenSpill = .4;
	strengthChromaMask = .4;

	cropBottom = cropLeft = cropRight = cropTop = 0;

	doBaseMask = doChromaMask = doDetailMask = doGreenSpill = true;
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
	/*if(w != width || h != height) //doesn't work, don't know why...
		maskChroma = Mat(height, width, DataType<unsigned char>::type);
	*/
	width = w-cropLeft*w-cropRight*w;
	height = h-cropTop*h-cropBottom*h;
	input = Mat(h, w, CV_8UC3, pixels);
	if(cropBottom != 0 || cropTop != 0 || cropLeft != 0 || cropRight != 0) {
		cv::Rect rect;
		rect.x      = cropLeft*w;
		rect.y      = cropTop*h;
		rect.width  = w-cropRight*w-rect.x;
		rect.height = h-cropBottom*h-rect.y;
		input = input(rect);
	}
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
	red = rgbInput[0];
	green = rgbInput[1];
	blue = rgbInput[2];

	//subtract the background form each channel and invert the green
	redSub = red - bgColor.r;
	bitwise_not(green, greenSub);
	greenSub -= 255 - bgColor.g;
	blueSub = blue - bgColor.b;

	maskChroma = Mat(height, width, DataType<unsigned char>::type);
	maskChroma = Scalar(255);

	//create the detail mask
	if(doDetailMask) {
		maskDetail = redSub + greenSub + blueSub;
		Mat maskDetailSpill = maskDetail;
		mapImage(maskDetail, maskDetail, clipBlackDetailMask, clipWhiteDetailMask);
	} else {
		maskDetail = Mat(height, width, DataType<unsigned char>::type);
		maskDetail = Scalar(255);
	}

	//create the mask green minus red, invert it, darken & erode
	if(doBaseMask) {
		maskBase = green - red;
		bitwise_not(maskBase, maskBase);
		maskBase -= (1-strengthBaseMask)*255;
		mapImage(maskBase, maskBase, clipBlackBaseMask, clipWhiteBaseMask);
		blur(maskBase, maskBase, cv::Size(5, 5));
		dilate(maskBase, maskBase, Mat());
		erode(maskBase, maskBase, Mat());
	} else {
		maskBase = Mat(height, width, DataType<unsigned char>::type);
		maskBase = Scalar(255);
	}


	if(doGreenSpill || doChromaMask) {
		//REMOVE GREEN SPILL with a multiply filter
		Mat hsvInput;
		cvtColor(input, hsvInput, CV_RGB2HSV);

		float amount = strengthGreenSpill*4;
		float hue = ofMap(bgColor.getHue(), 0, 255, 0, 1);
		for(int y=0; y < height; y++) {
			for(int x=0; x < width; x++) {
				float f =  sin( 2 * PI * ( hue + ( .25 - hsvInput.at<Vec3b>(y, x)[0]/180. )));
				if(doChromaMask) {
					float fi = f;
					if(fi>1)
						fi = 1;
					if(fi<0)
						fi = 0;
					maskChroma.at<unsigned char>(y, x) = fi*255;
				}
				if(doGreenSpill) {
					f = f * amount;
					if(f<0)
						f = 0;
					else if(f>1)
						f = 1;
					hsvInput.at<Vec3b>(y, x)[1] *= 1-f;
				}
			}
		}

		if(doGreenSpill) {
			cvtColor(hsvInput, input, CV_HSV2RGB);
			split(input, rgbInput);
			red = rgbInput[0];
			green = rgbInput[1];
			blue = rgbInput[2];
		}

		//work on the chroma mask
		bitwise_not(maskChroma, maskChroma);
		blur(maskChroma, maskChroma, cv::Size(5, 5));
		maskChroma -= (1-strengthChromaMask)*255;
		maskChroma *= strengthChromaMask;
	}

	//create the final mask
	mask = Mat(height, width, DataType<unsigned char>::type);
	if(!doDetailMask && !doChromaMask && !doBaseMask) {
		mask = Scalar(255);
	} else {
		mask = Scalar(0);
		if(doBaseMask)
			mask += maskBase;
		if(doDetailMask)
			mask += maskDetail;
		if(doChromaMask)
			mask += maskChroma;
		mapImage(mask, mask, clipBlackEndMask, clipWhiteEndMask);
	}

	//MERGE IT ALL
	Mat composition;
	std::vector<Mat> rgbaOutput;
	rgbaOutput.push_back(red);
	rgbaOutput.push_back(green);
	rgbaOutput.push_back(blue);
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
	ofImage::draw(x+cropLeft*w, y+cropTop*h, w-cropLeft*w-cropRight*w, h-cropTop*h-cropBottom*h);
}

ofColor ofxGreenscreen::getBgColor() {
	return bgColor;
}

ofPixels matToOfPixels(Mat* m) {
	ofPixels ret;
	ret.setFromExternalPixels(m->data, m->size[1], m->size[0], m->channels());
	return ret;
}

ofPixels ofxGreenscreen::getBaseMask() {
	return matToOfPixels(&maskBase);
}

ofPixels ofxGreenscreen::getBlueSub() {
	return matToOfPixels(&blueSub);
}

ofPixels ofxGreenscreen::getDetailMask() {
	return matToOfPixels(&maskDetail);
}

ofPixels ofxGreenscreen::getGreenSub() {
	return matToOfPixels(&greenSub);
}

ofPixels ofxGreenscreen::getMask() {
	return matToOfPixels(&mask);
}

ofPixels ofxGreenscreen::getRedSub() {
	return matToOfPixels(&redSub);
}
ofPixels ofxGreenscreen::getChromaMask() {
	return matToOfPixels(&maskChroma);
}

void ofxGreenscreen::setCropLeft(float val) {
	cropLeft = val;
}

void ofxGreenscreen::setCropRight(float val) {
	cropRight = val;
}
