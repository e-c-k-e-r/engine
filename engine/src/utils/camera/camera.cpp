#include <uf/utils/camera/camera.h>

#include <uf/utils/renderer/renderer.h>
#include <uf/ext/openvr/openvr.h>

#if UF_ENV_DREAMCAST
	bool uf::matrix::reverseInfiniteProjection = false;
#else
	bool uf::matrix::reverseInfiniteProjection = true;
#endif

pod::Vector3f uf::camera::eye( const pod::Camera& camera, int i ) {
	if ( i < 0 ) i = 0; 
	pod::Vector3f position = uf::transform::flatten( camera.transform ).position;
#if UF_USE_OPENVR
	if ( camera.stereoscopic && ext::openvr::enabled ) {
		position += ext::openvr::hmdPosition( i == 0 ? vr::Eye_Left : vr::Eye_Right );
	}
#endif
	return position;
}
void uf::camera::view( pod::Camera& camera, const pod::Matrix4f& mat, int i ) {
	if ( i < 0 ) i = uf::camera::maxViews; // camera.views;
	if ( i >= uf::camera::maxViews ) {
		for ( i = 0; i < uf::camera::maxViews; ++i ) {
			camera.viewport.matrices[i].view = mat;
		}
		return;
	}
	camera.viewport.matrices[i].view = mat;
}
void uf::camera::projection( pod::Camera& camera, const pod::Matrix4f& mat, int i ) {
	if ( i < 0 ) i = uf::camera::maxViews; // camera.views;
	if ( i >= uf::camera::maxViews ) {
		for ( i = 0; i < uf::camera::maxViews; ++i ) camera.viewport.matrices[i].projection = mat;
		return;
	}
	camera.viewport.matrices[i].projection = mat;
}
void uf::camera::update( pod::Camera& camera ) {
#if UF_USE_OPENVR
	if ( camera.stereoscopic && ext::openvr::enabled ) {
		auto view = uf::matrix::inverse( uf::transform::model( camera.transform ) );

		uf::camera::view( camera, ext::openvr::hmdViewMatrix(vr::Eye_Left, view ), 0 );
		uf::camera::view( camera, ext::openvr::hmdViewMatrix(vr::Eye_Right, view ), 1 );
	} else
#endif
	{
		auto view = uf::matrix::inverse( uf::transform::model( camera.transform ) );
		uf::camera::view( camera, view );
	}
}

//
uf::Camera::Camera() {
	this->stereoscopic = true;
	this->transform = uf::transform::initialize(this->transform);
	for ( auto i = 0; i < uf::camera::maxViews; ++i ) {
		this->viewport.matrices[i].view = uf::matrix::identity();
		this->viewport.matrices[i].projection = uf::matrix::identity();
	}
}

pod::Camera& uf::Camera::data() { return *this; }
const pod::Camera& uf::Camera::data() const { return *this; }

pod::Transform<>& uf::Camera::getTransform() { return this->transform; }
const pod::Transform<>& uf::Camera::getTransform() const { return this->transform; }
void uf::Camera::setTransform( const pod::Transform<>& transform ) { this->transform = transform; }

pod::Matrix4& uf::Camera::getView( size_t i ) { return this->viewport.matrices[MIN(i, uf::camera::maxViews - 1)].view; }
const pod::Matrix4& uf::Camera::getView( size_t i ) const { return this->viewport.matrices[MIN(i, uf::camera::maxViews - 1)].view; }
pod::Matrix4& uf::Camera::getProjection( size_t i ) { return this->viewport.matrices[MIN(i, uf::camera::maxViews - 1)].projection; }
const pod::Matrix4& uf::Camera::getProjection( size_t i ) const { return this->viewport.matrices[MIN(i, uf::camera::maxViews - 1)].projection; }

void uf::Camera::setStereoscopic( bool b ) { this->stereoscopic = b; }
pod::Vector3f uf::Camera::getEye( size_t i ) const {
	if ( i < 0 ) i = uf::camera::maxViews; // this->views;
	return uf::camera::eye( *this, i );
}
void uf::Camera::setView( const pod::Matrix4& mat, int i ) {
	if ( i < 0 ) i = uf::camera::maxViews; // this->views;
	uf::camera::view( *this, mat, i );
}
void uf::Camera::setProjection( const pod::Matrix4& mat, int i ) {
	if ( i < 0 ) i = uf::camera::maxViews; // this->views;
	uf::camera::projection( *this, mat, i );
}
void uf::Camera::update() {
	uf::camera::update( *this );
}