#pragma once

#include <uf/config.h>

#include <sstream>
#include <vector>
#include <cmath>
#include <stdint.h>

#include <uf/utils/math/angle.h>
#include <uf/utils/math/vector.h>
#include <uf/utils/math/quaternion.h>
#include <uf/utils/math/matrix.h>
#include <uf/utils/time/time.h>
#include <uf/utils/io/iostream.h>
#ifdef UF_USE_GLM
	#include "glm.h"
#endif
#include "math.h"
namespace pod {
	// Simple transforms (designed [to store in arrays] with minimal headaches)
	template<typename T = pod::Math::num_t>
	struct /*UF_API*/ Transform {
		typedef T type_t;

		pod::Vector3t<T> position;
		pod::Vector3t<T> scale = {1, 1, 1};

		pod::Vector3t<T> up = {0, 1, 0};
		pod::Vector3t<T> right = {1, 0, 0};
		pod::Vector3t<T> forward = {0, 0, 1};
		pod::Quaternion<T> orientation = {0, 0, 0, 1};

		pod::Matrix4t<T> model = uf::matrix::identity();

		pod::Transform<T>* reference = NULL;
	};
}

namespace uf {
	namespace transform {
		template<typename T> pod::Transform<T>& /*UF_API*/ initialize( pod::Transform<T>& transform ) {
			transform.up = {0, 1, 0};
			transform.right = {1, 0, 0};
			transform.forward = {0, 0, 1};
			transform.orientation = {0, 0, 0, 1};
			transform.model = uf::matrix::identity();
			return transform;
		}
		template<typename T> pod::Transform<T> /*UF_API*/ initialize() {
			pod::Transform<T> transform;
			return transform = uf::transform::initialize(transform);
		}
		template<typename T> pod::Transform<T>& /*UF_API*/ lookAt( pod::Transform<T>& transform, const pod::Vector3t<T>& at ) {
			pod::Vector3t<T> forward = uf::vector::normalize( at - transform.position );
			pod::Vector3t<T> right = uf::vector::normalize(uf::vector::cross( forward, transform.up ));
			pod::Vector3t<T> up = uf::vector::normalize(uf::vector::cross(at, right));

			transform.up = up;
			transform.right = right;
			transform.forward = forward;

			transform.orientation = uf::quaternion::lookAt( transform.position, at );
			return transform;
		}
		template<typename T> pod::Transform<T>& /*UF_API*/ move( pod::Transform<T>& transform, const pod::Vector3t<T>& axis, pod::Math::num_t delta ) {
			transform.position += (axis * delta);
			return transform;
		}
		template<typename T> pod::Transform<T>& /*UF_API*/ move( pod::Transform<T>& transform, const pod::Vector3t<T>& delta ) {
			transform.position += delta;
			return transform;
		}
		template<typename T> pod::Transform<T>& /*UF_API*/ reorient( pod::Transform<T>& transform ) {
		/*
			transform.up = uf::vector::normalize(uf::quaternion::rotate(transform.orientation, pod::Vector3{0.0f, 1.0f, 0.0f}));
			transform.right = uf::vector::normalize(uf::quaternion::rotate(transform.orientation, pod::Vector3{1.0f, 0.0f, 0.0f}));
			transform.forward = uf::vector::normalize(uf::quaternion::rotate(transform.orientation, pod::Vector3{0.0f, 0.0f, 1.0f}));
		*/
			pod::Quaternion<T> q = transform.orientation;
			transform.forward = { 2 * (q.x * q.z + q.w * q.y),  2 * (q.y * q.x - q.w * q.x), 1 - 2 * (q.x * q.x + q.y * q.y) };
			transform.up = { 2 * (q.x * q.y - q.w * q.z), 1 - 2 * (q.x * q.x + q.z * q.z), 2 * (q.y * q.z + q.w * q.x)};
			transform.right = { 1 - 2 * (q.y * q.y + q.z * q.z), 2 * (q.x * q.y + q.w * q.z), 2 * (q.x * q.z - q.w * q.y)};
			return transform;
		}
		template<typename T> pod::Transform<T>& /*UF_API*/ rotate( pod::Transform<T>& transform, const pod::Vector3t<T>& axis, pod::Math::num_t delta ) {
			pod::Quaternion<> quat = uf::quaternion::axisAngle( axis, delta );
			
			transform.orientation = uf::vector::normalize(uf::quaternion::multiply(transform.orientation, quat));
			transform = uf::transform::reorient(transform);

			return transform;
		}
		template<typename T> pod::Transform<T>& /*UF_API*/ rotate( pod::Transform<T>& transform, const pod::Quaternion<T>& quat ) {		
			transform.orientation = uf::vector::normalize(uf::quaternion::multiply(transform.orientation, quat));
			transform = uf::transform::reorient(transform);

			return transform;
		}
		template<typename T> pod::Transform<T>& /*UF_API*/ scale( pod::Transform<T>& transform, const pod::Vector3t<T>& factor ) {
			transform.scale = factor;
			return transform;
		}
		template<typename T> pod::Transform<T> /*UF_API*/ flatten( const pod::Transform<T>& transform, bool invert = false) {
			if ( !transform.reference ) return transform;
			pod::Transform<T> combined;
			const pod::Transform<T>* pointer = &transform;
			while ( pointer ) {
			//	if ( invert ) combined.position -= pointer->position; else combined.position += pointer->position;
				combined.position = combined.position + pointer->position;
				combined.orientation = invert ? uf::quaternion::multiply( pointer->orientation, combined.orientation ) : uf::quaternion::multiply( combined.orientation, pointer->orientation );
				pointer = pointer->reference;
			}
			return combined = uf::transform::reorient(combined);
		}
		template<typename T> pod::Matrix4t<T> /*UF_API*/ model( const pod::Transform<T>& transform, bool flatten = true ) {
			if ( flatten ) {
				uf::Matrix4t<T> translation, rotation, scale;
				pod::Transform<T> flatten = uf::transform::flatten(transform, false);
				// flatten.orientation.w *= -1;
				scale = uf::matrix::scale( scale, transform.scale );
				rotation = uf::quaternion::matrix(flatten.orientation);
				translation = uf::matrix::translate( uf::matrix::identity(), flatten.position );
				return translation * rotation * scale;
			}
			{
				std::vector<pod::Matrix4t<T>> models;
				pod::Matrix4t<T> translation, rotation, scale;
				const pod::Transform<T>* pointer = &transform;
				while ( pointer ) {					
					rotation = uf::quaternion::matrix( pointer->orientation);
					scale = uf::matrix::scale( scale, pointer->scale );
					translation = uf::matrix::translate( uf::matrix::identity(),  pointer->position );

					models.insert(models.begin(), translation * rotation * scale);

					pointer = pointer->reference;
				}
				pod::Matrix4t<T> model;
				for ( auto& matrix : models ) model = matrix * model;
				return model;
			}
		}
		template<typename T> pod::Transform<T> fromMatrix( const pod::Matrix4t<T>& matrix ) {
			pod::Transform<T> transform;
			transform.position = uf::matrix::multiply<float>( matrix, pod::Vector3f{ 0, 0, 0 } );
			transform.orientation = uf::quaternion::fromMatrix( matrix );
			return transform = reorient( transform );
		}
	}
}