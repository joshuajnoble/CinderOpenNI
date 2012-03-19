#include "cinder/app/AppBasic.h"
#include "cinder/gl/gl.h"

#include "OpenNI.h"

using namespace ci;
using namespace ci::app;
using namespace std;

#define SAMPLE_XML_PATH "../../../../Data/SamplesConfig.xml"
#define MAX_NUM_USERS 2


class OpenNIApp : public AppBasic {
  public:
	void setup();
	void mouseDown( MouseEvent event );	
	void update();
	void draw();
    
    // OpenNI callbacks
    
    static void User_NewUser(xn::UserGenerator& generator, XnUserID nId, void* pCookie);
    // Callback: An existing user was lost
    static void XN_CALLBACK_TYPE User_LostUser(xn::UserGenerator& generator, XnUserID nId, void* pCookie);
    // Callback: Detected a pose
    static void XN_CALLBACK_TYPE UserPose_PoseDetected(xn::PoseDetectionCapability& capability, const XnChar* strPose, XnUserID nId, void* pCookie);
    // Callback: Started calibration
    static void XN_CALLBACK_TYPE UserCalibration_CalibrationStart(xn::SkeletonCapability& capability, XnUserID nId, void* pCookie);
    static void XN_CALLBACK_TYPE UserCalibration_CalibrationComplete(xn::SkeletonCapability& capability, XnUserID nId, XnCalibrationStatus eStatus, void* pCookie);
    
    XnBool g_bNeedPose;
    string strPose;
    
    xn::Context g_context;
    xn::ScriptNode g_scriptNode;
    xn::DepthGenerator g_depth;
    xn::ImageGenerator g_image;
    xn::UserGenerator g_UserGenerator;
    
    xn::DepthMetaData g_depthMD;
    xn::ImageMetaData g_imageMD;
    
    gl::Texture color, depth;
    
    bool calibratedUser;
    
};

void OpenNIApp::setup()
{
    
    calibratedUser = false;
    strPose = "";
    
    //if(xn::OSDoesFileExist(fn, &exists)) {
    //    nRetVal = context.InitFromXmlFile(fn, scriptNode, &errors);
    //    nRetVal = g_UserGenerator.Create(g_Context);
    //}
    
    gl::Texture::Format format;
    
    color = gl::Texture(320, 240, format);
    depth = gl::Texture(320, 240, format);
    
}

void OpenNIApp::mouseDown( MouseEvent event )
{
}

void OpenNIApp::update()
{
    
    g_depth.GetMetaData(g_depthMD);
	g_image.GetMetaData(g_imageMD);
    
    depth.update(fromOpenNI(g_depthMD));
    color.update(fromOpenNI(g_imageMD));
    
    XnStatus nRetVal = XN_STATUS_OK;
    bool g_bNeedPose;
    
    if(!calibratedUser)
    {
        if (g_UserGenerator.GetSkeletonCap().NeedPoseForCalibration())
        {
            g_bNeedPose = TRUE;
            nRetVal = g_UserGenerator.GetPoseDetectionCap().RegisterToPoseDetected(  &OpenNIApp::UserPose_PoseDetected, this, hPoseDetected );
            
            // need this
            //g_UserGenerator.GetSkeletonCap().GetCalibrationPose(g_strPose);
        }
        
        g_UserGenerator.GetSkeletonCap().SetSkeletonProfile(XN_SKEL_PROFILE_ALL);
        
        nRetVal = g_context.StartGeneratingAll();
    }
    else
    {
        XnSkeletonJointTransformation torsoJoint;
        g_context.WaitOneUpdateAll(g_UserGenerator);
        XnUserID aUsers[MAX_NUM_USERS];
        XnUInt16 nUsers = MAX_NUM_USERS;
        g_UserGenerator.GetUsers(aUsers, nUsers);
        int numTracked=0;
        int userToPrint=-1;
        for(XnUInt16 i=0; i<nUsers; i++)
        {
            if(g_UserGenerator.GetSkeletonCap().IsTracking(aUsers[i])==FALSE)
                continue;
            
            g_UserGenerator.GetSkeletonCap().GetSkeletonJoint(aUsers[i],XN_SKEL_TORSO, torsoJoint);
        }
    }
    
    
}

void OpenNIApp::draw()
{
	// clear out the window with black
	gl::clear( Color( 0, 0, 0 ) ); 
}


////////////////////////////////////////////
// OpenNI callbacks
////////////////////////////////////////////

// Callback: New user was detected
void XN_CALLBACK_TYPE OpenNIApp::User_NewUser(xn::UserGenerator& generator, XnUserID nId, void* pCookie)
{
    OpenNIApp *app = static_cast<OpenNIApp*>(AppBasic::get());
    
    // New user found
    if (app->g_bNeedPose)
    {
        app->g_UserGenerator.GetPoseDetectionCap().StartPoseDetection(app->strPose.c_str(), nId);
    }
    else
    {
        app->g_UserGenerator.GetSkeletonCap().RequestCalibration(nId, TRUE);
    }
}
// Callback: An existing user was lost
void XN_CALLBACK_TYPE OpenNIApp::User_LostUser(xn::UserGenerator& generator, XnUserID nId, void* pCookie)
{
    //console() << " Lost user " << nId;	
}
// Callback: Detected a pose
void XN_CALLBACK_TYPE OpenNIApp::UserPose_PoseDetected(xn::PoseDetectionCapability& capability, const XnChar* strPose, XnUserID nId, void* pCookie)
{
    
    OpenNIApp *app = static_cast<OpenNIApp*>(AppBasic::get());
    app->g_UserGenerator.GetPoseDetectionCap().StopPoseDetection(nId);
    app->g_UserGenerator.GetSkeletonCap().RequestCalibration(nId, TRUE);
}
// Callback: Started calibration
void XN_CALLBACK_TYPE OpenNIApp::UserCalibration_CalibrationStart(xn::SkeletonCapability& capability, XnUserID nId, void* pCookie)
{
    
    //console() << " Calibration started for user " << nId ;
}

void XN_CALLBACK_TYPE OpenNIApp::UserCalibration_CalibrationComplete(xn::SkeletonCapability& capability, XnUserID nId, XnCalibrationStatus eStatus, void* pCookie)
{
    
    OpenNIApp *app = static_cast<OpenNIApp*>(AppBasic::get());
    
    
    if (eStatus == XN_CALIBRATION_STATUS_OK)
    {
        // Calibration succeeded
        app->console() << " Calibration complete, start tracking user " << nId;
        app->g_UserGenerator.GetSkeletonCap().StartTracking(nId);
    }
    else
    {
        // Calibration failed
        //console() << " Calibration failed for user " << nId;
        if(eStatus==XN_CALIBRATION_STATUS_MANUAL_ABORT)
        {
            app->console() << "Manual abort occured, stop attempting to calibrate!";
            return;
        }
        if (app->g_bNeedPose)
        {
            app->g_UserGenerator.GetPoseDetectionCap().StartPoseDetection(app->strPose.c_str(), nId);
        }
        else
        {
            app->g_UserGenerator.GetSkeletonCap().RequestCalibration(nId, TRUE);
        }
    }
}


CINDER_APP_BASIC( OpenNIApp, RendererGl )
