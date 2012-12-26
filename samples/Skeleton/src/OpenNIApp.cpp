#include "cinder/app/AppBasic.h"
#include "cinder/gl/gl.h"
#include "cinder/gl/Texture.h"

#define USING_NITE
#include <ciOpenNI.h>

using namespace ci;
using namespace std;
using namespace ci::app;

class OpenNIApp : public AppBasic {
  public:

	void setup();
	void mouseDown( MouseEvent event );	
	void update();
	void draw();
	void exit();

	// openNI drawing routines
	void drawSkeleton(const nite::UserData& userData);
	void drawLimb(const nite::SkeletonJoint& joint1, const nite::SkeletonJoint& joint2, Color& color);
    

	/////////////////////////////////////////////////////////////////////////////////////////////////
	openni::Device mDevice;
	nite::UserTracker* mTracker;
	nite::UserTrackerFrameRef userTrackerFrame;

	bool mVisibleUsers[10];
	nite::SkeletonState mSkeletonStates[10];

	int exitPoseTimeout; // this will be how long we want the user to make the Exit pose (hands crossed in this case) in mS
	uint64_t exitPoseUserTimestamp; //  this will be when they started making that
	int exitPoseUserId; // the user that started making the pose

	int depthX, depthY;

	gl::Texture colorTex, depthTex;

	int mUserCount;
	float mColorCounter;

};

void OpenNIApp::setup()
{
    
	for( int i = 0; i < 10; i++) { mSkeletonStates[i] = nite::SKELETON_NONE; }

	openni::Status rc = openni::OpenNI::initialize();
	if (rc != openni::STATUS_OK)
	{
		console() << "Failed to initialize OpenNI " << openni::OpenNI::getExtendedError() << endl;
		return;
	}

	const char* deviceUri = openni::ANY_DEVICE;
	rc = mDevice.open(deviceUri);

	if (rc != openni::STATUS_OK)
	{
		console() << "Failed to open device " << openni::OpenNI::getExtendedError() << endl;
		return;
	}

	nite::NiTE::initialize();

	if (mTracker->create(&mDevice) != nite::STATUS_OK)
	{
		console() << " Status error " <<  openni::STATUS_ERROR << endl;
		return;
	}

    gl::Texture::Format format;
    
    colorTex = gl::Texture(640, 480, format);
    depthTex = gl::Texture(320, 240, format);
    
}

void OpenNIApp::mouseDown( MouseEvent event )
{
}

void OpenNIApp::update()
{
	openni::VideoFrameRef depthFrame;
	nite::Status rc = mTracker->readFrame(&userTrackerFrame);
	if (rc != nite::STATUS_OK) {
		console() << "Get Data from user tracker failed " << endl;
		return;
	}

	// we have a depth frame
	depthFrame = userTrackerFrame.getDepthFrame();

    const nite::Array<nite::UserData>& users = userTrackerFrame.getUsers();

	mUserCount = userTrackerFrame.getUsers().getSize();

	for (int i = 0; i < userTrackerFrame.getUsers().getSize(); ++i)
	{
		const nite::UserData& user = userTrackerFrame.getUsers()[i];

		//updateUserState(user, userTrackerFrame.getTimestamp());

		if (user.isNew()) {
			console() << "New" << endl;
		} else if (user.isVisible() && !mVisibleUsers[user.getId()]) {
			console() << "User " << user.getId() << " visible at " << userTrackerFrame.getTimestamp() << endl;
		} else if (!user.isVisible() && mVisibleUsers[user.getId()]) {
			console() << "User " << user.getId() << " visible at " << userTrackerFrame.getTimestamp() << endl;
		} else if (user.isLost()) {
			console() << "Lost" << endl;
		}

		mVisibleUsers[user.getId()] = user.isVisible();


		if(mSkeletonStates[user.getId()] != user.getSkeleton().getState())
		{
			switch(mSkeletonStates[user.getId()] = user.getSkeleton().getState())
			{
			case nite::SKELETON_NONE:
				console() << "Stopped tracking." << endl;
				break;
			case nite::SKELETON_CALIBRATING:
				console() << "Calibrating..." << endl;
				break;
			case nite::SKELETON_TRACKED:
				console() << "Tracking!" << endl;
				break;
			case nite::SKELETON_CALIBRATION_ERROR_NOT_IN_POSE:
			case nite::SKELETON_CALIBRATION_ERROR_HANDS:
			case nite::SKELETON_CALIBRATION_ERROR_LEGS:
			case nite::SKELETON_CALIBRATION_ERROR_HEAD:
			case nite::SKELETON_CALIBRATION_ERROR_TORSO:
				console() << " OPENNI :: Calibration Failed " << endl;
				break;
			}
		}

		if (user.isNew())
		{
			mTracker->startSkeletonTracking(user.getId());
			mTracker->startPoseDetection(user.getId(), nite::POSE_CROSSED_HANDS);
		}

		// this allows a user to exit the application by making the crossed hands pose
		// reference OpenNI
		if (exitPoseUserId == 0 || exitPoseUserId == user.getId())
		{
			const nite::PoseData& pose = user.getPose(nite::POSE_CROSSED_HANDS);

			if (pose.isEntered())
			{
				// Start timer
				console() << "In exit pose. Keep it for " << exitPoseTimeout << " seconds to exit " << endl;
				console() << "Counting down " << exitPoseTimeout << " seconds to exit " << endl;

				exitPoseUserId = user.getId();
				exitPoseUserTimestamp = userTrackerFrame.getTimestamp();
			}
			else if (pose.isExited())
			{
				console() << " Count-down interrupted, we're not going to exit any more " << endl;
				exitPoseUserTimestamp = 0;
				exitPoseUserId = 0;
			}
			else if (pose.isHeld())
			{
				// tick
				if (userTrackerFrame.getTimestamp() - exitPoseUserTimestamp > exitPoseTimeout) // have we been doing this long enough
				{
					console() << " We're out people." << endl;
					exit();
					quit();
				}
			}
		}
	}
}

