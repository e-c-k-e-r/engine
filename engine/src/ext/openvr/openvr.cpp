#include <uf/ext/openvr/openvr.h>

#if UF_USE_OPENVR
#include <uf/utils/io/iostream.h>
#include <uf/utils/string/ext.h>
#include <uf/utils/string/io.h>
#include <uf/utils/renderer/renderer.h>
#include <uf/utils/io/inputs.h>
#include <uf/engine/scene/scene.h>

#include <stdlib.h>

#define VR_CHECK_INPUT_RESULT(f)\
	if ( (err = (f)) != vr::VRInputError_None ) {\
		UF_MSG_ERROR("VR Error: {}", err);\
		return false;\
	}

#define VR_CHECK_COMPOSITOR_RESULT(f)\
	if ( (err = (f)) != vr::VRCompositorError_None ) UF_MSG_ERROR("VR Error: {}", err);

namespace {
	struct VRContext {
		vr::IVRSystem* system = nullptr;
		vr::IVRCompositor* compositor = nullptr;

		struct Driver {
			uf::stl::string name;
			uf::stl::string serial;
			uf::stl::string manifest = "./data/openvr_manifest.json";
			vr::TrackedDevicePose_t poses[vr::k_unMaxTrackedDeviceCount];
			uf::stl::string types[vr::k_unMaxTrackedDeviceCount];
			vr::IVRRenderModels* renderModels;
		} driver;

		uint8_t renderPass = 0;
		float width = 0;
		float height = 0;

		bool enabled = false;
		bool swapEyes = false;
		uint8_t dominantEye = 0;

		struct {
			struct { pod::Matrix4t<> matrix; } hmd;
			struct ControllerState {
				uint32_t index;
				bool active;
				uf::Serializer state;
				pod::Matrix4t<> matrix;
				pod::Matrix4t<> tip;
			} left, right;
		} devices;

		struct {
			uf::stl::unordered_map<uf::stl::string, vr::VRActionHandle_t> actions;
			uf::stl::unordered_map<uf::stl::string, vr::VRActionSetHandle_t> actionSets;
			uf::stl::vector<vr::VRActiveActionSet_t> activeActionSets;
		} handles;

		struct QueuedRenderModel {
			vr::RenderModel_t* model = nullptr;
			vr::RenderModel_TextureMap_t* texture = nullptr;
		};
		uf::stl::unordered_map<uf::stl::string, QueuedRenderModel> queuedRenderModels;
		uf::stl::unordered_map<uf::stl::string, uf::Mesh> renderModels;
		uf::stl::unordered_map<uf::stl::string, uf::Image> renderTextures;

		uf::stl::vector<uf::stl::string> renderModelNames;
	} g_vr;

	inline pod::Matrix4t<> convertMatrix(const vr::HmdMatrix34_t& mat) {
		return {
			mat.m[0][0], mat.m[1][0], mat.m[2][0], 0.0f,
			mat.m[0][1], mat.m[1][1], mat.m[2][1], 0.0f,
			mat.m[0][2], mat.m[1][2], mat.m[2][2], 0.0f,
			mat.m[0][3], mat.m[1][3], mat.m[2][3], 1.0f
		};
	}

	inline uf::stl::string getTrackedDeviceString(vr::IVRSystem *pHmd, vr::TrackedDeviceIndex_t unDevice, vr::TrackedDeviceProperty prop, vr::TrackedPropertyError *peError = nullptr) {
		uint32_t requiredBufferLen = pHmd->GetStringTrackedDeviceProperty(unDevice, prop, nullptr, 0, peError);
		if( requiredBufferLen == 0 ) return "";

		uf::stl::string sResult;
		sResult.resize(requiredBufferLen);
		pHmd->GetStringTrackedDeviceProperty(unDevice, prop, sResult.data(), requiredBufferLen, peError);

		while(!sResult.empty() && sResult.back() == '\0') sResult.pop_back();
		return sResult;
	}

