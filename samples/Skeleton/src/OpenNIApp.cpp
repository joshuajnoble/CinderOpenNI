#include "cinder/app/AppBasic.h"
#include "cinder/gl/gl.h"

#include "OpenNI.h"

using namespace ci;
using namespace ci::app;
using namespace std;

#define SAMPLE_XML_PATH "ONIConfig.xml"
#define MAX_NUM_USERS 2

class OpenNIApp : public AppBasic {
  public:
	void setup();
	void mouseDown( MouseEvent event );	
	void update();
	void draw();
    
    XnBool mNeedPose;
    string strPose;
    
    XnCallbackHandle hUserCallbacks, hCalibrationStart, hCalibrationComplete, hPoseDetected, hCalibrationInProgress, hPoseInProgress;
    
    xn::Context mContext;
    xn::ScriptNode mScriptNode;
    xn::DepthGenerator mDepth;
    xn::ImageGenerator mImage;
    xn::UserGenerator mUserGenerator;
    
    xn::DepthMetaData mdepthMD;
    xn::ImageMetaData mimageMD;
    
    gl::Texture color, depth;
    
    bool calibratedUser;
    
    // OpenNI callbacks
    // ---------------------------------------------------------------------------------
    
    // Callback: New user was detected
    static void XN_CALLBACK_TYPE User_NewUser(xn::UserGenerator& generator, XnUserID nId, void* pCookie) {
        OpenNIApp *app = static_cast<OpenNIApp*>(pCookie);
        app->newUser(generator, nId);
    }
    
    // Callback: An existing user was lost
    static void XN_CALLBACK_TYPE User_LostUser(xn::UserGenerator& generator, XnUserID nId, void* pCookie) {
        OpenNIApp *app = static_cast<OpenNIApp*>(pCookie);
        app->lostUser(generator, nId);
    }
    
    // Callback: Detected a pose
    static void XN_CALLBACK_TYPE UserPose_PoseDetected(xn::PoseDetectionCapability& capability, const XnChar* strPose, XnUserID nId, void* pCookie) {
        OpenNIApp *app = static_cast<OpenNIApp*>(pCookie);
        app->poseDetected(capability, strPose, nId);
    }
    
    // Callback: Started calibration
    static void XN_CALLBACK_TYPE UserCalibration_CalibrationStart(xn::SkeletonCapability& capability, XnUserID nId, void* pCookie) {
        OpenNIApp *app = static_cast<OpenNIApp*>(pCookie);
        app->calibrationStart(capability, nId);
    }
    
    // Callback: Calibration Complete
    static void XN_CALLBACK_TYPE UserCalibration_CalibrationComplete(xn::SkeletonCapability& capability, XnUserID nId, XnCalibrationStatus eStatus, void* pCookie) {
        
        OpenNIApp *app = static_cast<OpenNIApp*>(pCookie);
        app->calibrationComplete(capability, nId, eStatus);
    }

    void newUser(xn::UserGenerator& generator, XnUserID nId);
    void lostUser(xn::UserGenerator& generator, XnUserID nId);
    void poseDetected(xn::PoseDetectionCapability& capability, const XnChar* strPose, XnUserID nId);
    void calibrationStart(xn::SkeletonCapability& capability, XnUserID nId);
    void calibrationComplete(xn::SkeletonCapability& capability, XnUserID nId, XnCalibrationStatus eStatus);
    
};

void OpenNIApp::setup()
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
    
    ret = mUserGenerator.Create(mContext);
        
    if (ret != XN_STATUS_OK) { console() << " can't create generator "; }
    
    if (!mUserGenerator.IsCapabilitySupported(XN_CAPABILITY_SKELETON)) {
        console() << "Supplied user generator doesn't support skeleton ";
        return;
    } else {
        console() << " let's do skeletons ";
    }
    
    if (!mUserGenerator.IsCapabilitySupported(XN_CAPABILITY_POSE_DETECTION)) {
        console() << " can do poses ";
        return;
    }

    if (mUserGenerator.GetSkeletonCap().NeedPoseForCalibration())
    {
        XnChar requiredPose[20] = "";
        
        if (!mUserGenerator.IsCapabilitySupported(XN_CAPABILITY_POSE_DETECTION))
        {
            console() << "Pose required, but not supported ";
            return;
        }
        
        mNeedPose = true;
        
        ret = mUserGenerator.GetPoseDetectionCap().RegisterToPoseDetected(UserPose_PoseDetected, NULL, hPoseDetected);
        if (ret != XN_STATUS_OK) { console() << " gotta get a pose "; }
        mUserGenerator.GetSkeletonCap().GetCalibrationPose(requiredPose);
        console() << " required pose is " << requiredPose;
    }
    
    ret = mContext.StartGeneratingAll();
    if (ret != XN_STATUS_OK) { console() << " can't start generating "; }
    
    mUserGenerator.RegisterUserCallbacks(&OpenNIApp::User_NewUser, &OpenNIApp::User_LostUser, this, hUserCallbacks);
    mUserGenerator.GetSkeletonCap().RegisterToCalibrationStart(&OpenNIApp::UserCalibration_CalibrationStart, this, hCalibrationStart);
    mUserGenerator.GetSkeletonCap().RegisterToCalibrationComplete(&OpenNIApp::UserCalibration_CalibrationComplete, this, hCalibrationComplete);
    
    gl::Texture::Format format;
    
    color = gl::Texture(320, 240, format);
    depth = gl::Texture(320, 240, format);
    
}