void OpenNIApp::drawSkeleton(const nite::UserData& userData)
{

	Color userColor(CM_HSV, mColorCounter, 1.f, 1.f );
	mColorCounter += 1.f / (float) mUserCount;

	drawLimb( userData.getSkeleton().getJoint(nite::JOINT_HEAD), userData.getSkeleton().getJoint(nite::JOINT_NECK), userColor);
	drawLimb( userData.getSkeleton().getJoint(nite::JOINT_LEFT_SHOULDER), userData.getSkeleton().getJoint(nite::JOINT_LEFT_ELBOW), userColor);
	drawLimb( userData.getSkeleton().getJoint(nite::JOINT_LEFT_ELBOW), userData.getSkeleton().getJoint(nite::JOINT_LEFT_HAND), userColor);
	drawLimb( userData.getSkeleton().getJoint(nite::JOINT_RIGHT_SHOULDER), userData.getSkeleton().getJoint(nite::JOINT_RIGHT_ELBOW), userColor);
	drawLimb( userData.getSkeleton().getJoint(nite::JOINT_RIGHT_ELBOW), userData.getSkeleton().getJoint(nite::JOINT_RIGHT_HAND), userColor);
	drawLimb( userData.getSkeleton().getJoint(nite::JOINT_LEFT_SHOULDER), userData.getSkeleton().getJoint(nite::JOINT_RIGHT_SHOULDER), userColor);
	drawLimb( userData.getSkeleton().getJoint(nite::JOINT_LEFT_SHOULDER), userData.getSkeleton().getJoint(nite::JOINT_TORSO), userColor);
	drawLimb( userData.getSkeleton().getJoint(nite::JOINT_RIGHT_SHOULDER), userData.getSkeleton().getJoint(nite::JOINT_TORSO), userColor);
	drawLimb( userData.getSkeleton().getJoint(nite::JOINT_TORSO), userData.getSkeleton().getJoint(nite::JOINT_LEFT_HIP), userColor);
	drawLimb( userData.getSkeleton().getJoint(nite::JOINT_TORSO), userData.getSkeleton().getJoint(nite::JOINT_RIGHT_HIP), userColor);
	drawLimb( userData.getSkeleton().getJoint(nite::JOINT_LEFT_HIP), userData.getSkeleton().getJoint(nite::JOINT_RIGHT_HIP), userColor);
	drawLimb( userData.getSkeleton().getJoint(nite::JOINT_LEFT_HIP), userData.getSkeleton().getJoint(nite::JOINT_LEFT_KNEE), userColor);
	drawLimb( userData.getSkeleton().getJoint(nite::JOINT_LEFT_KNEE), userData.getSkeleton().getJoint(nite::JOINT_LEFT_FOOT), userColor);
	drawLimb( userData.getSkeleton().getJoint(nite::JOINT_RIGHT_HIP), userData.getSkeleton().getJoint(nite::JOINT_RIGHT_KNEE), userColor);
	drawLimb( userData.getSkeleton().getJoint(nite::JOINT_RIGHT_KNEE), userData.getSkeleton().getJoint(nite::JOINT_RIGHT_FOOT), userColor);
}

void OpenNIApp::drawLimb(const nite::SkeletonJoint& joint1, const nite::SkeletonJoint& joint2, Color& color)
{	
	float coordinates[6] = {0};
	mTracker->convertJointCoordinatesToDepth(joint1.getPosition().x, joint1.getPosition().y, joint1.getPosition().z, &coordinates[0], &coordinates[1]);
	mTracker->convertJointCoordinatesToDepth(joint2.getPosition().x, joint2.getPosition().y, joint2.getPosition().z, &coordinates[3], &coordinates[4]);

	coordinates[0] *= getWindowWidth()/depthX;
	coordinates[1] *= getWindowHeight()/depthY;
	coordinates[3] *= getWindowWidth()/depthX;
	coordinates[4] *= getWindowHeight()/depthY;

	if (joint1.getPositionConfidence() == 1 && joint2.getPositionConfidence() == 1) {
		// make better colors
		gl::color(color);
	} else if (joint1.getPositionConfidence() < 0.5f || joint2.getPositionConfidence() < 0.5f) {
		return;
	} else {
		gl::color(Color(.5, .5, .5));
	}

	gl::drawLine( Vec3f(joint1.getPosition().x, joint1.getPosition().y, joint1.getPosition().z), Vec3f(joint2.getPosition().x, joint2.getPosition().y, joint2.getPosition().z));
}

void OpenNIApp::exit()
{
	delete mTracker;
	nite::NiTE::shutdown();
	openni::OpenNI::shutdown();
}

void OpenNIApp::draw()
{
	// clear out the window with black
	gl::clear( Color( 0, 0, 0 ) ); 
    gl::draw( depthTex );
	mColorCounter = 0.f;

	// draw all the users
	for(int i = 0; i < userTrackerFrame.getUsers().getSize(); i++) {

		if (!userTrackerFrame.getUsers()[i].isLost())
		{
			if (userTrackerFrame.getUsers()[i].getSkeleton().getState() == nite::SKELETON_TRACKED)
			{
				drawSkeleton(userTrackerFrame.getUsers()[i]);
			}
		}

	}

}



CINDER_APP_BASIC( OpenNIApp, RendererGl )
