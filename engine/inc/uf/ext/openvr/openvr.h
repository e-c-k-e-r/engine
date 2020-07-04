#pragma once

#include <uf/config.h>
#include <uf/utils/math/matrix.h>
#ifdef USE_OPENVR_MINGW
	#include <openvr_mingw.h>
#else
	#include <openvr.h>
#endif

namespace ext {
	namespace openvr {
		struct Driver {
			std::string name;
			std::string serial;
			vr::TrackedDevicePose_t poses[vr::k_unMaxTrackedDeviceCount];
			std::string types[vr::k_unMaxTrackedDeviceCount];
			vr::IVRRenderModels* renderModels;
		};
		extern UF_API Driver driver;
		extern UF_API vr::IVRSystem* context;
		extern UF_API uint8_t renderPass;
		extern UF_API float width, height;

		bool UF_API initialize( int stage = 0 );
		void UF_API tick();
		void UF_API terminate();
		void UF_API submit();
		void UF_API recommendedResolution( uint32_t& width, uint32_t& height );

		pod::Matrix4t<> UF_API positionMatrix();
		pod::Matrix4t<> UF_API projectionMatrix( vr::Hmd_Eye, float = 0.1f, float = 1024.0f );
		pod::Matrix4t<> UF_API eyeMatrix( vr::Hmd_Eye );
	}
}