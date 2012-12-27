#include "cinder/app/AppBasic.h"
#include "cinder/gl/gl.h"
#include "cinder/gl/Texture.h"

#define USING_NITE
#include "ciOpenNI.h"

using namespace ci;
using namespace ci::app;
using namespace std;

class OpenNIHandsApp : public AppBasic {
    
    public:
	void setup();
	void mouseDown( MouseEvent event );	
	void update();
	void draw();
	void exit();
    
    nite::HandTracker handTracker;
	nite::HandTrackerFrameRef handTrackerFrame;

	gl::Texture depth;
};

void OpenNIHandsApp::setup()
{
   nite::Status niteRc = nite::NiTE::initialize();
	if (niteRc != nite::STATUS_OK)
	{
		printf("NiTE initialization failed\n");
		return;
	}

	niteRc = handTracker.create();
	if (niteRc != nite::STATUS_OK)
	{
		printf("Couldn't create user tracker\n");
		return;
	}

	// we're only listening for these two
	handTracker.startGestureDetection(nite::GESTURE_WAVE);
	handTracker.startGestureDetection(nite::GESTURE_CLICK);

    gl::Texture::Format format;

    depth = gl::Texture(320, 240, format);
}

void OpenNIHandsApp::mouseDown( MouseEvent event )
{
}

void OpenNIHandsApp::update()
{
	nite::Status niteRc = handTracker.readFrame(&handTrackerFrame);
	if (niteRc != nite::STATUS_OK)
	{
		console() << "Get next frame failed " << endl;
		return;
	}

	// get the texture
	depth = gl::Texture(fromOpenNI(handTrackerFrame.getDepthFrame()));

	const nite::Array<nite::GestureData>& gestures = handTrackerFrame.getGestures();
	for (int i = 0; i < gestures.getSize(); ++i)
	{
		if (gestures[i].isComplete())
		{

			if(gestures[i].getType() == nite::GESTURE_CLICK) {
				console() << " caught a click " << endl;
			} else if(gestures[i].getType() == nite::GESTURE_HAND_RAISE) {
				console() << " caught a hand raise " << endl;
			} else if(gestures[i].getType() == nite::GESTURE_WAVE) {
				console() << " caught a wave " << endl;
			}

			nite::HandId newId;
			handTracker.startHandTracking(gestures[i].getCurrentPosition(), &newId);
		}
	}

	const nite::Array<nite::HandData>& hands = handTrackerFrame.getHands();
	for (int i = 0; i < hands.getSize(); ++i)
	{
		const nite::HandData& hand = hands[i];
		if (hand.isTracking())
		{
			console() << " Hand ID " << hand.getId() << " X: " << hand.getPosition().x << " Y: " << hand.getPosition().y << " Z: " << hand.getPosition().z << endl;
		}
	}
}

void OpenNIHandsApp::draw()
{
	// clear out the window with black
	gl::clear( Color( 0, 0, 0 ) ); 
}

void OpenNIHandsApp::exit()
{
	nite::NiTE::shutdown();
	handTrackerFrame.release();
}

CINDER_APP_BASIC( OpenNIHandsApp, RendererGl )
