#include <uf/ext/openvr/openvr.h>
#include <uf/utils/io/iostream.h>

vr::IVRSystem* ext::openvr::context;
ext::openvr::Driver ext::openvr::driver;
uint8_t ext::openvr::renderPass = 0;
float ext::openvr::width = 0;
float ext::openvr::height = 0;


// toPort
namespace {
	std::string GetTrackedDeviceString(vr::IVRSystem *pHmd, vr::TrackedDeviceIndex_t unDevice, vr::TrackedDeviceProperty prop, vr::TrackedPropertyError *peError = NULL) {
		uint32_t requiredBufferLen = pHmd->GetStringTrackedDeviceProperty(unDevice, prop, NULL, 0, peError);
		if( requiredBufferLen == 0 ) return "";

		char *pchBuffer = new char[requiredBufferLen];
		requiredBufferLen = pHmd->GetStringTrackedDeviceProperty(unDevice,prop,pchBuffer,requiredBufferLen,peError);
		std::string sResult = pchBuffer;
		delete[] pchBuffer;

		return sResult;
	}

	std::string GetTrackedDeviceClassString(vr::ETrackedDeviceClass klass) {
		std::string str = "Unknown class";

		switch ( klass ) {
			case vr::TrackedDeviceClass_Invalid:			// = 0, the ID was not valid.
				str = "invalid";
			break;
			case vr::TrackedDeviceClass_HMD:				// = 1, Head-Mounted Displays
				str = "hmd";
			break;
			case vr::TrackedDeviceClass_Controller:			// = 2, Tracked controllers
				str = "controller";
			break;
			case vr::TrackedDeviceClass_GenericTracker:		// = 3, Generic trackers, similar to controllers
				str = "generic tracker";
			break;
			case vr::TrackedDeviceClass_TrackingReference:	// = 4, Camera and base stations that serve as tracking reference points
				str = "base station";
			break;
			case vr::TrackedDeviceClass_DisplayRedirect:	// = 5, Accessories that aren't necessarily tracked themselves, but may redirect video output from other tracked devices
				str = "display redirect";
			break;
		}

		return str;
	}

	pod::Matrix4t<> hmdPosition;
}

