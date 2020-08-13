#include <uf/ext/openvr/openvr.h>
#include <uf/utils/io/iostream.h>
#include <uf/utils/string/ext.h>

#include <uf/ext/vulkan/vulkan.h>
#include <uf/ext/vulkan/rendermodes/stereoscopic_deferred.h>

#include <filesystem>

vr::IVRSystem* ext::openvr::context;
ext::openvr::Driver ext::openvr::driver;
uint8_t ext::openvr::renderPass = 0;
float ext::openvr::width = 0;
float ext::openvr::height = 0;
bool ext::openvr::enabled = false;

#define VR_CHECK_INPUT_RESULT(f)\
	if ( f != vr::VRInputError_None ) {\
		uf::iostream << __FILE__ << ":" << __FUNCTION__ << ":L" << __LINE__ << ": VR Error: " << vr::VR_GetVRInitErrorAsEnglishDescription(err) << "\n";\
		return false;\
	}

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
	struct {
		struct {
			pod::Matrix4t<> matrix;
		} hmd;
		struct {
			struct {
				uint32_t index;
				bool active;
				uf::Serializer state;
				pod::Matrix4t<> matrix;
				pod::Matrix4t<> tip;
				uf::Mesh mesh;
			} left, right;
		} controllers;
	} devices;

	struct {
		std::unordered_map<std::string, vr::VRActionHandle_t> actions;
		std::unordered_map<std::string, vr::VRActionSetHandle_t> actionSets;
	//	std::vector<vr::VRInputValueHandle_t> inputs;
		std::vector<vr::VRActiveActionSet_t> activeActionSets;
	} handles;

	struct QueuedRenderModel {
		vr::RenderModel_t* model;
		vr::RenderModel_TextureMap_t* texture;
	};
	std::unordered_map<std::string, QueuedRenderModel> queuedRenderModels;
	std::unordered_map<std::string, uf::Mesh> renderModels;
	std::vector<std::string> renderModelNames;
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
		else if ( trackedDeviceClass == vr::ETrackedDeviceClass::TrackedDeviceClass_Controller ) {
			vr::ETrackedControllerRole role = ext::openvr::context->GetControllerRoleForTrackedDeviceIndex(i);
			if ( role == vr::TrackedControllerRole_LeftHand ) ::devices.controllers.left.index = i;
			else if ( role == vr::TrackedControllerRole_RightHand ) ::devices.controllers.right.index = i;
		}
		if (i == vr::k_unTrackedDeviceIndex_Hmd) {
			ext::openvr::driver.name = ::GetTrackedDeviceString(ext::openvr::context, i, vr::Prop_TrackingSystemName_String);
			ext::openvr::driver.serial = ::GetTrackedDeviceString(ext::openvr::context, i, vr::Prop_SerialNumber_String);
		}
	}
