#include "cinder/app/AppBasic.h"
#include "cinder/gl/gl.h"
#include "cinder/gl/Texture.h"

#include "ciOpenNI.h"

using namespace ci;
using namespace ci::app;
using namespace std;

class oNIViewrApp : public AppBasic, public openni::VideoStream::Listener {
  public:

	oNIViewrApp();
	void setup();
	void mouseDown( MouseEvent event );	
	void update();
	void draw();

	int texWidth, texHeight;

	void onNewFrame(openni::VideoStream& stream);

	openni::Device mDevice;
	openni::VideoStream mDepthStream;
	openni::VideoStream mColorStream;
	openni::VideoFrameRef mDFrame, mRGBFrame;

	gl::Texture depthTex, colorTex;

};

oNIViewrApp::oNIViewrApp() {
}

void oNIViewrApp::setup()
{

	openni::Status rc = openni::OpenNI::initialize();
	if (rc != openni::STATUS_OK)
	{
		console () << " OpenNI Initialize failed " << openni::OpenNI::getExtendedError() << endl;
	}

	openni::Device device;
	rc = device.open(openni::ANY_DEVICE);
	if (rc != openni::STATUS_OK)
	{
		console() << "Couldn't open device " << openni::OpenNI::getExtendedError() << endl;
	}

	if (device.getSensorInfo(openni::SENSOR_COLOR) != NULL)
	{
		rc = mColorStream.create(device, openni::SENSOR_COLOR);
		if (rc != openni::STATUS_OK)
		{
			console() << "Couldn't create color stream " << openni::OpenNI::getExtendedError() << endl;
		}
	}
	rc = mColorStream.start();
	if (rc != openni::STATUS_OK)
	{
		console() << "Couldn't start color stream " << openni::OpenNI::getExtendedError() << endl;
	}

	if (device.getSensorInfo(openni::SENSOR_DEPTH) != NULL)
	{
		rc = mDepthStream.create(device, openni::SENSOR_DEPTH);
		if (rc != openni::STATUS_OK)
		{
			console() << "Couldn't create depth stream " << openni::OpenNI::getExtendedError() << endl;
		}
	}
	rc = mDepthStream.start();
	if (rc != openni::STATUS_OK)
	{
		console() << "Couldn't start depth stream " << openni::OpenNI::getExtendedError() << endl;
	}

	openni::VideoMode depthVideoMode;
	openni::VideoMode colorVideoMode;

	if (mDepthStream.isValid() && mColorStream.isValid())
	{

		mDepthStream.addListener(this);
		mColorStream.addListener(this);

		depthVideoMode = mDepthStream.getVideoMode();
		colorVideoMode = mColorStream.getVideoMode();

		int depthWidth = depthVideoMode.getResolutionX();
		int depthHeight = depthVideoMode.getResolutionY();
		int colorWidth = colorVideoMode.getResolutionX();
		int colorHeight = colorVideoMode.getResolutionY();

		depthTex = gl::Texture(depthWidth, depthHeight);
		colorTex = gl::Texture(colorWidth, colorHeight);

	}
	else if (mDepthStream.isValid())
	{
		depthVideoMode = mDepthStream.getVideoMode();
		
		depthTex = gl::Texture(depthVideoMode.getResolutionX(), depthVideoMode.getResolutionY());

		mDepthStream.addListener(this);

	}
	else if (mColorStream.isValid())
	{
		colorVideoMode = mColorStream.getVideoMode();
		colorTex = gl::Texture(colorVideoMode.getResolutionX(), colorVideoMode.getResolutionY());

		mColorStream.addListener(this);
	}
	else
	{
		console() << "Hey, we need one of these streams to be valid " << endl;
	}
}

void oNIViewrApp::mouseDown( MouseEvent event )
{
}

void oNIViewrApp::onNewFrame(openni::VideoStream& stream)
{
	if(stream.getVideoMode().getPixelFormat() == openni::PIXEL_FORMAT_RGB888) { // this is the color stream
		stream.readFrame(&mRGBFrame);
		colorTex = gl::Texture( fromOpenNI( mRGBFrame ));
	}

	if(stream.getVideoMode().getPixelFormat() == openni::PIXEL_FORMAT_DEPTH_1_MM) { // this is the depth stream
		stream.readFrame(&mDFrame);
		depthTex = gl::Texture( fromOpenNI( mDFrame ));
	}

}

void oNIViewrApp::update()
{
}

void oNIViewrApp::draw()
{
	// clear out the window with black
	gl::clear( Color( 0, 0, 0 ) ); 
	gl::pushMatrices();
	gl::draw( colorTex );
	gl::translate( Vec2f( (float) colorTex.getWidth(), 0));
	gl::draw( depthTex );
	gl::popMatrices();
}

CINDER_APP_BASIC( oNIViewrApp, RendererGl )