bool ext::openvr::initialize( int stage ) {
	if ( stage == 1 ) {
		if ( !vr::VRCompositor() ) {
			uf::iostream << "Error while initializing HMD: VRCompositor() failed" << "\n";
			return false;
		}
		return true;
	}
	if ( !vr::VR_IsHmdPresent() ) {
		uf::iostream << "Error while initializing HMD: not present" << "\n";
		return false;
	}
	if ( !vr::VR_IsRuntimeInstalled() ) {
		uf::iostream << "Error while initializing HMD: no runtime is installed" << "\n";
		return false;
	}
	
	int baseStations = 0;
	// const char* runtime_path = vr::VR_GetRuntimePath();
	vr::HmdError err;
	ext::openvr::context = vr::VR_Init( &err, vr::EVRApplicationType::VRApplication_Scene );
	if ( err != vr::VRInitError_None || ext::openvr::context == NULL ) {
		uf::iostream << "Error while initializing SteamVR runtime: " << vr::VR_GetVRInitErrorAsEnglishDescription(err) << "\n";
		return false;
	}

	// Get connected devices
	for ( uint32_t i = vr::k_unTrackedDeviceIndex_Hmd; i < vr::k_unMaxTrackedDeviceCount; ++i ) {
		if ( !ext::openvr::context->IsTrackedDeviceConnected(i) ) continue;
		vr::ETrackedDeviceClass trackedDeviceClass = ext::openvr::context->GetTrackedDeviceClass(i);
		std::string trackedDeviceType = ::GetTrackedDeviceClassString(trackedDeviceClass);
		ext::openvr::driver.types[i] = trackedDeviceType;

		uf::iostream << "Tracking device " << i << " is connected " << "\n";
		uf::iostream << "  Device type: " << trackedDeviceType << ". Name: " << ::GetTrackedDeviceString(ext::openvr::context, i, vr::Prop_TrackingSystemName_String) << "\n";

		if (trackedDeviceClass == vr::ETrackedDeviceClass::TrackedDeviceClass_TrackingReference)
			++baseStations;

		if (i == vr::k_unTrackedDeviceIndex_Hmd) {
			ext::openvr::driver.name = ::GetTrackedDeviceString(ext::openvr::context, vr::k_unTrackedDeviceIndex_Hmd, vr::Prop_TrackingSystemName_String);
			ext::openvr::driver.serial = ::GetTrackedDeviceString(ext::openvr::context, vr::k_unTrackedDeviceIndex_Hmd, vr::Prop_SerialNumber_String);
		}
	}

	// Get render models
	driver.renderModels = (vr::IVRRenderModels*) vr::VR_GetGenericInterface( vr::IVRRenderModels_Version, &err );
	if ( err != vr::VRInitError_None ) {
		uf::iostream << "Error while retrieving render model interface: " << vr::VR_GetVRInitErrorAsEnglishDescription(err) << "\n";
		return false;
	}

	uf::iostream << "HMD " << ext::openvr::driver.name << " " << ext::openvr::driver.serial << "\n";


	return true;
}
void ext::openvr::terminate() {
	vr::VR_Shutdown();
	ext::openvr::context = NULL;
}
void ext::openvr::tick() {
	if ( !ext::openvr::context ) return;
	vr::VREvent_t event;
	while ( ext::openvr::context->PollNextEvent(&event, sizeof(event)) ) {
		std::string trackingDeviceClassName = ::GetTrackedDeviceClassString(ext::openvr::context->GetTrackedDeviceClass(event.trackedDeviceIndex));
		switch( event.eventType ) {
			case vr::VREvent_TrackedDeviceActivated: {
				uf::iostream << "Device " << event.trackedDeviceIndex << " attached (" << trackingDeviceClassName << ")" << "\n";
				ext::openvr::driver.types[event.trackedDeviceIndex] = trackingDeviceClassName;
			}
			break;
			case vr::VREvent_TrackedDeviceDeactivated: {
				uf::iostream << "Device " << event.trackedDeviceIndex << " detached (" << trackingDeviceClassName << ")" << "\n";
				ext::openvr::driver.types[event.trackedDeviceIndex] = "";
			}
			break;
			case vr::VREvent_TrackedDeviceUpdated: {
				uf::iostream << "Device " << event.trackedDeviceIndex << " updated (" << trackingDeviceClassName << ")" << "\n";
			}
			break;
			case vr::VREvent_ButtonPress: {
				vr::VREvent_Controller_t controllerData = event.data.controller;
				uf::iostream << "Pressed button " << ext::openvr::context->GetButtonIdNameFromEnum((vr::EVRButtonId) controllerData.button) << " of device " << event.trackedDeviceIndex << " (" << trackingDeviceClassName << ")" << "\n";
			}
			break;
			case vr::VREvent_ButtonUnpress: {
				vr::VREvent_Controller_t controllerData = event.data.controller;
				uf::iostream << "Unpressed button " << ext::openvr::context->GetButtonIdNameFromEnum((vr::EVRButtonId) controllerData.button) << " of device " << event.trackedDeviceIndex << " (" << trackingDeviceClassName << ")" << "\n";
			}
			break;
			case vr::VREvent_ButtonTouch: {
				vr::VREvent_Controller_t controllerData = event.data.controller;
				uf::iostream << "Touched button " << ext::openvr::context->GetButtonIdNameFromEnum((vr::EVRButtonId) controllerData.button) << " of device " << event.trackedDeviceIndex << " (" << trackingDeviceClassName << ")" << "\n";
			}
			break;
			case vr::VREvent_ButtonUntouch: {
				vr::VREvent_Controller_t controllerData = event.data.controller;
				uf::iostream << "Untouched button " << ext::openvr::context->GetButtonIdNameFromEnum((vr::EVRButtonId) controllerData.button) << " of device " << event.trackedDeviceIndex << " (" << trackingDeviceClassName << ")" << "\n";
			}
			break;
		}
	}

	// Obtain tracking device poses
	{
		float displayFrequency = ext::openvr::context->GetFloatTrackedDeviceProperty(vr::k_unTrackedDeviceIndex_Hmd, vr::Prop_DisplayFrequency_Float);
		float frameDuration = 1.f / displayFrequency;
		float vsyncToPhotons = ext::openvr::context->GetFloatTrackedDeviceProperty(vr::k_unTrackedDeviceIndex_Hmd, vr::Prop_SecondsFromVsyncToPhotons_Float);
		float predictedDisplayTime = frameDuration + vsyncToPhotons;
		ext::openvr::context->GetDeviceToAbsoluteTrackingPose(
			vr::ETrackingUniverseOrigin::TrackingUniverseStanding,
			predictedDisplayTime,
			ext::openvr::driver.poses,
			vr::k_unMaxTrackedDeviceCount
		);
	}
/*
	int actualY = 110, trackedDevices = 0;
	for (int nDevice = 0; nDevice < vr::k_unMaxTrackedDeviceCount; ++nDevice ) {
		if ((ext::openvr::driver.poses[nDevice].bDeviceIsConnected) && (ext::openvr::driver.poses[nDevice].bPoseIsValid)) {
			// Check whether the tracked device is a controller. If so, set text color based on the trigger button state
			vr::VRControllerState_t controller_state;
			if (ext::openvr::context->GetControllerState(nDevice,&controller_state,sizeof(controller_state)))
				((vr::ButtonMaskFromId(vr::EVRButtonId::k_EButton_Axis1) & controller_state.ulButtonPressed) == 0) ? color = green : color = blue;

			print_text(("Tracked device #" + ftos((float) nDevice,0) + " (" + tracked_device_type[nDevice] + ")").c_str(), color, 10, actualY);

			// We take just the translation part of the matrix (actual position of tracked device, not orientation)
			float v[3] = {
				ext::openvr::driver.poses[nDevice].mDeviceToAbsoluteTracking.m[0][3],
				ext::openvr::driver.poses[nDevice].mDeviceToAbsoluteTracking.m[1][3],
				ext::openvr::driver.poses[nDevice].mDeviceToAbsoluteTracking.m[2][3]
			};
			trackedDevices++;
		}
	}
	print_text(("Tracked devices: " + ftos((float) trackedDevices,0)).c_str(), black, 10, 70);
*/
	
	vr::VRCompositor()->WaitGetPoses(&driver.poses[0], vr::k_unMaxTrackedDeviceCount, nullptr, 0);
/*
	for ( size_t i = 0; i < vr::k_unMaxTrackedDeviceCount; ++i ) {
		if ( ext::openvr::driver.poses[i].bDeviceIsConnected && ext::openvr::driver.poses[i].bPoseIsValid ) {

		}
	}
*/
	if ( ext::openvr::driver.poses[vr::k_unTrackedDeviceIndex_Hmd].bDeviceIsConnected && ext::openvr::driver.poses[vr::k_unTrackedDeviceIndex_Hmd].bPoseIsValid ) {
		vr::HmdMatrix34_t mat = ext::openvr::driver.poses[vr::k_unTrackedDeviceIndex_Hmd].mDeviceToAbsoluteTracking;
		::hmdPosition = {
			mat.m[0][0], mat.m[1][0], mat.m[2][0], 0.0,
			mat.m[0][1], mat.m[1][1], mat.m[2][1], 0.0,
			mat.m[0][2], mat.m[1][2], mat.m[2][2], 0.0,
			mat.m[0][3], mat.m[1][3], mat.m[2][3], 1.0f
		};
		::hmdPosition = uf::matrix::invert(::hmdPosition);
	}
}