/*
	// Get render models
	driver.renderModels = (vr::IVRRenderModels*) vr::VR_GetGenericInterface( vr::IVRRenderModels_Version, &err );
	if ( err != vr::VRInitError_None ) {
		uf::iostream << "Error while retrieving render model interface: " << vr::VR_GetVRInitErrorAsEnglishDescription(err) << "\n";
		return false;
	}
*/
	uf::iostream << "HMD " << ext::openvr::driver.name << " " << ext::openvr::driver.serial << "\n";

	// load manifest
	{
		std::filesystem::path p = ext::openvr::driver.manifest;
		p = std::filesystem::absolute( p );
		ext::openvr::driver.manifest = p.string();
		VR_CHECK_INPUT_RESULT( vr::VRInput()->SetActionManifestPath( ext::openvr::driver.manifest.c_str() ) );
		// configure action handles
		{
			uf::Serializer manifest;
			manifest.readFromFile(ext::openvr::driver.manifest);

			std::cout << ext::openvr::driver.manifest << std::endl;
			for ( auto i = 0; i < manifest["action_sets"].size(); ++i ) {
				std::cout << manifest["action_sets"][i] << std::endl;
				std::string name = manifest["action_sets"][i]["name"].asString();
				vr::VRActionSetHandle_t handle;
				VR_CHECK_INPUT_RESULT( vr::VRInput()->GetActionSetHandle( name.c_str(), &handle ) );
				handles.actionSets[name] = handle;
				std::cout << "Registered action set: " << name << std::endl;
				// handles.actionSets.push_back(handle);
			}
			for ( auto i = 0; i < manifest["actions"].size(); ++i ) {
				std::string name = manifest["actions"][i]["name"].asString();
				vr::VRActionHandle_t handle;
				VR_CHECK_INPUT_RESULT( vr::VRInput()->GetActionHandle( name.c_str(), &handle ) );
				handles.actions[name] = handle;
				std::cout << "Registered action: " << name << std::endl;
				// handles.actions.push_back(handle);

				// add haptics to hooks
				{
					std::vector<std::string> split = uf::string::split( name, "/" );
					std::string shortname = split.back();
					split = uf::string::split( shortname, "." );
					if ( split.front() == "hapticVibration" ) {
						std::cout << "Registered hook for haptic: " << ("VR:Haptics."+split.back()) << std::endl;
						
						uf::hooks.addHook( "VR:Haptics."+split.back(), [](const std::string& event)->std::string{
							uf::Serializer json = event;
							std::string name;
							std::string side = json["side"].asString();
							uf::Serializer manifest;
							manifest.readFromFile(ext::openvr::driver.manifest);
							{
								for ( auto i = 0; i < manifest["actions"].size(); ++i ) {
									std::string handleName = manifest["actions"][i]["name"].asString();
									std::vector<std::string> split = uf::string::split( handleName, "/" );
									std::string shortname = split.back();
									if ( shortname != "hapticVibration." + side ) continue;
									name = handleName;
									break;
								}
							}
							if ( name == "" ) return "false";
							vr::VRActionHandle_t& handle = handles.actions[name];
							auto err = vr::VRInput()->TriggerHapticVibrationAction( handle, json["delay"].asFloat(), json["duration"].asFloat(), json["frequency"].asFloat(), json["amplitude"].asFloat(), vr::k_ulInvalidInputValueHandle );
							if ( err != vr::VRInputError_None ) {
								std::cout << err << std::endl;
								return "false";
							}
							return "true";
						});
					}
				}
			}
		}
	}

	// load render models
	{
		size_t count = vr::VRRenderModels()->GetRenderModelCount();
		::renderModelNames.reserve(count);
		for ( size_t i = 0; i < count; ++i ) {
			size_t strlen = vr::VRRenderModels()->GetRenderModelName( i, NULL, 0 );
			char buffer[strlen];
			vr::VRRenderModels()->GetRenderModelName( i, &buffer[0], strlen );
			std::string name = buffer;
			::renderModelNames.push_back( name );
		}
	}

	uf::hooks.addHook( "VR:Seat.Reset", [&](const std::string& event)->std::string{
		uf::Serializer json = event;
		ext::openvr::resetPosition();
		return "true";
	});

	return true;
}
void ext::openvr::terminate() {
	::devices.controllers.left.mesh.graphic.destroy();
	::devices.controllers.right.mesh.graphic.destroy();
	::devices.controllers.left.mesh.destroy();
	::devices.controllers.right.mesh.destroy();
	vr::VR_Shutdown();
	ext::openvr::context = NULL;
}
void ext::openvr::resetPosition() {
	ext::openvr::context->ResetSeatedZeroPose();
}
void ext::openvr::tick() {
	if ( !ext::openvr::context ) return;
	
	// load render model from api
	for ( auto pair : ::queuedRenderModels ) {
		std::string name = pair.first;
		auto& queued = pair.second;
		{
			vr::EVRRenderModelError status = vr::VRRenderModels()->LoadRenderModel_Async( name.c_str(), &queued.model );
			// wait for load
			if ( status == vr::VRRenderModelError_Loading ) continue;
			// error'd out
			if ( status != vr::VRRenderModelError_None ) {
				std::cout << "Failed to load render model " << name << ": " << vr::VRRenderModels()->GetRenderModelErrorNameFromEnum( status ) << std::endl;
				::queuedRenderModels.erase(name);
				continue;
			}
		}
		// loaded, now to load texture
		{
			vr::EVRRenderModelError status = vr::VRRenderModels()->LoadTexture_Async( queued.model->diffuseTextureId, &queued.texture );
			// wait for load
			if ( status == vr::VRRenderModelError_Loading ) continue;
			// error'd out
			if ( status != vr::VRRenderModelError_None ) {
				std::cout << "Failed to load render model " << name << "'s texture: " << vr::VRRenderModels()->GetRenderModelErrorNameFromEnum( status ) << std::endl;
				vr::VRRenderModels()->FreeRenderModel( queued.model );
				::queuedRenderModels.erase(name);
				continue;
			}
		}
		// loaded texture, process
		{
			uf::Mesh& mesh = renderModels[name];
			mesh.vertices.reserve(queued.model->unVertexCount);
			for ( size_t i = 0; i < queued.model->unVertexCount; ++i ) {
				auto& v = queued.model->rVertexData[i];
				
				uf::Mesh::vertex_t vertex;
				vertex.position = pod::Vector3f{ v.vPosition.v[0], v.vPosition.v[1], -v.vPosition.v[2] };
				vertex.normal = pod::Vector3f{ v.vNormal.v[0], v.vNormal.v[1], v.vNormal.v[2] };
				vertex.uv = pod::Vector2f{ v.rfTextureCoord[0], v.rfTextureCoord[1] };
				
				mesh.vertices.push_back(vertex);
			}
			mesh.indices.reserve(queued.model->unTriangleCount * 3);
			for ( std::size_t i = 0; i < queued.model->unTriangleCount * 3; ++i ) {
				mesh.indices.push_back( queued.model->rIndexData[i] );
			}
			// grab texture
			size_t len = queued.texture->unWidth * queued.texture->unHeight * 4;
		//	mesh.graphic.texture.fromBuffers( (void*) queued.texture->rubTextureMapData, len, VK_FORMAT_R8G8B8A8_UNORM, queued.texture->unWidth, queued.texture->unHeight, ext::vulkan::device, ext::vulkan::device.graphicsQueue );
			mesh.initialize(true);
			mesh.graphic.process = false;
			mesh.graphic.initialize();

			auto& texture = mesh.graphic.material.textures.emplace_back();
			texture.fromBuffers( (void*) queued.texture->rubTextureMapData, len, VK_FORMAT_R8G8B8A8_UNORM, queued.texture->unWidth, queued.texture->unHeight, ext::vulkan::device, ext::vulkan::device.graphicsQueue );
		}
		// clear
		{
			::queuedRenderModels.erase(name);
			vr::VRRenderModels()->FreeRenderModel( queued.model );
			vr::VRRenderModels()->FreeTexture( queued.texture );
		}
		// call hook
		uf::Serializer payload;
		payload["name"] = name;
		uf::hooks.call( "VR:Model.Loaded", payload );
	}
	// 

	// Obtain tracking device poses
	float predictedDisplayTime = 0.0f;
	{
		float displayFrequency = ext::openvr::context->GetFloatTrackedDeviceProperty(vr::k_unTrackedDeviceIndex_Hmd, vr::Prop_DisplayFrequency_Float);
		float frameDuration = 1.f / displayFrequency;
		float vsyncToPhotons = ext::openvr::context->GetFloatTrackedDeviceProperty(vr::k_unTrackedDeviceIndex_Hmd, vr::Prop_SecondsFromVsyncToPhotons_Float);
		predictedDisplayTime = frameDuration + vsyncToPhotons;
		ext::openvr::context->GetDeviceToAbsoluteTrackingPose(
			vr::ETrackingUniverseOrigin::TrackingUniverseStanding,
			predictedDisplayTime,
			ext::openvr::driver.poses,
			vr::k_unMaxTrackedDeviceCount
		);
		vr::VRCompositor()->WaitGetPoses(&driver.poses[0], vr::k_unMaxTrackedDeviceCount, nullptr, 0);
		if ( ext::openvr::driver.poses[vr::k_unTrackedDeviceIndex_Hmd].bDeviceIsConnected && ext::openvr::driver.poses[vr::k_unTrackedDeviceIndex_Hmd].bPoseIsValid ) {
			vr::HmdMatrix34_t mat = ext::openvr::driver.poses[vr::k_unTrackedDeviceIndex_Hmd].mDeviceToAbsoluteTracking;
			::devices.hmd.matrix = {
				mat.m[0][0], mat.m[1][0], mat.m[2][0], 0.0,
				mat.m[0][1], mat.m[1][1], mat.m[2][1], 0.0,
				mat.m[0][2], mat.m[1][2], mat.m[2][2], 0.0,
				mat.m[0][3], mat.m[1][3], mat.m[2][3], 1.0f
			};
			::devices.hmd.matrix = uf::matrix::invert(::devices.hmd.matrix);
		}
	}
	// Parse actions
	{
		{
			handles.activeActionSets.clear();
			for ( auto pair : handles.actionSets ) {
				vr::VRActiveActionSet_t actionSet = { 0 };
				actionSet.ulActionSet = pair.second;
				handles.activeActionSets.push_back(actionSet);
			}
		}
		vr::VRInput()->UpdateActionState( handles.activeActionSets.data(), sizeof(vr::VRActiveActionSet_t), handles.activeActionSets.size() );
		for ( auto pair : handles.actions ) {
			std::string name = pair.first;
			auto& handle = pair.second;
			// analog data
			{
				vr::InputAnalogActionData_t data;
				if ( vr::VRInputError_None == vr::VRInput()->GetAnalogActionData(handle, &data, sizeof(data), vr::k_ulInvalidInputValueHandle) ) {
					float deltaTime = data.fUpdateTime;
					pod::Vector3f delta = { data.deltaX, data.deltaY, data.deltaZ };
					pod::Vector3f position = { data.x, data.y, data.z };
					if ( data.bActive ) {
						uf::Serializer payload;
						std::vector<std::string> split = uf::string::split( name, "/" );
						std::string shortname = split.back();
						split = uf::string::split( shortname, "." );
						
						payload["name"] = split.front();
						payload["hand"] = split.back();
						payload["full name"] = name;
						payload["deltaTime"] = deltaTime;
						payload["analog"]["position"][0] = position.x;
						payload["analog"]["position"][1] = position.y;
						payload["analog"]["position"][2] = position.z;
						payload["analog"]["delta"][0] = delta.x;
						payload["analog"]["delta"][1] = delta.y;
						payload["analog"]["delta"][2] = delta.z;
						// store state
						if ( split.back() == "left" ) ::devices.controllers.left.state[split.front()] = payload;
						else if ( split.back() == "right" ) ::devices.controllers.right.state[split.front()] = payload;
						// send payload only on deltas
						if ( delta != pod::Vector3f{ 0, 0, 0 } ) uf::hooks.call( "VR:Input.Analog", payload );
					}
				}
			}
			// digital data
			{
				vr::InputDigitalActionData_t data;
				if ( vr::VRInputError_None == vr::VRInput()->GetDigitalActionData(handle, &data, sizeof(data), vr::k_ulInvalidInputValueHandle) ) {
					if ( data.bActive ) {
						uf::Serializer payload;
						std::vector<std::string> split = uf::string::split( name, "/" );
						std::string shortname = split.back();
						split = uf::string::split( shortname, "." );
						
						payload["name"] = split.front();
						payload["hand"] = split.back();
						payload["full name"] = name;
						payload["deltaTime"] = data.fUpdateTime;
						payload["state"] = data.bState;
						// store state
						if ( split.back() == "left" ) ::devices.controllers.left.state[split.front()] = payload;
						else if ( split.back() == "right" ) ::devices.controllers.right.state[split.front()] = payload;
						// send payload only on deltas
						if ( data.bChanged ) uf::hooks.call( "VR:Input.Digital", payload );
					}
				}
			}
			// pose data
			{
				vr::InputPoseActionData_t data;
				if ( vr::VRInputError_None == vr::VRInput()->GetPoseActionDataRelativeToNow(handle, vr::TrackingUniverseStanding,  predictedDisplayTime, &data, sizeof(data), vr::k_ulInvalidInputValueHandle) ) {
					std::vector<std::string> split = uf::string::split( name, "/" );
					std::string shortname = split.back();
					split = uf::string::split( shortname, "." );
					if ( split.front() == "handPose" ) {
						if ( split.back() == "left" ) ::devices.controllers.left.active = data.bActive;
						else if ( split.back() == "right" ) ::devices.controllers.right.active = data.bActive;
					}
					if ( data.bActive ) {
						vr::HmdMatrix34_t mat = data.pose.mDeviceToAbsoluteTracking;
						pod::Matrix4t<> res = {
							mat.m[0][0], mat.m[1][0], mat.m[2][0], 0.0, 
							mat.m[0][1], mat.m[1][1], mat.m[2][1], 0.0,
							mat.m[0][2], mat.m[1][2], mat.m[2][2], 0.0,
							mat.m[0][3], mat.m[1][3], mat.m[2][3], 1.0f
						};
						if ( split.back() == "left" ) {
							if ( split.front() == "handPose" ) ::devices.controllers.left.matrix = res;
							else if ( split.front() == "handTip" ) ::devices.controllers.left.tip = res;
						}
						else if ( split.back() == "right" ) {
							if ( split.front() == "handPose" ) ::devices.controllers.right.matrix = res;
							else if ( split.front() == "handTip" ) ::devices.controllers.right.tip = res;
						}
					}
				}
			}
		}
	}
}
bool ext::openvr::requestRenderModel( const std::string& name ) {
	if ( std::find( renderModelNames.begin(), renderModelNames.end(), name ) == renderModelNames.end() ) return false;
	if ( renderModels.count(name) == 1 ) return true;
	::queuedRenderModels[name];
	return false;	
}
void ext::openvr::submit() { bool invert = false;
	
	ext::vulkan::StereoscopicDeferredRenderMode* renderMode = (ext::vulkan::StereoscopicDeferredRenderMode*) &ext::vulkan::getRenderMode("");
	
	float width = renderMode->width > 0 ? renderMode->width : ext::vulkan::width;
	float height = renderMode->height > 0 ? renderMode->height : ext::vulkan::height;
	auto& leftOutputAttachment = renderMode->renderTargets.left.attachments[renderMode->renderTargets.left.attachments.size()-1];
	auto& rightOutputAttachment = renderMode->renderTargets.right.attachments[renderMode->renderTargets.right.attachments.size()-1];

	// Submit to SteamVR
	vr::VRTextureBounds_t bounds;
	bounds.uMin = 0.0f;
	bounds.uMax = 1.0f;
	bounds.vMin = 0.0f;
	bounds.vMax = 1.0f;

	vr::VRVulkanTextureData_t vulkanData;
	vulkanData.m_pDevice = ( VkDevice_T * ) ext::vulkan::device;
	vulkanData.m_pPhysicalDevice = ( VkPhysicalDevice_T * ) ext::vulkan::device.physicalDevice;
	vulkanData.m_pInstance = ( VkInstance_T *) ext::vulkan::device.instance;
	vulkanData.m_pQueue = ( VkQueue_T * ) ext::vulkan::device.presentQueue;
	vulkanData.m_nQueueFamilyIndex = ext::vulkan::device.queueFamilyIndices.present;

	vulkanData.m_nWidth = width;
	vulkanData.m_nHeight = height;
	vulkanData.m_nSampleCount = 1;

	vr::Texture_t texture = { &vulkanData, vr::TextureType_Vulkan, vr::ColorSpace_Auto };
	
	vulkanData.m_nFormat = leftOutputAttachment.format;
	vulkanData.m_nImage = (uint64_t) (VkImage) leftOutputAttachment.image;
	// std::cout << leftOutputAttachment.image << std::endl;
	vr::VRCompositor()->Submit( invert ? vr::Eye_Right : vr::Eye_Left, &texture, &bounds );

	vulkanData.m_nFormat = rightOutputAttachment.format;
	vulkanData.m_nImage = (uint64_t) (VkImage) rightOutputAttachment.image;
	// std::cout << rightOutputAttachment.image << std::endl;
	vr::VRCompositor()->Submit( invert ? vr::Eye_Left : vr::Eye_Right, &texture, &bounds );

	vr::VRCompositor()->PostPresentHandoff();
}