void OpenNIApp::mouseDown( MouseEvent event )
{
}

void OpenNIApp::update()
{
    
    XnStatus ret = XN_STATUS_OK;
    ret = mContext.WaitAnyUpdateAll();
	if (ret != XN_STATUS_OK)
	{
        console() << " can't update ";
        return;
    }
    
    mDepth.GetMetaData(mdepthMD);
	mImage.GetMetaData(mimageMD);
    
    depth = Surface( fromOpenNI(mdepthMD) );
    color = Surface( fromOpenNI(mimageMD));
    
    XnUserID aUsers[MAX_NUM_USERS];
    XnUInt16 nUsers;
    XnSkeletonJointTransformation torsoJoint;
    
    // print the torso information for the first user already tracking
    mUserGenerator.GetUsers(aUsers, nUsers);
    int numTracked=0;
    int userToPrint=-1;
    for(XnUInt16 i=0; i<nUsers; i++)
    {
        if(mUserGenerator.GetSkeletonCap().IsTracking(aUsers[i])==FALSE)
            continue;
        
        mUserGenerator.GetSkeletonCap().GetSkeletonJoint(aUsers[i],XN_SKEL_TORSO,torsoJoint);
        console() << "user " << aUsers[i] << " head at " << torsoJoint.position.position.X << "  " << torsoJoint.position.position.Y << " " << torsoJoint.position.position.Z;
    }
    
    
}

void OpenNIApp::draw()
{
	// clear out the window with black
	gl::clear( Color( 0, 0, 0 ) ); 
    
    //gl::draw( depth );
    gl::draw( color );
}


////////////////////////////////////////////
/* 
 OpenNI callbacks
 You need to add these to your application to have it receive events from OpenNI
 I thought about making these something hidden that your application would tie into
 but on reflection, thought that was not entirely in the spirit of Cinder and that
 most people would want to do something of sufficient complexity that they would need
 to have these in their own application
*/
////////////////////////////////////////////

// local callbacks

void OpenNIApp::newUser(xn::UserGenerator& generator, XnUserID nId) {
    
    console() << " new user " << nId ;
    
    // New user found
    if (mNeedPose)
    {
        mUserGenerator.GetPoseDetectionCap().StartPoseDetection(strPose.c_str(), nId);
    }
    else
    {
        mUserGenerator.GetSkeletonCap().RequestCalibration(nId, TRUE);
    }

}

void OpenNIApp::lostUser(xn::UserGenerator& generator, XnUserID nId) {
    console() << " Lost user " << nId;	
}

void OpenNIApp::poseDetected(xn::PoseDetectionCapability& capability, const XnChar* strPose, XnUserID nId) {
    
    console() << " pose detected " << nId ;
    
    mUserGenerator.GetPoseDetectionCap().StopPoseDetection(nId);
    mUserGenerator.GetSkeletonCap().RequestCalibration(nId, TRUE);

}

void OpenNIApp::calibrationStart(xn::SkeletonCapability& capability, XnUserID nId) {
    console() << " Calibration started for user " << nId ;
}

void OpenNIApp::calibrationComplete(xn::SkeletonCapability& capability, XnUserID nId, XnCalibrationStatus eStatus) {
    
    if (eStatus == XN_CALIBRATION_STATUS_OK)
    {
        // Calibration succeeded
        console() << " Calibration complete, start tracking user " << nId;
        mUserGenerator.GetSkeletonCap().StartTracking(nId);
    }
    else
    {
        // Calibration failed
        //console() << " Calibration failed for user " << nId;
        if(eStatus==XN_CALIBRATION_STATUS_MANUAL_ABORT)
        {
            console() << "Manual abort occured, stop attempting to calibrate!";
            return;
        }
        if (mNeedPose)
        {
            mUserGenerator.GetPoseDetectionCap().StartPoseDetection(strPose.c_str(), nId);
        }
        else
        {
            mUserGenerator.GetSkeletonCap().RequestCalibration(nId, TRUE);
        }
    }
}



CINDER_APP_BASIC( OpenNIApp, RendererGl )
