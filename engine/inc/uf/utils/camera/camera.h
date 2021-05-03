#pragma once

#include <uf/config.h>
#include <uf/utils/math/vector.h>
#include <uf/utils/math/matrix.h>
#include <uf/utils/math/quaternion.h>
#include <uf/utils/math/transform.h>

namespace uf {
	class UF_API Camera {
	protected:
		bool m_modified;
		// ortho camera uses { size.x, size.y, tops.x, tops.y, bounds.x, bounds.y } => { left, right, bottom, top, near, far }
		struct {
			struct {
				pod::Vector2 lr, bt, nf;
			} ortho;
			struct {
				pod::Math::num_t fov;
				pod::Vector2 size ;
				pod::Vector2 bounds;
			} perspective;
			int mode;
			pod::Vector3 offset;
			bool stereoscopic = false;
		} m_settings;
		struct {
		/*
			struct {
				pod::Matrix4 view, projection;
			} left, right;
			pod::Matrix4 model;
		*/
			std::vector<pod::Matrix4f> views;
			std::vector<pod::Matrix4f> projections;
		} m_matrices;
		pod::Transform<> m_transform;
	public:
		static bool USE_REVERSE_INFINITE_PROJECTION;
		
		Camera();

		bool modified() const;
		void setStereoscopic( bool );
		pod::Transform<>& getTransform();
		const pod::Transform<>& getTransform() const;

		pod::Matrix4& getView( size_t = 0 );
		const pod::Matrix4& getView( size_t = 0 ) const;

		pod::Matrix4& getProjection( size_t = 0 );
		const pod::Matrix4& getProjection( size_t = 0 ) const;

		pod::Vector3f getEye( size_t = 0 ) const;

		pod::Math::num_t& getFov();
		pod::Math::num_t getFov() const;

		pod::Vector2& getSize();
		const pod::Vector2& getSize() const;

		pod::Vector2& getBounds();
		const pod::Vector2& getBounds() const;

		pod::Vector3& getOffset();
		const pod::Vector3& getOffset() const;

		void setTransform( const pod::Transform<>& transform );
		
		void setView( const pod::Matrix4& mat, size_t = SIZE_MAX );
		void setProjection( const pod::Matrix4& mat, size_t = SIZE_MAX );
	//	void setModel( const pod::Matrix4& mat );


		void setFov( pod::Math::num_t fov );
		void setSize( const pod::Vector2& );
		void setBounds( const pod::Vector2& );

		void ortho( const pod::Vector2&, const pod::Vector2&, const pod::Vector2& );
		
		void setOffset( const pod::Vector3& );

		void update(bool override = false);
		void updateView();
		void updateProjection();
	};
}