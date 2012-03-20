#include "cinder/app/AppBasic.h"
#include "cinder/gl/gl.h"

//#include "OpenNI.h"

using namespace ci;
using namespace ci::app;
using namespace std;

class OpenNIHandsApp : public AppBasic {
    
    public:
	void setup();
	void mouseDown( MouseEvent event );	
	void update();
	void draw();
    
    xn::Context m_context;
    xn::ScriptNode m_scriptNode;
    xn::DepthGenerator m_depth;
    xn::ImageGenerator m_image;
    
	xn::GestureGenerator	mGestureGenerator;
	xn::HandsGenerator		mHandsGenerator;
    
    xn::DepthMetaData m_depthMD;
    xn::ImageMetaData m_imageMD;
    
    void gestureRecognized(xn::GestureGenerator& generator, const XnChar* strGesture, const XnPoint3D*	pIDPosition, const XnPoint3D*	pEndPosition);
	void gestureProcess( xn::GestureGenerator& generator, const XnChar* strGesture, const XnPoint3D* pPosition, XnFloat fProgress);
	void handCreate( xn::HandsGenerator& generator, XnUserID nId, const XnPoint3D* pPosition, XnFloat fTime );
	void handUpdate( xn::HandsGenerator& generator, XnUserID nId, const XnPoint3D* pPosition, XnFloat fTime );
	void handDestroy( xn::HandsGenerator& generator, XnUserID nId, XnFloat fTime );
    
    XnCallbackHandle handle;
    
	// OpenNI Gesture and Hands Generator callbacks
	static void XN_CALLBACK_TYPE Gesture_Recognized(xn::GestureGenerator& generator, const XnChar* strGesture, const XnPoint3D* pIDPosition, const XnPoint3D* pEndPosition, void* pCookie) 
    {
        OpenNIHandsApp *app = static_cast<OpenNIHandsApp*>(pCookie);
        app->gestureRecognized( generator, strGesture, pIDPosition, pEndPosition );
    }
	
    static void XN_CALLBACK_TYPE Gesture_Process(xn::GestureGenerator& generator, const XnChar* strGesture, const XnPoint3D* pPosition, XnFloat fProgress, void* pCookie) 
    {
        OpenNIHandsApp *app = static_cast<OpenNIHandsApp*>(pCookie);
        app->gestureProcess( generator, strGesture, pPosition, fProgress );
    }
	
    static void XN_CALLBACK_TYPE Hand_Create( xn::HandsGenerator& generator, XnUserID nId,const XnPoint3D* pPosition, XnFloat	fTime, void* pCookie) 
    {
        OpenNIHandsApp *app = static_cast<OpenNIHandsApp*>(pCookie);
        app->handCreate( generator, nId, pPosition, fTime );
    }
	
    static void XN_CALLBACK_TYPE Hand_Update( xn::HandsGenerator& generator, XnUserID nId, const XnPoint3D* pPosition, XnFloat fTime, void* pCookie) 
    {
        OpenNIHandsApp *app = static_cast<OpenNIHandsApp*>(pCookie);
        app->handUpdate( generator, nId, pPosition, fTime );
    }
	
    static void XN_CALLBACK_TYPE Hand_Destroy( xn::HandsGenerator& generator, XnUserID nId, XnFloat fTime, void* pCookie) 
    {
        OpenNIHandsApp *app = static_cast<OpenNIHandsApp*>(pCookie);
        app->handDestroy( generator, nId, fTime );
    }
    
};

void OpenNIHandsApp::setup()
{
   calibratedUser = false;
    strPose = "";
    
    string str = getResourcePath() + "/ONIConfig.xml";
    
    xn::EnumerationErrors errors;
    
    XnStatus ret = XN_STATUS_OK;
    
    ret = mContext.InitFromXmlFile(str.c_str(), mScriptNode, &errors);
   
    if (ret != XN_STATUS_OK) { console() << " can't make context "; }
    
    ret = mContext.FindExistingNode(XN_NODE_TYPE_DEPTH, mDepth);
    
    if (ret != XN_STATUS_OK) { console() << " can't make depth "; }
    
    ret = mContext.FindExistingNode(XN_NODE_TYPE_IMAGE, mImage);
    
    if (ret != XN_STATUS_OK) { console() << " can't make image "; }

    ret = mGestureGenerator.Create(mContext);
    
    if (ret != XN_STATUS_OK) { console() << " can't make image "; }

    ret = mHandsGenerator.Create(mContext);
    
    if (ret != XN_STATUS_OK) { console() << " can't make image "; }
    
    ret = mContext.StartGeneratingAll();

    if (ret != XN_STATUS_OK) { console() << " can't start generating "; }
    
    mUserGenerator.RegisterUserCallbacks(&OpenNIApp::User_NewUser, &OpenNIApp::User_LostUser, this, hUserCallbacks);
    mUserGenerator.GetSkeletonCap().RegisterToCalibrationStart(&OpenNIApp::UserCalibration_CalibrationStart, this, hCalibrationStart);
    mUserGenerator.GetSkeletonCap().RegisterToCalibrationComplete(&OpenNIApp::UserCalibration_CalibrationComplete, this, hCalibrationComplete);
    
    // first the simple hand callbacks
    mHandsGenerator.RegisterHandCallbacks(Hand_Create, Hand_Update, Hand_Destroy, this, chandle);

    // now the gestural ones
    mGestureGenerator.RegisterGestureCallbacks(Gesture_Recognized, Gesture_Process, this, chandle);
    mGestureGenerator.AddGesture("click", this);
    mGestureGenerator.AddGesture("wave", this);

    gl::Texture::Format format;
    
    color = gl::Texture(640, 480, format);
    depth = gl::Texture(320, 240, format);
}

void OpenNIHandsApp::mouseDown( MouseEvent event )
{
}

void OpenNIHandsApp::update()
{
}

void OpenNIHandsApp::draw()
{
	// clear out the window with black
	gl::clear( Color( 0, 0, 0 ) ); 
}

//---------------------------------------------------------------------------
// Hooks
//---------------------------------------------------------------------------

/*void OpenNIHandsApp::gestureRecognized(xn::GestureGenerator& generator, const XnChar* strGesture, const XnPoint3D*	pIDPosition, const XnPoint3D*	pEndPosition) {
    console() << strGesture;
}

void OpenNIHandsApp::gestureProcess( xn::GestureGenerator& generator, const XnChar* strGesture, const XnPoint3D* pPosition, XnFloat fProgress) {
    console() << strGesture;
}

void OpenNIHandsApp::handCreate( xn::HandsGenerator& generator, XnUserID nId, const XnPoint3D* pPosition, XnFloat fTime ) {
    console() << " created " << nId << " " << pPosition->X << " " << pPosition->Y << " " << pPosition->Z;
}

void OpenNIHandsApp::handUpdate( xn::HandsGenerator& generator, XnUserID nId, const XnPoint3D* pPosition, XnFloat fTime ) {
    console() << " updated " << nId << " " << pPosition->X << " " << pPosition->Y << " " << pPosition->Z;
}

void OpenNIHandsApp::handDestroy( xn::HandsGenerator& generator, XnUserID nId, XnFloat fTime ) {
    console() << " destroyed " << nId << " " << fTime;
}*/


CINDER_APP_BASIC( OpenNIHandsApp, RendererGl )