void ext::openvr::recommendedResolution( uint32_t& width, uint32_t& height ) {
	ext::openvr::width = width;
	ext::openvr::height = height;
	ext::openvr::context->GetRecommendedRenderTargetSize( &width, &height );
}

pod::Matrix4t<> ext::openvr::hmdHeadPositionMatrix() {
	return ::devices.hmd.matrix;
}
pod::Matrix4t<> ext::openvr::hmdEyePositionMatrix( vr::Hmd_Eye eye ) {
	vr::HmdMatrix34_t mat = ext::openvr::context->GetEyeToHeadTransform( eye );
	pod::Matrix4t<> res = {
		mat.m[0][0], mat.m[1][0], mat.m[2][0], 0.0, 
		mat.m[0][1], mat.m[1][1], mat.m[2][1], 0.0,
		mat.m[0][2], mat.m[1][2], mat.m[2][2], 0.0,
		mat.m[0][3], mat.m[1][3], mat.m[2][3], 1.0f
	};
	res = uf::matrix::inverse( res );
	return res;
}
pod::Matrix4t<> ext::openvr::hmdViewTranslationMatrix() {
	return uf::matrix::translate( uf::matrix::identity(), hmdPosition() );
}
pod::Matrix4t<> ext::openvr::hmdViewRotationMatrix() {
	pod::Matrix4t<> mat = hmdHeadPositionMatrix();
	mat[(4*3)+0] = 0;
	mat[(4*3)+1] = 0;
	mat[(4*3)+2] = 0;
	return mat;
}
pod::Vector3f ext::openvr::hmdPosition() {
	pod::Matrix4t<> mat = hmdHeadPositionMatrix();
	return {
		mat[(4*3)+0],
		mat[(4*3)+1],
		-mat[(4*3)+2],
	};
}
pod::Vector3f ext::openvr::hmdEyePosition( vr::Hmd_Eye eye ) {
	vr::HmdMatrix34_t mat = ext::openvr::context->GetEyeToHeadTransform( eye );
	pod::Vector3f offset = { mat.m[0][3], mat.m[1][3], mat.m[2][3] };
	return offset;
}
pod::Vector3f ext::openvr::hmdPosition( vr::Hmd_Eye eye ) {
	return hmdPosition() + hmdEyePosition( eye );
}
pod::Quaternion<> ext::openvr::hmdQuaternion() {
	return uf::quaternion::fromMatrix( hmdHeadPositionMatrix() ); // * pod::Vector4f{ 1, 1, -1, -1 };
}
pod::Matrix4t<> ext::openvr::hmdViewMatrix( vr::Hmd_Eye eye, const pod::Matrix4f& mv ) {
	return hmdEyePositionMatrix( eye ) * uf::matrix::translate( uf::matrix::identity(), hmdPosition() ) * uf::matrix::inverse( uf::quaternion::matrix( ext::openvr::hmdQuaternion() * pod::Vector4f{ 1, 1, -1, -1 } ) ) * mv;
//	return uf::matrix::translate( uf::matrix::identity(), hmdEyePosition( eye ) ) * uf::matrix::inverse( uf::quaternion::matrix( ext::openvr::hmdQuaternion() ) ) * mv;
//	return uf::matrix::scale(uf::matrix::identity(), pod::Vector3f{1,1,-1}) * hmdEyePositionMatrix( eye ) * hmdHeadPositionMatrix() * mv;
}
pod::Matrix4t<> ext::openvr::hmdProjectionMatrix( vr::Hmd_Eye eye, float zNear, float zFar ) {
	struct {
		float left;
		float right;
		float top;
		float bottom;
	} frustum;
	ext::openvr::context->GetProjectionRaw( eye, &frustum.left, &frustum.right, &frustum.top, &frustum.bottom );

	if ( true ) {
		frustum.left = -abs( frustum.left );
		frustum.right = abs( frustum.right );
		frustum.top = -abs( frustum.top );
		frustum.bottom = abs( frustum.bottom );
	} else {
	}
		std::cout << frustum.left << "\t" << frustum.right << "\t" << frustum.top << "\t" << frustum.bottom << std::endl;

	/*
		float fov = this->m_settings.perspective.fov * (3.14159265358f / 180.0f);
		float raidou = (float) this->m_settings.perspective.size.x / (float) this->m_settings.perspective.size.y;
		float f = 1.0f / tan( 0.5f * fov );
		this->m_matrices.projection = {
			f / raidou, 	0.0f, 	 0.0f, 	0.0f,
			0.0f, 			-f, 	 0.0f, 	0.0f,
			0.0f,       	0.0f,    0.0f, 	1.0f,
			0.0f,       	0.0f,   this->m_settings.perspective.bounds.x, 	0.0f
		};
	*/

	switch ( 0 ) {
		case 0: {
			float Sx = 2 / (frustum.right - frustum.left);
			float Sy = 2 / (frustum.top - frustum.bottom);
			float S20 = (frustum.right + frustum.left) / (frustum.right - frustum.left);
			float S21 = (frustum.top + frustum.bottom) / (frustum.top - frustum.bottom);
			float Sz = -zFar / (zFar - zNear);
			float S32 = -zFar * zNear / (zFar - zNear);
		/*
			return {
				 Sx,   0,   0,   0,
				  0,  Sy,   0,   0,
				S20, S21,   0,   1,
				  0,   0, zNear,   0,
			};
		*/
			return uf::matrix::multiply( pod::Matrix4f{
				1, 0, 0, 0,
				0, 1, 0, 0,
				0, 0, 1, 0,
				0, 0, 0, 1,
			}, pod::Matrix4f{
				 Sx,   0,   0,   0,
				  0,  Sy,   0,   0,
				-S20, -S21,   0,   1,
				  0,   0, zNear,   0,
			});
		} break;
		default: {
			vr::HmdMatrix44_t mat = ext::openvr::context->GetProjectionMatrix( eye, zFar, zNear );
			return uf::matrix::multiply( pod::Matrix4f{
				1, 0, 0, 0,
				0,-1, 0, 0,
				0, 0, 1, 0,
				0, 0, 0, 1,
			}, pod::Matrix4f{
				mat.m[0][0], mat.m[1][0], mat.m[2][0], mat.m[3][0],
				mat.m[0][1], mat.m[1][1], mat.m[2][1], mat.m[3][1], 
			    -mat.m[0][2], -mat.m[1][2], mat.m[2][2],-mat.m[3][2], 
				mat.m[0][3], mat.m[1][3], mat.m[2][3], mat.m[3][3]
			});
		} break;
	}
}
uf::Serializer ext::openvr::controllerState( vr::Controller_Hand hand, const std::string& key ) {
	if ( hand == vr::Controller_Hand::Hand_Left ) return key == "" ? ::devices.controllers.left.state : ::devices.controllers.left.state[key];
	else if ( hand == vr::Controller_Hand::Hand_Right ) return key == "" ? ::devices.controllers.right.state : ::devices.controllers.right.state[key];
	return uf::Serializer();
}
pod::Vector3f ext::openvr::controllerPosition( vr::Controller_Hand hand, bool tip ) {
	pod::Matrix4t<> mat = controllerMatrix( hand, tip );
	return {
		 mat[(4*3)+0],
		 mat[(4*3)+1],
		-mat[(4*3)+2],
	};
}
pod::Quaternion<> ext::openvr::controllerQuaternion( vr::Controller_Hand hand, bool tip ) {
	return uf::quaternion::fromMatrix( controllerMatrix( hand, tip ) ) * pod::Vector4f{ 1, 1, -1, 1 };
}
pod::Matrix4t<> ext::openvr::controllerTranslationMatrix( vr::Controller_Hand hand, bool tip ) {
	return uf::matrix::translate( uf::matrix::identity(), controllerPosition( hand, tip ) );
}
pod::Matrix4t<> ext::openvr::controllerRotationMatrix( vr::Controller_Hand hand, bool tip ) {
	pod::Matrix4t<> mat = controllerMatrix( hand, tip );
	mat[(4*3)+0] = 0;
	mat[(4*3)+1] = 0;
	mat[(4*3)+2] = 0;
	return uf::matrix::inverse( mat );
}
pod::Matrix4t<> ext::openvr::controllerModelMatrix( vr::Controller_Hand hand, bool tip ) {
	return uf::matrix::translate( uf::matrix::identity(), controllerPosition( hand, tip ) ) * uf::quaternion::matrix( controllerQuaternion( hand, tip ) );
//	return uf::matrix::translate( uf::matrix::identity(), controllerPosition( hand, tip ) ) * controllerRotationMatrix( hand, tip );
//	return uf::matrix::scale( uf::matrix::identity(), pod::Vector3f{ 1, 1, -1 } ) * controllerMatrix( hand, tip );
//	return uf::matrix::translate(  uf::quaternion::matrix( ext::openvr::controllerQuaternion( hand, tip ) ),  controllerPosition( hand, tip ) );
}
pod::Matrix4t<> ext::openvr::controllerMatrix( vr::Controller_Hand hand, bool tip ) {
	if ( hand == vr::Controller_Hand::Hand_Left ) return tip ? ::devices.controllers.left.tip : ::devices.controllers.left.matrix;
	else if ( hand == vr::Controller_Hand::Hand_Right ) return tip ? ::devices.controllers.right.tip : ::devices.controllers.right.matrix;
	return uf::matrix::identity();
}
bool ext::openvr::controllerActive( vr::Controller_Hand hand ) {
	if ( hand == vr::Controller_Hand::Hand_Left ) return ::devices.controllers.left.active;
	else if ( hand == vr::Controller_Hand::Hand_Right ) return ::devices.controllers.right.active;
	return false;
}
uf::Mesh& ext::openvr::getRenderModel( const std::string& name ) {
	return ::renderModels[name];
}
uf::Mesh& ext::openvr::controllerRenderModel( vr::Controller_Hand hand ) {
	if ( hand == vr::Controller_Hand::Hand_Left ) return renderModels["{indexcontroller}valve_controller_knu_1_0_left"]; //return ::devices.controllers.left.mesh;
	else if ( hand == vr::Controller_Hand::Hand_Right ) return renderModels["{indexcontroller}valve_controller_knu_1_0_right"]; //return ::devices.controllers.right.mesh;
	throw false; //std::exception("error");
}