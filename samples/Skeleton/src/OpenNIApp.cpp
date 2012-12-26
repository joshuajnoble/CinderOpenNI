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
    
    bool calibratedUser;
    
	openni::Device mDevice;
	nite::UserTracker* mTracker;

	bool mVisibleUsers[10];
	nite::SkeletonState mSkeletonStates[10];

	gl::Texture colorTex, depthTex;

};

void OpenNIApp::setup()
{
    
	for( int i = 0; i < 10; i++) { mSkeletonStates[i] = {nite::SKELETON_NONE}; }

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
		console() << "Failed to open device ", openni::OpenNI::getExtendedError());
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
    
	nite::UserTrackerFrameRef userTrackerFrame;
	openni::VideoFrameRef depthFrame;
	nite::Status rc = mTracker->readFrame(&userTrackerFrame);
	if (rc != nite::STATUS_OK)
	{
		console() << "Get Data from user tracker failed " << endl;
		return;
	}

	depthFrame = userTrackerFrame.getDepthFrame();

    const nite::Array<nite::UserData>& users = userTrackerFrame.getUsers();
	for (int i = 0; i < users.getSize(); ++i)
	{
		const nite::UserData& user = users[i];

		//updateUserState(user, userTrackerFrame.getTimestamp());

	if (user.isNew()) {
		console() << "New");
	} else if (user.isVisible() && !mVisibleUsers[user.getId()]) {
		console() << "User " << user.getId() << " visible at " << userTrackerFrame.getTimestamp() << endl;
	} else if (!user.isVisible() && mVisibleUsers[user.getId()]) {
		console() << "User " << user.getId() << " visible at " << userTrackerFrame.getTimestamp() << endl;
	} else if (user.isLost()) {
		console() << "Lost");
	}
	mVisibleUsers[user.getId()] = user.isVisible();


	if(mSkeletonStates[user.getId()] != user.getSkeleton().getState())
	{
		switch(mSkeletonStates[user.getId()] = user.getSkeleton().getState())
		{
		case nite::SKELETON_NONE:
			console() << "Stopped tracking.")
			break;
		case nite::SKELETON_CALIBRATING:
			console() << "Calibrating...")
			break;
		case nite::SKELETON_TRACKED:
			console() << "Tracking!")
			break;
		case nite::SKELETON_CALIBRATION_ERROR_NOT_IN_POSE:
		case nite::SKELETON_CALIBRATION_ERROR_HANDS:
		case nite::SKELETON_CALIBRATION_ERROR_LEGS:
		case nite::SKELETON_CALIBRATION_ERROR_HEAD:
		case nite::SKELETON_CALIBRATION_ERROR_TORSO:
			console() << "Calibration Failed... :-|" << endl;
			break;
		}
	}

		if (user.isNew())
		{
			mTracker->startSkeletonTracking(user.getId());
			mTracker->startPoseDetection(user.getId(), nite::POSE_CROSSED_HANDS);
		}
		else if (!user.isLost())
		{
			if (users[i].getSkeleton().getState() == nite::SKELETON_TRACKED)
			{
				DrawSkeleton(mTracker, user);
			}
		}

		if (m_poseUser == 0 || m_poseUser == user.getId())
		{
			const nite::PoseData& pose = user.getPose(nite::POSE_CROSSED_HANDS);

			if (pose.isEntered())
			{
				// Start timer
				sprintf(g_generalMessage, "In exit pose. Keep it for %d second%s to exit\n", g_poseTimeoutToExit/1000, g_poseTimeoutToExit/1000 == 1 ? "" : "s");
				printf("Counting down %d second to exit\n", g_poseTimeoutToExit/1000);
				m_poseUser = user.getId();
				m_poseTime = userTrackerFrame.getTimestamp();
			}
			else if (pose.isExited())
			{
				memset(g_generalMessage, 0, sizeof(g_generalMessage));
				printf("Count-down interrupted\n");
				m_poseTime = 0;
				m_poseUser = 0;
			}
			else if (pose.isHeld())
			{
				// tick
				if (userTrackerFrame.getTimestamp() - m_poseTime > g_poseTimeoutToExit * 1000)
				{
					printf("Count down complete. Exit...\n");
					Finalize();
					exit(2);
				}
			}
		}
	}

    
}