	inline uf::stl::string getTrackedDeviceClassString(vr::ETrackedDeviceClass klass) {
		switch ( klass ) {
			case vr::TrackedDeviceClass_HMD: return "hmd";
			case vr::TrackedDeviceClass_Controller: return "controller";
			case vr::TrackedDeviceClass_GenericTracker:	return "generic tracker";
			case vr::TrackedDeviceClass_TrackingReference: return "base station";
			case vr::TrackedDeviceClass_DisplayRedirect: return "display redirect";
			default: return "invalid";
		}
	}
}

bool ext::openvr::enabled = false;
bool ext::openvr::initialize() {
	if ( !ext::openvr::enabled ) {
		return false;
	}

	if ( !vr::VR_IsHmdPresent() || !vr::VR_IsRuntimeInstalled() ) {
		UF_MSG_ERROR("VR Error: HMD not present or runtime missing.");
		return false;
	}

	vr::HmdError err;
	g_vr.system = vr::VR_Init( &err, vr::EVRApplicationType::VRApplication_Scene );
	if ( err != vr::VRInitError_None || !g_vr.system ) {
		UF_MSG_ERROR("Error initializing SteamVR: {}", vr::VR_GetVRInitErrorAsEnglishDescription(err));
		return false;
	}

	// get connected devices
	int baseStations = 0;
	for ( uint32_t i = vr::k_unTrackedDeviceIndex_Hmd; i < vr::k_unMaxTrackedDeviceCount; ++i ) {
		if ( !g_vr.system->IsTrackedDeviceConnected(i) ) continue;

		vr::ETrackedDeviceClass trackedClass = g_vr.system->GetTrackedDeviceClass(i);
		uf::stl::string typeName = getTrackedDeviceClassString(trackedClass);
		g_vr.driver.types[i] = typeName;

		UF_MSG_DEBUG("Tracking device {} connected ({}): {}", i, typeName, getTrackedDeviceString(g_vr.system, i, vr::Prop_TrackingSystemName_String));

		if ( trackedClass == vr::ETrackedDeviceClass::TrackedDeviceClass_TrackingReference ) {
			++baseStations;
		} else if ( trackedClass == vr::ETrackedDeviceClass::TrackedDeviceClass_Controller ) {
			vr::ETrackedControllerRole role = g_vr.system->GetControllerRoleForTrackedDeviceIndex(i);
			if ( role == vr::TrackedControllerRole_LeftHand ) g_vr.devices.left.index = i;
			else if ( role == vr::TrackedControllerRole_RightHand ) g_vr.devices.right.index = i;
		}

		if (i == vr::k_unTrackedDeviceIndex_Hmd) {
			g_vr.driver.name = getTrackedDeviceString(g_vr.system, i, vr::Prop_TrackingSystemName_String);
			g_vr.driver.serial = getTrackedDeviceString(g_vr.system, i, vr::Prop_SerialNumber_String);
		}
	}

	UF_MSG_DEBUG("HMD name={}, serial={}", g_vr.driver.name, g_vr.driver.serial);

	// load manifest and action handles
	{
		vr::EVRInputError err;
		VR_CHECK_INPUT_RESULT( vr::VRInput()->SetActionManifestPath( uf::io::absolute(g_vr.driver.manifest).c_str() ) );

		uf::Serializer manifest;
		manifest.readFromFile(g_vr.driver.manifest);

		for ( auto i = 0; i < manifest["action_sets"].size(); ++i ) {
			uf::stl::string name = manifest["action_sets"][i]["name"].as<uf::stl::string>();
			vr::VRActionSetHandle_t handle;
			VR_CHECK_INPUT_RESULT( vr::VRInput()->GetActionSetHandle( name.c_str(), &handle ) );
			g_vr.handles.actionSets[name] = handle;
		}

		for ( auto i = 0; i < manifest["actions"].size(); ++i ) {
			uf::stl::string name = manifest["actions"][i]["name"].as<uf::stl::string>();
			vr::VRActionHandle_t handle;
			VR_CHECK_INPUT_RESULT( vr::VRInput()->GetActionHandle( name.c_str(), &handle ) );
			g_vr.handles.actions[name] = handle;

			// add haptics to hooks
			uf::stl::vector<uf::stl::string> split = uf::string::split( name, "/" );
			uf::stl::string shortname = split.back();
			split = uf::string::split( shortname, "." );

			if ( split.front() == "hapticVibration" ) {
				uf::hooks.addHook( "VR:Haptics." + split.back(), [handle](ext::json::Value& json){
					vr::VRInput()->TriggerHapticVibrationAction(
						handle, json["delay"].as<float>(), json["duration"].as<float>(),
						json["frequency"].as<float>(), json["amplitude"].as<float>(),
						vr::k_ulInvalidInputValueHandle
					);
				});
			}
		}
	}

	// cache model names
	size_t count = vr::VRRenderModels()->GetRenderModelCount();
	g_vr.renderModelNames.reserve(count);
	for ( size_t i = 0; i < count; ++i ) {
		size_t strlen = vr::VRRenderModels()->GetRenderModelName( i, nullptr, 0 );
		if ( strlen == 0 ) continue;
		uf::stl::string name(strlen, '\0');
		vr::VRRenderModels()->GetRenderModelName( i, name.data(), strlen );
		name.pop_back();
		g_vr.renderModelNames.emplace_back( name );
	}

	uf::hooks.addHook( "VR:Seat.Reset", [&](ext::json::Value& json){ resetPosition(); });

	g_vr.compositor = vr::VRCompositor();
	if ( !g_vr.compositor ) {
		UF_MSG_ERROR("VR Error: Failed to initialize VR Compositor!");
		vr::VR_Shutdown();
		g_vr.system = nullptr;
		return false;
	}

	g_vr.compositor->SetExplicitTimingMode(vr::EVRCompositorTimingMode::VRCompositorTimingMode_Explicit_ApplicationPerformsPostPresentHandoff);
	g_vr.compositor->WaitGetPoses(&g_vr.driver.poses[0], vr::k_unMaxTrackedDeviceCount, nullptr, 0);

	return true;
}
void ext::openvr::terminate() {
	for ( auto& pair : g_vr.renderModels ) pair.second.destroy();
	g_vr.renderModels.clear();
	vr::VR_Shutdown();
	g_vr.system = nullptr;
}
void ext::openvr::resetPosition() {
	// if ( g_vr.system ) g_vr.system->ResetSeatedZeroPose();
}
void ext::openvr::tick() {
	if ( !g_vr.system ) return;

	// process queued render models
	for ( auto it = g_vr.queuedRenderModels.begin(); it != g_vr.queuedRenderModels.end(); ) {
		uf::stl::string name = it->first;
		auto& queued = it->second;

		if ( !queued.model ) {
			vr::EVRRenderModelError status = vr::VRRenderModels()->LoadRenderModel_Async( name.c_str(), &queued.model );
			if ( status == vr::VRRenderModelError_Loading ) { ++it; continue; }
			if ( status != vr::VRRenderModelError_None ) {
				it = g_vr.queuedRenderModels.erase(it);
				continue;
			}
		}

		if ( !queued.texture ) {
			vr::EVRRenderModelError status = vr::VRRenderModels()->LoadTexture_Async( queued.model->diffuseTextureId, &queued.texture );
			if ( status == vr::VRRenderModelError_Loading ) { ++it; continue; }
			if ( status != vr::VRRenderModelError_None ) {
				vr::VRRenderModels()->FreeRenderModel( queued.model );
				it = g_vr.queuedRenderModels.erase(it);
				continue;
			}
		}

		// process mesh
		{
			uf::stl::vector<pod::Vertex_3F2F3F> vertices;
			uf::stl::vector<uint16_t> indices;

			vertices.reserve( queued.model->unVertexCount );
			indices.reserve(queued.model->unTriangleCount * 3);

			for ( size_t i = 0; i < queued.model->unVertexCount; ++i ) {
				auto& v = queued.model->rVertexData[i];

				// might need to invert z
				auto& vertex = vertices.emplace_back();
				vertex.position = pod::Vector3f{ v.vPosition.v[0], v.vPosition.v[1], v.vPosition.v[2] };
				vertex.uv = pod::Vector2f{ v.rfTextureCoord[0], v.rfTextureCoord[1] };
				vertex.normal = pod::Vector3f{ v.vNormal.v[0], v.vNormal.v[1], v.vNormal.v[2] };
			}

			for ( std::size_t i = 0; i < queued.model->unTriangleCount * 3; i += 3 ) {
				indices.emplace_back( queued.model->rIndexData[i + 0] );
				indices.emplace_back( queued.model->rIndexData[i + 1] );
				indices.emplace_back( queued.model->rIndexData[i + 2] );
			}

			auto& mesh = g_vr.renderModels[name];
			mesh.bind<pod::Vertex_3F2F3F, uint16_t>();
			mesh.insertVertices(vertices);
			mesh.insertIndices(indices);
		}
		// process texture
		{
			auto& image = g_vr.renderTextures[name];
			uf::image::load( image, queued.texture->rubTextureMapData, { queued.texture->unWidth, queued.texture->unHeight }, 8, 4 );
		}

		ext::json::Value payload;
		payload["name"] = name;
		uf::hooks.call( "VR:Model.Loaded", payload );

		vr::VRRenderModels()->FreeRenderModel( queued.model );
		vr::VRRenderModels()->FreeTexture( queued.texture );
		it = g_vr.queuedRenderModels.erase( it );
	}

	// parse actions
	float fPredictedSecondsFromNow = predictedTimeToDisplay();

	g_vr.handles.activeActionSets.clear();
	for ( const auto& pair : g_vr.handles.actionSets ) {
		vr::VRActiveActionSet_t actionSet = { 0 };
		actionSet.ulActionSet = pair.second;
		g_vr.handles.activeActionSets.emplace_back(actionSet);
	}

	vr::VRInput()->UpdateActionState( g_vr.handles.activeActionSets.data(), sizeof(vr::VRActiveActionSet_t), g_vr.handles.activeActionSets.size() );

	for ( const auto& pair : g_vr.handles.actions ) {
		uf::stl::string name = pair.first;
		auto& handle = pair.second;

		uf::stl::vector<uf::stl::string> split = uf::string::split( name, "/" );
		uf::stl::string shortname = split.back();
		split = uf::string::split( shortname, "." );
		auto actionStr = split.front();
		auto handStr = split.back();

		auto& controller = (handStr == "left") ? g_vr.devices.left : g_vr.devices.right;

		vr::InputDigitalActionData_t digitalData;
		if ( vr::VRInputError_None == vr::VRInput()->GetDigitalActionData(handle, &digitalData, sizeof(digitalData), vr::k_ulInvalidInputValueHandle) ) {
			if ( digitalData.bActive ) {

				if ( handStr == "left" ) {
					if ( actionStr == "A" ) uf::inputs::controller::states::L_A = digitalData.bState;
					else if ( actionStr == "B" ) uf::inputs::controller::states::L_B = digitalData.bState;
					else if ( actionStr == "X" ) uf::inputs::controller::states::L_X = digitalData.bState;
					else if ( actionStr == "Y" ) uf::inputs::controller::states::L_Y = digitalData.bState;
					else if ( actionStr == "DPadUp" ) uf::inputs::controller::states::L_DPAD_UP = digitalData.bState;
					else if ( actionStr == "DPadDown" ) uf::inputs::controller::states::L_DPAD_DOWN = digitalData.bState;
					else if ( actionStr == "DPadLeft" ) uf::inputs::controller::states::L_DPAD_LEFT = digitalData.bState;
					else if ( actionStr == "DPadRight" ) uf::inputs::controller::states::L_DPAD_RIGHT = digitalData.bState;
					else if ( actionStr == "Start" ) uf::inputs::controller::states::START = digitalData.bState;
				} else if ( handStr == "right" ) {
					if ( actionStr == "A" ) uf::inputs::controller::states::R_A = digitalData.bState;
					else if ( actionStr == "B" ) uf::inputs::controller::states::R_B = digitalData.bState;
					else if ( actionStr == "X" ) uf::inputs::controller::states::R_X = digitalData.bState;
					else if ( actionStr == "Y" ) uf::inputs::controller::states::R_Y = digitalData.bState;
					else if ( actionStr == "DPadUp" ) uf::inputs::controller::states::R_DPAD_UP = digitalData.bState;
					else if ( actionStr == "DPadDown" ) uf::inputs::controller::states::R_DPAD_DOWN = digitalData.bState;
					else if ( actionStr == "DPadLeft" ) uf::inputs::controller::states::R_DPAD_LEFT = digitalData.bState;
					else if ( actionStr == "DPadRight" ) uf::inputs::controller::states::R_DPAD_RIGHT = digitalData.bState;
					else if ( actionStr == "Start" ) uf::inputs::controller::states::START = digitalData.bState;
				}

				if ( digitalData.bChanged ) {
					ext::json::Value payload;
					payload["name"] = actionStr;
					payload["hand"] = handStr;
					payload["full name"] = name;
					payload["deltaTime"] = digitalData.fUpdateTime;
					payload["state"] = digitalData.bState;
					controller.state[actionStr] = payload;
					uf::hooks.call( "VR:Input.Digital", payload );
				}
			}
		}

		vr::InputAnalogActionData_t analogData;
		if ( vr::VRInputError_None == vr::VRInput()->GetAnalogActionData(handle, &analogData, sizeof(analogData), vr::k_ulInvalidInputValueHandle) ) {
			if ( analogData.bActive ) {
				if ( handStr == "left" ) {
					if ( actionStr == "Joystick" || actionStr == "Trackpad" ) {
						uf::inputs::controller::states::L_JOYSTICK.x = analogData.x;
						uf::inputs::controller::states::L_JOYSTICK.y = analogData.y;
					} else if ( actionStr == "Trigger" ) {
						uf::inputs::controller::states::L_TRIGGER = analogData.x;
					}
				} else if ( handStr == "right" ) {
					if ( actionStr == "Joystick" || actionStr == "Trackpad" ) {
						uf::inputs::controller::states::R_JOYSTICK.x = analogData.x;
						uf::inputs::controller::states::R_JOYSTICK.y = analogData.y;
					} else if ( actionStr == "Trigger" ) {
						uf::inputs::controller::states::R_TRIGGER = analogData.x;
					}
				}

				pod::Vector3f delta = { analogData.deltaX, analogData.deltaY, analogData.deltaZ };
				pod::Vector3f position = { analogData.x, analogData.y, analogData.z };
				if ( delta != pod::Vector3f{ 0, 0, 0 } ) {
					ext::json::Value payload;
					payload["name"] = actionStr;
					payload["hand"] = handStr;
					payload["full name"] = name;
					payload["deltaTime"] = uf::time::delta;
					payload["analog"]["position"] = uf::vector::encode( position );
					payload["analog"]["delta"] = uf::vector::encode( delta );
					controller.state[actionStr] = payload;
					uf::hooks.call( "VR:Input.Analog", payload );
				}
			}
		}

		vr::InputPoseActionData_t poseData;
		if ( vr::VRInputError_None == vr::VRInput()->GetPoseActionDataRelativeToNow(handle, vr::TrackingUniverseStanding, fPredictedSecondsFromNow, &poseData, sizeof(poseData), vr::k_ulInvalidInputValueHandle) ) {
			if ( actionStr == "handPose" ) controller.active = poseData.bActive;
			if ( poseData.bActive ) {
				pod::Matrix4t<> res = convertMatrix(poseData.pose.mDeviceToAbsoluteTracking);
				if ( actionStr == "handPose" ) controller.matrix = res;
				else if ( actionStr == "handTip" ) controller.tip = res;
			}
		}
	}
}
bool ext::openvr::requestRenderModel( const uf::stl::string& name ) {
	if ( std::find( g_vr.renderModelNames.begin(), g_vr.renderModelNames.end(), name ) == g_vr.renderModelNames.end() ) return false;
	if ( g_vr.renderModels.count(name) == 1 ) return true;
	g_vr.queuedRenderModels[name];
	return false;
}
void ext::openvr::submit() {
	if ( !g_vr.system ) return;
#if UF_USE_VULKAN
	bool invert = g_vr.swapEyes;

	auto& scene = uf::scene::getCurrentScene();
	uf::renderer::RenderMode* renderModePointer = nullptr;
	if ( uf::renderer::hasRenderMode("VR") ) {
		renderModePointer = &uf::renderer::getRenderMode("VR");
	} else if ( scene.hasComponent<uf::renderer::DeferredRenderMode>() ) {
		renderModePointer = &scene.getComponent<uf::renderer::DeferredRenderMode>();
	} else {
		renderModePointer = &uf::renderer::getRenderMode("", true);
	}
	if ( !renderModePointer ) return;
	auto& renderMode = *renderModePointer;
	float width = renderMode.width > 0 ? renderMode.width : uf::renderer::settings::width;
	float height = renderMode.height > 0 ? renderMode.height : uf::renderer::settings::height;

	if ( !renderMode.hasAttachment("left") || !renderMode.hasAttachment("right") ) return;

	auto& leftEyeAttachment = renderMode.getAttachment("left");
	auto& rightEyeAttachment = renderMode.getAttachment("right");

	vr::VRTextureBounds_t bounds;
	bounds.uMin = 0.0f;
	bounds.uMax = 1.0f;
	bounds.vMin = 0.0f;
	bounds.vMax = 1.0f;

	vr::VRVulkanTextureData_t vulkanData = {};
	vulkanData.m_pDevice = ( VkDevice_T * ) uf::renderer::device;
	vulkanData.m_pPhysicalDevice = ( VkPhysicalDevice_T * ) uf::renderer::device.physicalDevice;
	vulkanData.m_pInstance = ( VkInstance_T *) uf::renderer::device.instance;
	vulkanData.m_pQueue = ( VkQueue_T * ) uf::renderer::device.getQueue( uf::renderer::QueueEnum::PRESENT );
	vulkanData.m_nQueueFamilyIndex = uf::renderer::device.queueFamilyIndices.present;
	vulkanData.m_nWidth = width;
	vulkanData.m_nHeight = height;
	vulkanData.m_nSampleCount = 1;

	vr::Texture_t texture = { &vulkanData, vr::TextureType_Vulkan, vr::ColorSpace_Auto };

	vr::EVRCompositorError err;
	vulkanData.m_nFormat = leftEyeAttachment.descriptor.format;
	vulkanData.m_nImage = (uint64_t) (VkImage) leftEyeAttachment.image;
	VR_CHECK_COMPOSITOR_RESULT(vr::VRCompositor()->Submit( invert ? vr::Eye_Right : vr::Eye_Left, &texture, &bounds ));


	vulkanData.m_nFormat = rightEyeAttachment.descriptor.format;
	vulkanData.m_nImage = (uint64_t) (VkImage) rightEyeAttachment.image;
	VR_CHECK_COMPOSITOR_RESULT(vr::VRCompositor()->Submit( invert ? vr::Eye_Left : vr::Eye_Right, &texture, &bounds ));

	vr::VRCompositor()->PostPresentHandoff();
#endif
}
void ext::openvr::synchronize() {
	if ( !g_vr.system ) return;
	vr::VRCompositor()->WaitGetPoses(nullptr, 0, nullptr, 0);
	updateTracking(1);
}
float ext::openvr::predictedTimeToDisplay( float additional ) {
	float displayFreq = g_vr.system->GetFloatTrackedDeviceProperty(vr::k_unTrackedDeviceIndex_Hmd, vr::Prop_DisplayFrequency_Float);
	float fFrameDuration = 1.f / displayFreq;

	float fSecondsSinceLastVsync = 0.0f;
	g_vr.system->GetTimeSinceLastVsync( &fSecondsSinceLastVsync, nullptr );

	float fSecondsFromVSyncToPhotons = g_vr.system->GetFloatTrackedDeviceProperty(vr::k_unTrackedDeviceIndex_Hmd, vr::Prop_SecondsFromVsyncToPhotons_Float);
	return additional * fFrameDuration - fSecondsSinceLastVsync + fSecondsFromVSyncToPhotons;
}
float ext::openvr::updateTracking(float additional) {
	float fPredicted = predictedTimeToDisplay(additional);
	g_vr.system->GetDeviceToAbsoluteTrackingPose(
		vr::ETrackingUniverseOrigin::TrackingUniverseStanding,
		fPredicted, g_vr.driver.poses, vr::k_unMaxTrackedDeviceCount
	);

	if ( g_vr.driver.poses[vr::k_unTrackedDeviceIndex_Hmd].bDeviceIsConnected && g_vr.driver.poses[vr::k_unTrackedDeviceIndex_Hmd].bPoseIsValid ) {
		g_vr.devices.hmd.matrix = convertMatrix(g_vr.driver.poses[vr::k_unTrackedDeviceIndex_Hmd].mDeviceToAbsoluteTracking);
		g_vr.devices.hmd.matrix = uf::matrix::inverse(g_vr.devices.hmd.matrix);
	}
	return fPredicted;
}

