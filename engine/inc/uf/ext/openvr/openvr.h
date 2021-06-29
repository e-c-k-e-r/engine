#pragma once

#include <uf/config.h>

#if UF_USE_OPENVR
#include <uf/utils/math/matrix.h>
#include <uf/utils/math/quaternion.h>
#include <uf/utils/graphic/graphic.h>
#include <uf/utils/serialize/serializer.h>

#ifdef USE_OPENVR_MINGW
	#include <openvr/openvr_mingw.hpp>
#else
	#include <openvr/openvr.h>
#endif

namespace vr {
	enum EVRHand {
		Hand_Left = 0,
		Hand_Right = 1
	};
	typedef EVRHand Controller_Hand;
}

namespace ext {
	namespace openvr {
		struct Driver {
			uf::stl::string name;
			uf::stl::string serial;
			uf::stl::string manifest;
			vr::TrackedDevicePose_t poses[vr::k_unMaxTrackedDeviceCount];
			uf::stl::string types[vr::k_unMaxTrackedDeviceCount];
			vr::IVRRenderModels* renderModels;
		};
		extern UF_API Driver driver;
		extern UF_API vr::IVRSystem* context;
		extern UF_API uint8_t renderPass;
		extern UF_API float width, height;
		extern UF_API bool enabled;
		extern UF_API bool swapEyes;
		extern UF_API uint8_t dominantEye;

		bool UF_API initialize( int stage = 0 );
		void UF_API tick();
		void UF_API terminate();
		void UF_API submit();
		void UF_API synchronize( bool async = true );
		float UF_API predictedTimeToDisplay( float additional = 1 );
		float UF_API updateTracking( float additional = 1 );
		void UF_API recommendedResolution( uint32_t& width, uint32_t& height );

		pod::Matrix4t<> UF_API hmdHeadPositionMatrix();
		pod::Matrix4t<> UF_API hmdEyePositionMatrix( vr::Hmd_Eye );

		pod::Matrix4t<> UF_API hmdViewTranslationMatrix();
		pod::Matrix4t<> UF_API hmdViewRotationMatrix();
		pod::Vector3f UF_API hmdPosition();
		pod::Vector3f UF_API hmdPosition( vr::Hmd_Eye eye );
		pod::Vector3f UF_API hmdEyePosition( vr::Hmd_Eye eye );
		pod::Quaternion<> UF_API hmdQuaternion();
		pod::Matrix4t<> UF_API hmdViewMatrix( vr::Hmd_Eye eye, const pod::Matrix4f& = uf::matrix::identity() );
		pod::Matrix4t<> UF_API hmdProjectionMatrix( vr::Hmd_Eye, float = 0.1f, float = 1024.0f );

		uf::Serializer UF_API controllerState( vr::Controller_Hand, const uf::stl::string& = "" );
		pod::Vector3f UF_API controllerPosition( vr::Controller_Hand, bool = false );
		pod::Quaternion<> UF_API controllerQuaternion( vr::Controller_Hand, bool = false );
		pod::Matrix4t<> UF_API controllerTranslationMatrix( vr::Controller_Hand, bool = false );
		pod::Matrix4t<> UF_API controllerRotationMatrix( vr::Controller_Hand, bool = false );
		pod::Matrix4t<> UF_API controllerModelMatrix( vr::Controller_Hand, bool = false );
		pod::Matrix4t<> UF_API controllerMatrix( vr::Controller_Hand, bool = false );
		bool UF_API controllerActive( vr::Controller_Hand );

		bool UF_API requestRenderModel( const uf::stl::string& );
		uf::Graphic& UF_API getRenderModel( const uf::stl::string& );
		uf::Graphic& UF_API controllerRenderModel( vr::Controller_Hand );

		void UF_API resetPosition();
	/*
		pod::Matrix4t<> UF_API positionMatrix();
		pod::Matrix4t<> UF_API eyeMatrix( vr::Hmd_Eye );
	*/
	}
}
#endif