void OpenNIApp::DrawSkeleton(nite::UserTracker* pUserTracker, const nite::UserData& userData)
{

	float coordinates[6] = {0};
	pUserTracker->convertJointCoordinatesToDepth(joint1.getPosition().x, joint1.getPosition().y, joint1.getPosition().z, &coordinates[0], &coordinates[1]);
	pUserTracker->convertJointCoordinatesToDepth(joint2.getPosition().x, joint2.getPosition().y, joint2.getPosition().z, &coordinates[3], &coordinates[4]);

	coordinates[0] *= GL_WIN_SIZE_X/g_nXRes;
	coordinates[1] *= GL_WIN_SIZE_Y/g_nYRes;
	coordinates[3] *= GL_WIN_SIZE_X/g_nXRes;
	coordinates[4] *= GL_WIN_SIZE_Y/g_nYRes;

	if (joint1.getPositionConfidence() == 1 && joint2.getPositionConfidence() == 1)
	{
		glColor3f(1.0f - Colors[color][0], 1.0f - Colors[color][1], 1.0f - Colors[color][2]);
	}
	else if (joint1.getPositionConfidence() < 0.5f || joint2.getPositionConfidence() < 0.5f)
	{
		return;
	}
	else
	{
		glColor3f(.5, .5, .5);
	}


	DrawLimb(pUserTracker, userData.getSkeleton().getJoint(nite::JOINT_HEAD), userData.getSkeleton().getJoint(nite::JOINT_NECK), userData.getId() % colorCount);

	DrawLimb(pUserTracker, userData.getSkeleton().getJoint(nite::JOINT_LEFT_SHOULDER), userData.getSkeleton().getJoint(nite::JOINT_LEFT_ELBOW), userData.getId() % colorCount);
	DrawLimb(pUserTracker, userData.getSkeleton().getJoint(nite::JOINT_LEFT_ELBOW), userData.getSkeleton().getJoint(nite::JOINT_LEFT_HAND), userData.getId() % colorCount);

	DrawLimb(pUserTracker, userData.getSkeleton().getJoint(nite::JOINT_RIGHT_SHOULDER), userData.getSkeleton().getJoint(nite::JOINT_RIGHT_ELBOW), userData.getId() % colorCount);
	DrawLimb(pUserTracker, userData.getSkeleton().getJoint(nite::JOINT_RIGHT_ELBOW), userData.getSkeleton().getJoint(nite::JOINT_RIGHT_HAND), userData.getId() % colorCount);

	DrawLimb(pUserTracker, userData.getSkeleton().getJoint(nite::JOINT_LEFT_SHOULDER), userData.getSkeleton().getJoint(nite::JOINT_RIGHT_SHOULDER), userData.getId() % colorCount);

	DrawLimb(pUserTracker, userData.getSkeleton().getJoint(nite::JOINT_LEFT_SHOULDER), userData.getSkeleton().getJoint(nite::JOINT_TORSO), userData.getId() % colorCount);
	DrawLimb(pUserTracker, userData.getSkeleton().getJoint(nite::JOINT_RIGHT_SHOULDER), userData.getSkeleton().getJoint(nite::JOINT_TORSO), userData.getId() % colorCount);

	DrawLimb(pUserTracker, userData.getSkeleton().getJoint(nite::JOINT_TORSO), userData.getSkeleton().getJoint(nite::JOINT_LEFT_HIP), userData.getId() % colorCount);
	DrawLimb(pUserTracker, userData.getSkeleton().getJoint(nite::JOINT_TORSO), userData.getSkeleton().getJoint(nite::JOINT_RIGHT_HIP), userData.getId() % colorCount);

	DrawLimb(pUserTracker, userData.getSkeleton().getJoint(nite::JOINT_LEFT_HIP), userData.getSkeleton().getJoint(nite::JOINT_RIGHT_HIP), userData.getId() % colorCount);


	DrawLimb(pUserTracker, userData.getSkeleton().getJoint(nite::JOINT_LEFT_HIP), userData.getSkeleton().getJoint(nite::JOINT_LEFT_KNEE), userData.getId() % colorCount);
	DrawLimb(pUserTracker, userData.getSkeleton().getJoint(nite::JOINT_LEFT_KNEE), userData.getSkeleton().getJoint(nite::JOINT_LEFT_FOOT), userData.getId() % colorCount);

	DrawLimb(pUserTracker, userData.getSkeleton().getJoint(nite::JOINT_RIGHT_HIP), userData.getSkeleton().getJoint(nite::JOINT_RIGHT_KNEE), userData.getId() % colorCount);
	DrawLimb(pUserTracker, userData.getSkeleton().getJoint(nite::JOINT_RIGHT_KNEE), userData.getSkeleton().getJoint(nite::JOINT_RIGHT_FOOT), userData.getId() % colorCount);
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
    
    //gl::draw( depth );
    gl::draw( color );
}



CINDER_APP_BASIC( OpenNIApp, RendererGl )
