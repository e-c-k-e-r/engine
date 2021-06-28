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
	if ( i >= uf::camera::maxViews ) {
		for ( i = 0; i < uf::camera::maxViews; ++i ) camera.viewport.matrices[i].view = mat;
		return;
	}
	camera.viewport.matrices[i].view = mat;
}
void uf::camera::projection( pod::Camera& camera, const pod::Matrix4f& mat, uint_fast8_t i ) {
	if ( i >= uf::camera::maxViews ) {
		for ( i = 0; i < uf::camera::maxViews; ++i ) camera.viewport.matrices[i].projection = mat;
		return;
	}
	camera.viewport.matrices[i].projection = mat;
}
void uf::camera::update( pod::Camera& camera, bool override ) {
	if ( !override && !camera.modified ) return;
#if UF_USE_OPENVR
	if ( this->m_pod.stereoscopic && ext::openvr::context ) {
		camera.transform.orientation = uf::quaternion::identity();
		pod::Matrix4t<> view = uf::matrix::inverse( uf::transform::model( camera.transform, false, 1 ) );
		camera.transform.orientation = ext::openvr::hmdQuaternion();

		uf::camera::view( camera, ext::openvr::hmdViewMatrix(vr::Eye_Left, view ), 0 );
		uf::camera::view( camera, ext::openvr::hmdViewMatrix(vr::Eye_Right, view ), 1 );
	} else {
#else
	{
#endif
		pod::Matrix4t<> view = uf::matrix::inverse( uf::transform::model( camera.transform, false, 1 ) );
		uf::camera::view( camera, view );
	}
	camera.modified = false;
}

//
uf::Camera::Camera() {
	this->m_pod.modified = true;
	this->m_pod.stereoscopic = true;
	
	this->m_pod.transform = uf::transform::initialize(this->m_pod.transform);
	for ( uint_fast8_t i = 0; i < uf::camera::maxViews; ++i ) {
		this->m_pod.viewport.matrices[i].view = uf::matrix::identity();
		this->m_pod.viewport.matrices[i].projection = uf::matrix::identity();
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

bool uf::Camera::modified() const { return this->m_pod.modified; }
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

#if 0
bool uf::Camera::modified() const {
	return this->m_pod.modified;
}
pod::Transform<>& uf::Camera::getTransform() {
	return this->m_pod.transform;
}
const pod::Transform<>& uf::Camera::getTransform() const {
	return this->m_pod.transform;
}

pod::Matrix4& uf::Camera::getView( size_t eye ) {
	if ( eye >= this->m_pod.matrices.views.size() ) eye = 0;
	return this->m_pod.matrices.views[eye];
}
pod::Matrix4& uf::Camera::getProjection( size_t eye ) {
	if ( eye >= this->m_pod.matrices.projections.size() ) eye = 0;
	return this->m_pod.matrices.projections[eye];
}
const pod::Matrix4& uf::Camera::getView( size_t eye ) const {
	if ( eye >= this->m_pod.matrices.views.size() ) eye = 0;
	return this->m_pod.matrices.views[eye];
}
const pod::Matrix4& uf::Camera::getProjection( size_t eye ) const {
	if ( eye >= this->m_pod.matrices.projections.size() ) eye = 0;
	return this->m_pod.matrices.projections[eye];
}

pod::Vector3f uf::Camera::getEye( size_t eye ) const {
	pod::Vector3f position = uf::transform::flatten( this->m_pod.transform ).position;
#if UF_USE_OPENVR
	if ( this->m_pod.stereoscopic && ext::openvr::context ) {
		position += ext::openvr::hmdPosition( eye == 0 ? vr::Eye_Left : vr::Eye_Right );
	}
#endif
	return position;
}

void uf::Camera::setTransform( const pod::Transform<>& transform ) {
	this->m_pod.transform = transform;
	this->update(true);
}
void uf::Camera::setView( const pod::Matrix4& mat, size_t i ) {
	if ( i >= uf::camera::maxViews ) {
		for ( i = 0; i < this->m_pod.matrices.views.size(); ++i )
			this->m_pod.matrices.views[i] = mat;
		return;
	}
	if ( i >= this->m_pod.matrices.views.size() ) this->m_pod.matrices.views.resize( i, uf::matrix::identity() );
	this->m_pod.matrices.views[i] = mat;
}
void uf::Camera::setProjection( const pod::Matrix4& mat, size_t i ) {
	if ( i >= uf::camera::maxViews ) {
		for ( i = 0; i < this->m_pod.matrices.projections.size(); ++i )
			this->m_pod.matrices.projections[i] = mat;
		return;
	}
	if ( i >= this->m_pod.matrices.projections.size() ) this->m_pod.matrices.projections.resize( i, uf::matrix::identity() );
	this->m_pod.matrices.projections[i] = mat;
}

void uf::Camera::setStereoscopic(bool settings) {
	this->m_pod.stereoscopic = settings;
}
void uf::Camera::update(bool override) {
/*
	if ( !override && !this->modified() ) return;
	this->updateView();
	this->updateProjection();
	this->m_pod.modified = true;
*/
}
void uf::Camera::updateView() {
	uf::camera::updateView( this->m_pod );
/*
	auto& transform = this->getTransform();
#if UF_USE_OPENVR
	if ( this->m_pod.stereoscopic && ext::openvr::context ) {
		transform.orientation = uf::quaternion::identity();
		pod::Matrix4t<> view = uf::matrix::inverse( uf::transform::model( transform, false, 1 ) );
		transform.orientation = ext::openvr::hmdQuaternion();

		this->setView( ext::openvr::hmdViewMatrix(vr::Eye_Left, view ), 0 );
		this->setView( ext::openvr::hmdViewMatrix(vr::Eye_Right, view ), 1 );
	} else {
#else
	{
#endif
		pod::Matrix4t<> view = uf::matrix::inverse( uf::transform::model( transform, false, 1 ) );
		this->setView( view );
	}
*/
}
void uf::Camera::updateProjection() {
/*
#if UF_USE_OPENVR
	if ( this->m_pod.stereoscopic && ext::openvr::context ) {
		this->setProjection( ext::openvr::hmdProjectionMatrix( vr::Eye_Left, this->m_pod.settings.perspective.bounds.x, this->m_pod.settings.perspective.bounds.y ), 0 );
		this->setProjection( ext::openvr::hmdProjectionMatrix( vr::Eye_Right, this->m_pod.settings.perspective.bounds.x, this->m_pod.settings.perspective.bounds.y ), 1 );
		return;
	}
#endif
	if ( this->m_pod.settings.mode < 0 ) {
		// Maintain aspect ratio
		if ( this->m_pod.settings.ortho.lr.x == this->m_pod.settings.ortho.bt.x && this->m_pod.settings.ortho.lr.y == this->m_pod.settings.ortho.bt.y && this->m_pod.settings.ortho.bt.x == -this->m_pod.settings.ortho.bt.y ) {
			float raidou = this->m_pod.settings.perspective.size.x / this->m_pod.settings.perspective.size.y;
			float range = this->m_pod.settings.ortho.bt.y - this->m_pod.settings.ortho.bt.x;
			float correction = raidou * range / 2.0;
			this->setProjection(uf::matrix::ortho(
				-correction, correction,
				this->m_pod.settings.ortho.bt.x, this->m_pod.settings.ortho.bt.y, 
				this->m_pod.settings.ortho.nf.x, this->m_pod.settings.ortho.nf.y
			));
			return;
		}
		this->setProjection(uf::matrix::ortho(
			this->m_pod.settings.ortho.lr.x, this->m_pod.settings.ortho.lr.y, 
			this->m_pod.settings.ortho.bt.x, this->m_pod.settings.ortho.bt.y, 
			this->m_pod.settings.ortho.nf.x, this->m_pod.settings.ortho.nf.y
		));
		return;
	}
	float zNear = this->m_pod.settings.perspective.bounds.x;
	float zFar = this->m_pod.settings.perspective.bounds.y;
	float fov = this->m_pod.settings.perspective.fov * (3.14159265358f / 180.0f);
	float raidou = (float) this->m_pod.settings.perspective.size.x / (float) this->m_pod.settings.perspective.size.y;
	if ( uf::camera::reverseInfiniteProjection ) {
		float f = 1.0f / tan( 0.5f * fov );
	#if UF_USE_OPENGL
		this->setProjection({
			f / raidou, 	0.0f, 	 0.0f, 	0.0f,
			0.0f, 			 f, 	 0.0f, 	0.0f,
			0.0f,       	0.0f,    0.0f, 	1.0f,
			0.0f,       	0.0f,   zNear, 	0.0f
		});
	#elif UF_USE_VULKAN
		this->setProjection({
			f / raidou, 	0.0f, 	 0.0f, 	0.0f,
			0.0f, 			-f, 	 0.0f, 	0.0f,
			0.0f,       	0.0f,    0.0f, 	1.0f,
			0.0f,       	0.0f,   zNear, 	0.0f
		});
	#endif
	} else {
		float range = zNear - zFar;
		float f = tanf( fov / 2.0 );

		float Sx = 1.0 / (f * raidou);
		float Sy = 1.0 / f;
		float Sz = (-zNear - zFar) / range;
		float Pz = 2.0 * zFar * zNear / range;
	#if UF_USE_VULKAN
		Sy = -Sy;
	#endif
		this->setProjection({
			Sx, 	 0, 	 0, 	  0,
			 0, 	Sy, 	 0, 	  0,
			 0, 	 0, 	Sz, 	  1,
			 0, 	 0, 	Pz, 	  0
		});
	}
*/
}
#endif