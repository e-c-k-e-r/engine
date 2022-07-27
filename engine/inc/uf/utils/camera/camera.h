#pragma once

#include <uf/config.h>
#include <uf/utils/math/vector.h>
#include <uf/utils/math/matrix.h>
#include <uf/utils/math/quaternion.h>
#include <uf/utils/math/transform.h>

namespace uf {
	namespace camera {
		extern UF_API bool reverseInfiniteProjection;
		constexpr const uint_fast8_t maxViews = 6;
	}
}

namespace pod {
	struct UF_API Camera {
		bool modified = false;
		bool stereoscopic = false;
		pod::Transform<> transform;
		
		struct Viewports {
			struct Matrices{
				pod::Matrix4f view;
				pod::Matrix4f projection;
			} matrices[uf::camera::maxViews];
		} viewport;
		
		pod::Matrix4f previous[uf::camera::maxViews];

	/*
		struct Metadata {
			float fov;
			pod::Vector2f bounds;
		};
	*/
	};
}

namespace uf {
	namespace camera {
		pod::Vector3f UF_API eye( const pod::Camera&, uint_fast8_t = 0 );
		void UF_API view( pod::Camera&, const pod::Matrix4&, uint_fast8_t = 255 );
		void UF_API projection( pod::Camera&, const pod::Matrix4&, uint_fast8_t = 255 );
		void UF_API update( pod::Camera&, bool = true );
	}
}

namespace uf {
	class UF_API Camera {
	protected:
		pod::Camera m_pod;
	public:
		Camera();

		pod::Camera& data();
		const pod::Camera& data() const;

		pod::Transform<>& getTransform();
		const pod::Transform<>& getTransform() const;
		void setTransform( const pod::Transform<>& transform );

		pod::Matrix4& getView( uint_fast8_t = 0 );
		const pod::Matrix4& getView( uint_fast8_t = 0 ) const;

		pod::Matrix4& getProjection( uint_fast8_t = 0 );
		const pod::Matrix4& getProjection( uint_fast8_t = 0 ) const;

		pod::Matrix4& getPrevious( uint_fast8_t = 0 );
		const pod::Matrix4& getPrevious( uint_fast8_t = 0 ) const;

		bool modified() const;
		void setStereoscopic( bool );
		
		pod::Vector3f getEye( uint_fast8_t = 0 ) const;

		void setView( const pod::Matrix4& mat, uint_fast8_t = 255 );
		void setProjection( const pod::Matrix4& mat, uint_fast8_t = 255 );
		void update(bool = false);
	};
}