#include <uf/utils/camera/camera.h>

#include <uf/utils/renderer/renderer.h>
#include <uf/ext/openvr/openvr.h>

#if UF_ENV_DREAMCAST
	bool uf::matrix::reverseInfiniteProjection = false;
#else
	bool uf::matrix::reverseInfiniteProjection = true;
#endif
// const uint_fast8_t uf::camera::maxViews = 6;

pod::Vector3f uf::camera::eye( const pod::Camera& camera, uint_fast8_t i ) {
	pod::Vector3f position = uf::transform::flatten( camera.transform ).position;
#if UF_USE_OPENVR
	if ( camera.stereoscopic && ext::openvr::context ) {
		position += ext::openvr::hmdPosition( eye == 0 ? vr::Eye_Left : vr::Eye_Right );
	}
#endif
	return position;
}
void uf::camera::view( pod::Camera& camera, const pod::Matrix4f& mat, uint_fast8_t i ) {
	camera.ttl = uf::time::frame + 1;
	if ( i >= uf::camera::maxViews ) {
		for ( i = 0; i < uf::camera::maxViews; ++i ) {
//			camera.previous[i] = camera.viewport.matrices[i].projection * camera.viewport.matrices[i].view;
			camera.viewport.matrices[i].view = mat;
		}
		return;
	}
//	camera.previous[i] = camera.viewport.matrices[i].projection * camera.viewport.matrices[i].view;
	camera.viewport.matrices[i].view = mat;
}
void uf::camera::projection( pod::Camera& camera, const pod::Matrix4f& mat, uint_fast8_t i ) {
	camera.ttl = uf::time::frame + 1;
	if ( i >= uf::camera::maxViews ) {
		for ( i = 0; i < uf::camera::maxViews; ++i ) camera.viewport.matrices[i].projection = mat;
		return;
	}
	camera.viewport.matrices[i].projection = mat;
}
void uf::camera::update( pod::Camera& camera, bool override ) {
	if ( !override && uf::time::frame <= camera.ttl ) return; // skip updating the view matrices if it was already updated
#if UF_USE_OPENVR
	if ( this->m_pod.stereoscopic && ext::openvr::context ) {
		camera.transform.orientation = uf::quaternion::identity();
		pod::Matrix4t<> view = uf::matrix::inverse( uf::transform::model( camera.transform, false, 1 ) );
		camera.transform.orientation = ext::openvr::hmdQuaternion();

		uf::camera::view( camera, ext::openvr::hmdViewMatrix(vr::Eye_Left, view ), 0 );
		uf::camera::view( camera, ext::openvr::hmdViewMatrix(vr::Eye_Right, view ), 1 );
	} else
#endif
	{
		pod::Matrix4t<> view = uf::matrix::inverse( uf::transform::model( camera.transform, false, 1 ) );
		uf::camera::view( camera, view );
	}
}

//
uf::Camera::Camera() {
	this->m_pod.ttl = {};
	this->m_pod.stereoscopic = true;
	
	this->m_pod.transform = uf::transform::initialize(this->m_pod.transform);
	for ( uint_fast8_t i = 0; i < uf::camera::maxViews; ++i ) {
		this->m_pod.viewport.matrices[i].view = uf::matrix::identity();
		this->m_pod.viewport.matrices[i].projection = uf::matrix::identity();
		// this->m_pod.previous[i] = uf::matrix::identity();
	}
}

pod::Camera& uf::Camera::data() { return this->m_pod; }
const pod::Camera& uf::Camera::data() const { return this->m_pod; }

pod::Transform<>& uf::Camera::getTransform() { return this->m_pod.transform; }
const pod::Transform<>& uf::Camera::getTransform() const { return this->m_pod.transform; }
void uf::Camera::setTransform( const pod::Transform<>& transform ) { this->m_pod.transform = transform; }

pod::Matrix4& uf::Camera::getView( uint_fast8_t i ) { return this->m_pod.viewport.matrices[MIN(i, uf::camera::maxViews)].view; }
const pod::Matrix4& uf::Camera::getView( uint_fast8_t i ) const { return this->m_pod.viewport.matrices[MIN(i, uf::camera::maxViews)].view; }

pod::Matrix4& uf::Camera::getProjection( uint_fast8_t i ) { return this->m_pod.viewport.matrices[MIN(i, uf::camera::maxViews)].projection; }
const pod::Matrix4& uf::Camera::getProjection( uint_fast8_t i ) const { return this->m_pod.viewport.matrices[MIN(i, uf::camera::maxViews)].projection; }

// pod::Matrix4& uf::Camera::getPrevious( uint_fast8_t i ) { return this->m_pod.previous[MIN(i, uf::camera::maxViews)]; }
// const pod::Matrix4& uf::Camera::getPrevious( uint_fast8_t i ) const { return this->m_pod.previous[MIN(i, uf::camera::maxViews)]; }

bool uf::Camera::modified() const { return uf::time::frame <= this->m_pod.ttl; }
void uf::Camera::setStereoscopic( bool b ) { this->m_pod.stereoscopic = b; }
pod::Vector3f uf::Camera::getEye( uint_fast8_t i ) const {
	return uf::camera::eye( this->m_pod, i );
}
void uf::Camera::setView( const pod::Matrix4& mat, uint_fast8_t i ) {
	uf::camera::view( this->m_pod, mat, i );
}
void uf::Camera::setProjection( const pod::Matrix4& mat, uint_fast8_t i ) {
	uf::camera::projection( this->m_pod, mat, i );
}
void uf::Camera::update( bool override ) {
	uf::camera::update( this->m_pod, override );
}