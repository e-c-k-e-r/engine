#pragma once

#include <uf/config.h>
#include <uf/utils/math/shapes.h>
#include <uf/utils/math/vector.h>
#include <uf/utils/math/transform.h>

namespace uf {
	namespace debug {
		void UF_API drawLine( const pod::Vector3f& start, const pod::Vector3f& end, const pod::Vector4f& color = { 1, 1, 1, 1 } );
		void UF_API drawAabb( pod::AABB aabb, pod::Transform<> transform = {} );
		void UF_API drawObb( pod::OBB obb, pod::Transform<> transform = {} );
		void UF_API drawSphere( pod::Sphere sphere, pod::Transform<> transform = {} );
		void UF_API drawCapsule( pod::Capsule capsule, pod::Transform<> transform = {} );
		void UF_API drawPlane( pod::Plane plane, pod::Transform<> transform = {} );
		void UF_API drawTriangle( pod::Triangle tri, pod::Transform<> transform = {} );
		
		void UF_API addLine( const pod::Vector3f& start, const pod::Vector3f& end, const pod::Vector4f& color = { 1, 1, 1, 1 }, float ttl = 1.0f );
		void UF_API draw( float dt = 0 );
	}
}