#include <uf/ext/vulkan/vulkan.h>
#include <uf/ext/vulkan/commands/multiview.h>
void ext::openvr::submit() {
	float width = ext::vulkan::command->width > 0 ? ext::vulkan::command->width : ext::vulkan::width;
	float height = ext::vulkan::command->height > 0 ? ext::vulkan::command->height : ext::vulkan::height;
	// Submit to SteamVR
	vr::VRTextureBounds_t bounds;
	bounds.uMin = 0.0f;
	bounds.uMax = 1.0f;
	bounds.vMin = 0.0f;
	bounds.vMax = 1.0f;

	vr::VRVulkanTextureData_t vulkanData;
	vulkanData.m_nImage = (uint64_t) (VkImage) ext::vulkan::swapchain.buffers[ext::vulkan::currentBuffer].image;
	vulkanData.m_pDevice = ( VkDevice_T * ) ext::vulkan::device;
	vulkanData.m_pPhysicalDevice = ( VkPhysicalDevice_T * ) ext::vulkan::device.physicalDevice;
	vulkanData.m_pInstance = ( VkInstance_T *) ext::vulkan::device.instance;
	vulkanData.m_pQueue = ( VkQueue_T * ) ext::vulkan::device.presentQueue;
	vulkanData.m_nQueueFamilyIndex = ext::vulkan::device.queueFamilyIndices.present;

	vulkanData.m_nWidth = width;
	vulkanData.m_nHeight = height;
	vulkanData.m_nFormat = ext::vulkan::swapchain.colorFormat;
	vulkanData.m_nSampleCount = 1;

	vr::Texture_t texture = { &vulkanData, vr::TextureType_Vulkan, vr::ColorSpace_Auto };
	
	vulkanData.m_nImage = (uint64_t) (VkImage) ((ext::vulkan::MultiviewCommand*) ext::vulkan::command)->framebuffers.left.attachments[0].image;
	vr::VRCompositor()->Submit( vr::Eye_Left, &texture, &bounds );

	vulkanData.m_nImage = (uint64_t) (VkImage) ((ext::vulkan::MultiviewCommand*) ext::vulkan::command)->framebuffers.right.attachments[0].image;
	vr::VRCompositor()->Submit( vr::Eye_Right, &texture, &bounds );

	vr::VRCompositor()->PostPresentHandoff();
}

