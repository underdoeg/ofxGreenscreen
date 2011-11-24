#include "testApp.h"
#define USE_GUI

#ifdef USE_GUI
#include "ofxSimpleGuiToo.h"
float bgColor[4];
#endif

//--------------------------------------------------------------
void testApp::setup() {
	grabber.initGrabber(1280, 960);

#ifdef USE_GUI
	//gui.addTitle("SETTINGS");
	gui.addColorPicker("key color", bgColor);
	gui.addSlider("base mask strength", greenscreen.strengthBaseMask, 0.0, 1.f);
	gui.addSlider("green spill strength", greenscreen.strengthGreenSpill, 0.0, 1.f);


	//gui.addTitle("CLIPPING");
	gui.addSlider("base mask black", greenscreen.clipBlackBaseMask, 0.0, 1.f);
	gui.addSlider("base mask white", greenscreen.clipWhiteBaseMask, 0.0, 1.f);
	gui.addSlider("detail mask black", greenscreen.clipBlackDetailMask, 0.0, 1.f);
	gui.addSlider("detail mask white", greenscreen.clipWhiteDetailMask, 0.0, 1.f);
	gui.addSlider("end mask black", greenscreen.clipBlackDetailMask, 0.0, 1.f);
	gui.addSlider("end mask white", greenscreen.clipWhiteDetailMask, 0.0, 1.f);

	//gui.addTitle("OUTPUT");
	gui.addFPSCounter();
	gui.addContent("camera", grabber);
	gui.addContent("base mask", baseMask);
	gui.addContent("detail mask", detailMask);
	gui.addContent("mask", mask);

	gui.addContent("red sub", redSub);
	gui.addContent("green sub", greenSub);
	gui.addContent("blue sub", blueSub);
	//gui.addContent("keyed", greenscreen);

	gui.loadFromXML();
#endif
}

//--------------------------------------------------------------
void testApp::update() {
	grabber.update();
	if(grabber.isFrameNew())
		greenscreen.setPixels(grabber.getPixelsRef());
#ifdef USE_GUI
	if(gui.isOn()){
		greenscreen.setBgColor(ofColor(bgColor[0]*255, bgColor[1]*255, bgColor[2]*255));

		//THIS PART IS REALLY SLOW!!!
		mask.setFromPixels(greenscreen.getMask());
		detailMask.setFromPixels(greenscreen.getDetailMask());
		baseMask.setFromPixels(greenscreen.getBaseMask());
		redSub.setFromPixels(greenscreen.getRedSub());
		greenSub.setFromPixels(greenscreen.getGreenSub());
		blueSub.setFromPixels(greenscreen.getBlueSub());
	}
#endif
}

//--------------------------------------------------------------
void testApp::draw() {
	ofEnableAlphaBlending();
	greenscreen.draw(0, 0, greenscreen.getWidth(), greenscreen.getHeight());
	//grabber.draw(0, 0, 214, 160);
	greenscreen.drawBgColor();
#ifdef USE_GUI
	if(!gui.isOn()){
		ofSetColor(0);
		ofDrawBitmapString("FPS "+ofToString(ofGetFrameRate()), 5, greenscreen.getHeight()+20);
	}
	gui.draw();
#endif
}

//--------------------------------------------------------------
void testApp::keyPressed(int key) {
	/*if(key == OF_KEY_DOWN)
		greenscreen.clipBlackEndMask -= .01;
	if(key == OF_KEY_UP)
		greenscreen.clipBlackEndMask += .01;
	if(key == OF_KEY_LEFT)
		greenscreen.clipWhiteEndMask -= .01;
	if(key == OF_KEY_RIGHT)
		greenscreen.clipWhiteEndMask += .01;
	*/

}

//--------------------------------------------------------------
void testApp::keyReleased(int key) {
#ifdef USE_GUI
	if(key == ' ')
		gui.toggleDraw();
#endif
}

//--------------------------------------------------------------
void testApp::mouseMoved(int x, int y ) {

}

//--------------------------------------------------------------
void testApp::mouseDragged(int x, int y, int button) {
	/*
	if(x== dragStart.x || y==dragStart.y)
		return;
	greenscreen.learnBgColor(grabber.getPixelsRef(), dragStart.x, dragStart.y, x-dragStart.x, y-dragStart.y);
	*/
}

//--------------------------------------------------------------
void testApp::mousePressed(int x, int y, int button) {
#ifdef USE_GUI
	if(!gui.isOn())
#endif
		greenscreen.setBgColor(grabber.getPixelsRef().getColor(x, y));
#ifdef USE_GUI
	ofColor c = greenscreen.getBgColor();
	bgColor[0] = c.r/255.;
	bgColor[1] = c.g/255.;
	bgColor[2] = c.b/255.;
#endif
}

//--------------------------------------------------------------
void testApp::mouseReleased(int x, int y, int button) {

}

//--------------------------------------------------------------
void testApp::windowResized(int w, int h) {

}

//--------------------------------------------------------------
void testApp::gotMessage(ofMessage msg) {

}

//--------------------------------------------------------------
void testApp::dragEvent(ofDragInfo dragInfo) {

}
