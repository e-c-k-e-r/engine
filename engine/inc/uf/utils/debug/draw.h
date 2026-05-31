#pragma once

#include <uf/config.h>
#include <uf/utils/math/shapes.h>
#include <uf/utils/math/vector.h>
#include <uf/utils/math/transform.h>

namespace uf {
	namespace debug {
		void UF_API drawLine( const pod::Vector3f& start, const pod::Vector3f& end, const pod::Vector4f& color = { 1, 1, 1, 1 } );

		void UF_API drawShape( const pod::AABB& aabb, const pod::Transform<>& transform = {} );
		void UF_API drawShape( const pod::OBB& obb, const pod::Transform<>& transform = {} );
		void UF_API drawShape( const pod::Sphere& sphere, const pod::Transform<>& transform = {} );
		void UF_API drawShape( const pod::Capsule& capsule, const pod::Transform<>& transform = {} );
		void UF_API drawShape( const pod::Plane& plane, const pod::Transform<>& transform = {} );
		void UF_API drawShape( const pod::Triangle& tri, const pod::Transform<>& transform = {} );
		
		void UF_API addLine( const pod::Vector3f& start, const pod::Vector3f& end, const pod::Vector4f& color = { 1, 1, 1, 1 }, float ttl = 1.0f );
		void UF_API drawLines( float dt = 0 );
		
		void UF_API drawText( const uf::stl::string& string, const pod::Vector3f& position, const pod::Vector4f& color = { 1, 1, 1, 1 } );
		void UF_API drawTexts( float dt = 0 );
		void UF_API draw( float dt = 0 );
	}
}