void ext::openvr::recommendedResolution( uint32_t& width, uint32_t& height ) {
	ext::openvr::width = width;
	ext::openvr::height = height;
	ext::openvr::context->GetRecommendedRenderTargetSize( &width, &height );
}

pod::Matrix4t<> ext::openvr::positionMatrix(  ) {
	return ::hmdPosition;
}
pod::Matrix4t<> ext::openvr::projectionMatrix( vr::Hmd_Eye eye, float n, float f ) {
	vr::HmdMatrix44_t mat = ext::openvr::context->GetProjectionMatrix( eye, n, f );
	return {
		mat.m[0][0], mat.m[1][0], mat.m[2][0], mat.m[3][0],
		mat.m[0][1], mat.m[1][1], mat.m[2][1], mat.m[3][1], 
		mat.m[0][2], mat.m[1][2], mat.m[2][2], mat.m[3][2], 
		mat.m[0][3], mat.m[1][3], mat.m[2][3], mat.m[3][3]
	};
}

pod::Matrix4t<> ext::openvr::eyeMatrix( vr::Hmd_Eye eye ) {
	vr::HmdMatrix34_t mat = ext::openvr::context->GetEyeToHeadTransform( eye );
	pod::Matrix4t<> res = {
		mat.m[0][0], mat.m[1][0], mat.m[2][0], 0.0, 
		mat.m[0][1], mat.m[1][1], mat.m[2][1], 0.0,
		mat.m[0][2], mat.m[1][2], mat.m[2][2], 0.0,
		mat.m[0][3], mat.m[1][3], mat.m[2][3], 1.0f
	};
	return uf::matrix::invert( res );
}