void ext::openvr::recommendedResolution( uint32_t& width, uint32_t& height ) {
	g_vr.system->GetRecommendedRenderTargetSize( &width, &height );
	g_vr.width = width; g_vr.height = height;
}

pod::Matrix4t<> ext::openvr::hmdHeadPositionMatrix() {
	return g_vr.devices.hmd.matrix;
}
pod::Matrix4t<> ext::openvr::hmdEyePositionMatrix( vr::Hmd_Eye eye ) {
	return uf::matrix::inverse( convertMatrix(g_vr.system->GetEyeToHeadTransform( eye )) );
}
pod::Matrix4t<> ext::openvr::hmdViewTranslationMatrix() {
	return uf::matrix::translate( uf::matrix::identity(), hmdPosition() );
}
pod::Matrix4t<> ext::openvr::hmdViewRotationMatrix() {
	pod::Matrix4t<> mat = hmdHeadPositionMatrix();
	mat(3,0) = 0;
	mat(3,1) = 0;
	mat(3,2) = 0;
	return mat;
}
pod::Vector3f ext::openvr::hmdPosition() {
	pod::Matrix4t<> mat = hmdHeadPositionMatrix();
	return {
		mat(3,0),
		mat(3,1),
		-mat(3,2),
	};
}
pod::Vector3f ext::openvr::hmdEyePosition( vr::Hmd_Eye eye ) {
	vr::HmdMatrix34_t mat = g_vr.system->GetEyeToHeadTransform( eye );
	return { mat.m[0][3], mat.m[1][3], mat.m[2][3] };
}
pod::Vector3f ext::openvr::hmdPosition( vr::Hmd_Eye eye ) {
	return hmdPosition() + hmdEyePosition( eye );
}
pod::Quaternion<> ext::openvr::hmdQuaternion() {
	return uf::quaternion::fromMatrix( hmdHeadPositionMatrix() );
}
pod::Matrix4t<> ext::openvr::hmdViewMatrix( vr::Hmd_Eye eye, const pod::Matrix4f& view ) {
	return hmdEyePositionMatrix( eye ) *
		uf::matrix::translate( uf::matrix::identity(), hmdPosition() ) *
		uf::matrix::inverse( uf::quaternion::matrix( hmdQuaternion() * pod::Vector4f{ 1, 1, -1, 1 } ) ) *
		view;
}
pod::Matrix4t<> ext::openvr::hmdProjectionMatrix( vr::Hmd_Eye eye, float zNear, float zFar ) {
	float left, right, top, bottom;
	g_vr.system->GetProjectionRaw( eye, &left, &right, &top, &bottom );

	pod::Matrix4t<> m = uf::matrix::identity();

	float idx = 1.0f / (right - left);
	float idy = 1.0f / (bottom - top);

	m(0,0) = 2.0f * idx;
	m(1,1) = 2.0f * idy;
	m(0,2) = (right + left) * idx;
	m(1,2) = (bottom + top) * idy;
	m(3,2) = 1.0f;
	m(3,3) = 0.0f;

	if ( zFar <= 0.0f ) {
		m(2,2) = 0.0f;
		m(2,3) = zNear;
	} else {
		float range = zFar - zNear;
		m(2,2) = zFar / range;
		m(2,3) = -(zFar * zNear) / range;
	}

	return m;
}
uf::Serializer ext::openvr::controllerState( vr::Controller_Hand hand, const uf::stl::string& key ) {
	auto& state = ( hand == vr::Controller_Hand::Hand_Left ) ? g_vr.devices.left.state : g_vr.devices.right.state;
	return key == "" ? state : state[key];
}
pod::Vector3f ext::openvr::controllerPosition( vr::Controller_Hand hand, bool tip ) {
	pod::Matrix4t<> mat = controllerMatrix( hand, tip );
	return {
		 mat(3,0),
		 mat(3,1),
		-mat(3,2),
	};
}
pod::Quaternion<> ext::openvr::controllerQuaternion( vr::Controller_Hand hand, bool tip ) {
	pod::Quaternion<> q = uf::quaternion::fromMatrix( controllerMatrix( hand, tip ) );
	q.w *= -1;
	return q;
}
pod::Matrix4t<> ext::openvr::controllerTranslationMatrix( vr::Controller_Hand hand, bool tip ) {
	return uf::matrix::translate( uf::matrix::identity(), controllerPosition( hand, tip ) );
}
pod::Matrix4t<> ext::openvr::controllerRotationMatrix( vr::Controller_Hand hand, bool tip ) {
	pod::Matrix4t<> mat = controllerMatrix( hand, tip );
	mat(3,0) = 0;
	mat(3,1) = 0;
	mat(3,2) = 0;
	return uf::matrix::inverse( mat );
}
pod::Matrix4t<> ext::openvr::controllerModelMatrix( vr::Controller_Hand hand, bool tip ) {
	return uf::matrix::translate( uf::matrix::identity(), controllerPosition( hand, tip ) ) * uf::quaternion::matrix( controllerQuaternion( hand, tip ) );
}
pod::Matrix4t<> ext::openvr::controllerMatrix( vr::Controller_Hand hand, bool tip ) {
	if ( hand == vr::Controller_Hand::Hand_Left ) return tip ? g_vr.devices.left.tip : g_vr.devices.left.matrix;
	else if ( hand == vr::Controller_Hand::Hand_Right ) return tip ? g_vr.devices.right.tip : g_vr.devices.right.matrix;
	return uf::matrix::identity();
}
bool ext::openvr::controllerActive( vr::Controller_Hand hand ) {
	return ( hand == vr::Controller_Hand::Hand_Left ) ? g_vr.devices.left.active : g_vr.devices.right.active;
}
uf::Mesh& ext::openvr::getRenderModel( const uf::stl::string& name ) {
	return g_vr.renderModels[name];
}
uf::Mesh& ext::openvr::controllerRenderModel( vr::Controller_Hand hand ) {
	if ( hand == vr::Controller_Hand::Hand_Left ) return g_vr.renderModels["{indexcontroller}valve_controller_knu_1_0_left"];
	else if ( hand == vr::Controller_Hand::Hand_Right ) return g_vr.renderModels["{indexcontroller}valve_controller_knu_1_0_right"];
	UF_EXCEPTION("OpenVR error: invalid hand requested");
}
uf::Image& ext::openvr::getRenderTexture( const uf::stl::string& name ) {
	return g_vr.renderTextures[name];
}
uf::Image& ext::openvr::controllerRenderTexture( vr::Controller_Hand hand ) {
	if ( hand == vr::Controller_Hand::Hand_Left ) return g_vr.renderTextures["{indexcontroller}valve_controller_knu_1_0_left"];
	else if ( hand == vr::Controller_Hand::Hand_Right ) return g_vr.renderTextures["{indexcontroller}valve_controller_knu_1_0_right"];
	UF_EXCEPTION("OpenVR error: invalid hand requested");
}
#endif