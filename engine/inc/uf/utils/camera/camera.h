#pragma once

#include <uf/config.h>
#include <uf/utils/math/vector.h>
#include <uf/utils/math/matrix.h>
#include <uf/utils/math/quaternion.h>
#include <uf/utils/math/transform.h>

namespace uf {
	namespace camera {
		extern UF_API bool reverseInfiniteProjection;
		constexpr const size_t maxViews = 6;
	}
}

namespace pod {
	struct UF_API Camera {
		pod::Transform<> transform;		
		struct Viewports {
			struct Matrices{
				pod::Matrix4f view;
				pod::Matrix4f projection;
			} matrices[uf::camera::maxViews];
			size_t views = uf::camera::maxViews;
		} viewport;

		bool stereoscopic = false;
	};
}

namespace uf {
	namespace camera {
		pod::Vector3f UF_API eye( const pod::Camera&, size_t = 0 );
		void UF_API view( pod::Camera&, const pod::Matrix4&, size_t = uf::camera::maxViews );
		void UF_API projection( pod::Camera&, const pod::Matrix4&, size_t = uf::camera::maxViews );
		void UF_API update( pod::Camera& );
	}
}

namespace uf {
	class UF_API Camera : protected pod::Camera {
	public:
		Camera();

		pod::Camera& data();
		const pod::Camera& data() const;

		pod::Transform<>& getTransform();
		const pod::Transform<>& getTransform() const;
		void setTransform( const pod::Transform<>& transform );

		pod::Matrix4& getView( size_t = 0 );
		const pod::Matrix4& getView( size_t = 0 ) const;

		pod::Matrix4& getProjection( size_t = 0 );
		const pod::Matrix4& getProjection( size_t = 0 ) const;

		pod::Matrix4& getPrevious( size_t = 0 );
		const pod::Matrix4& getPrevious( size_t = 0 ) const;

		pod::Vector3f getEye( size_t = 0 ) const;
		void setStereoscopic( bool );		

		void setView( const pod::Matrix4& mat, size_t = uf::camera::maxViews );
		void setProjection( const pod::Matrix4& mat, size_t = uf::camera::maxViews );
		void update();
	};
}