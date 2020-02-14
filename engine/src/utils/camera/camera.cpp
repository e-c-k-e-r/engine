#include <uf/utils/camera/camera.h>

#include <uf/utils/renderer/renderer.h>
#include <uf/ext/openvr/openvr.h>

bool uf::Camera::USE_REVERSE_INFINITE_PROJECTION = true;\

uf::Camera::Camera() : m_modified(false) {
	this->m_settings.perspective.fov = 100;
	this->m_settings.perspective.size = {1280, 720};
	this->m_settings.perspective.bounds = {0.001, 32};
	this->m_settings.offset = {0, 0, 0};
	this->m_settings.mode = 1;
	
//	this->setModel(uf::matrix::identity());
//	this->setView(uf::matrix::identity());
//	this->setProjection(uf::matrix::identity());
	this->m_matrices.views.resize(6, uf::matrix::identity());
	this->m_matrices.projections.resize(6, uf::matrix::identity());
	
	this->m_transform = uf::transform::initialize(this->m_transform);
	this->m_transform.position = {0, 0, 0};
}

bool uf::Camera::modified() const {
	return !this->m_modified;
}
pod::Transform<>& uf::Camera::getTransform() {
	return this->m_transform;
}
const pod::Transform<>& uf::Camera::getTransform() const {
	return this->m_transform;
}

pod::Matrix4& uf::Camera::getView( size_t eye ) {
	if ( eye >= this->m_matrices.views.size() ) eye = 0;
	return this->m_matrices.views[eye];
//	if ( eye >= this->m_matrices.views.size() ) return uf::matrix::identity();
//	if ( eye >= this->m_matrices.views.size() ) this->m_matrices.views.resize( uf::matrix::identity() );
/*
	switch ( eye ) {
		case 0:
			return this->m_matrices.left.view;
		break;
		case 1:
			return this->m_matrices.right.view;
		break;
		default:
			return this->m_matrices.left.view;
		break;
	}
*/
}
pod::Matrix4& uf::Camera::getProjection( size_t eye ) {
	if ( eye >= this->m_matrices.projections.size() ) eye = 0;
	return this->m_matrices.projections[eye];
//	if ( eye >= this->m_matrices.projections.size() ) return uf::matrix::identity();
//	if ( eye >= this->m_matrices.projections.size() ) this->m_matrices.projections.resize( eye, uf::matrix::identity() );
/*
	switch ( eye ) {
		case 0:
			return this->m_matrices.left.projection;
		break;
		case 1:
			return this->m_matrices.right.projection;
		break;
		default:
			return this->m_matrices.left.projection;
		break;
	}
*/
}
/*
pod::Matrix4& uf::Camera::getModel() {
	return this->m_matrices.model;
}
*/
const pod::Matrix4& uf::Camera::getView( size_t eye ) const {
	if ( eye >= this->m_matrices.views.size() ) eye = 0;
	return this->m_matrices.views[eye];
//	if ( eye >= this->m_matrices.views.size() ) return uf::matrix::identity();
//	if ( eye >= this->m_matrices.views.size() ) this->m_matrices.views.resize( eye, uf::matrix::identity() );
/*
	switch ( eye ) {
		case 0:
			return this->m_matrices.left.view;
		break;
		case 1:
			return this->m_matrices.right.view;
		break;
		default:
			return this->m_matrices.left.view;
		break;
	}
*/
}
const pod::Matrix4& uf::Camera::getProjection( size_t eye ) const {
	if ( eye >= this->m_matrices.projections.size() ) eye = 0;
	return this->m_matrices.projections[eye];
//	if ( eye >= this->m_matrices.projections.size() ) return uf::matrix::identity();
//	if ( eye >= this->m_matrices.projections.size() ) this->m_matrices.projections.resize( eye, uf::matrix::identity() );
/*
	switch ( eye ) {
		case 0:
			return this->m_matrices.left.projection;
		break;
		case 1:
			return this->m_matrices.right.projection;
		break;
		default:
			return this->m_matrices.left.projection;
		break;
	}
*/
}
/*
const pod::Matrix4& uf::Camera::getModel() const {
	return this->m_matrices.model;
}
*/
pod::Math::num_t& uf::Camera::getFov() {
	return this->m_settings.perspective.fov;
}
pod::Math::num_t uf::Camera::getFov() const {
	return this->m_settings.perspective.fov;
}

pod::Vector2& uf::Camera::getSize() {
	return this->m_settings.perspective.size;
}
const pod::Vector2& uf::Camera::getSize() const {
	return this->m_settings.perspective.size;
}

pod::Vector2& uf::Camera::getBounds() {
	return this->m_settings.perspective.bounds;
}
const pod::Vector2& uf::Camera::getBounds() const {
	return this->m_settings.perspective.bounds;
}

pod::Vector3& uf::Camera::getOffset() {
	return this->m_settings.offset;
}
const pod::Vector3& uf::Camera::getOffset() const {
	return this->m_settings.offset;
}

void uf::Camera::setTransform( const pod::Transform<>& transform ) {
	this->m_transform = transform;
	this->update(true);
}
void uf::Camera::setView( const pod::Matrix4& mat, size_t i ) {
	if ( i >= uf::renderer::settings::maxViews ) {
		for ( i = 0; i < this->m_matrices.views.size(); ++i )
			this->m_matrices.views[i] = mat;
		return;
	}
	if ( i >= this->m_matrices.views.size() ) this->m_matrices.views.resize( i, uf::matrix::identity() );
	this->m_matrices.views[i] = mat;
/*
	switch ( i ) {
		case 0:
			this->m_matrices.left.view = mat;
		break;
		case 1:
			this->m_matrices.right.view = mat;
		break;
		default:
			this->setView( mat, 0 );
			this->setView( mat, 1 );
		break;
	}
*/
}
void uf::Camera::setProjection( const pod::Matrix4& mat, size_t i ) {
	if ( i >= uf::renderer::settings::maxViews ) {
		for ( i = 0; i < this->m_matrices.projections.size(); ++i )
			this->m_matrices.projections[i] = mat;
		return;
	}
	if ( i >= this->m_matrices.projections.size() ) this->m_matrices.projections.resize( i, uf::matrix::identity() );
	this->m_matrices.projections[i] = mat;
/*
	switch ( i ) {
		case 0:
			this->m_matrices.left.projection = mat;
		break;
		case 1:
			this->m_matrices.right.projection = mat;
		break;
		default:
			this->setProjection( mat, 0 );
			this->setProjection( mat, 1 );
		break;
	}
*/
}
/*
void uf::Camera::setModel( const pod::Matrix4& mat ) {
	this->m_matrices.model = mat;
}
*/
void uf::Camera::setFov( pod::Math::num_t fov ) {
	this->m_settings.mode = 1;
	this->m_settings.perspective.fov = fov;
	this->update(true);
}
void uf::Camera::setSize( const pod::Vector2& size ) {
	this->m_settings.perspective.size = size;
	this->update(true);
}
void uf::Camera::setBounds( const pod::Vector2& bounds ) {
	this->m_settings.mode = 1;
	this->m_settings.perspective.bounds = bounds;
	this->update(true);
}
void uf::Camera::setOffset( const pod::Vector3& offset ) {
	this->m_settings.offset = offset;
	this->update(true);
}

void uf::Camera::ortho( const pod::Vector2& lr, const pod::Vector2& bt, const pod::Vector2& nf ) {
	this->m_settings.mode = -1;
	this->m_settings.ortho.lr = lr;
	this->m_settings.ortho.bt = bt;
	this->m_settings.ortho.nf = nf;

	this->update(true);
}
void uf::Camera::setStereoscopic(bool settings) {
	this->m_settings.stereoscopic = settings;
}
void uf::Camera::update(bool override) {
	if ( !override && !this->modified() ) return;
	this->updateView();
	this->updateProjection();
	this->m_modified = true;
}
void uf::Camera::updateView() {
	auto& transform = this->getTransform();
#if UF_USE_OPENVR
	if ( this->m_settings.stereoscopic && ext::openvr::context ) {
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
}
void uf::Camera::updateProjection() {
#if UF_USE_OPENVR
	if ( this->m_settings.stereoscopic && ext::openvr::context ) {
		this->setProjection( ext::openvr::hmdProjectionMatrix( vr::Eye_Left, this->m_settings.perspective.bounds.x, this->m_settings.perspective.bounds.y ), 0 );
		this->setProjection( ext::openvr::hmdProjectionMatrix( vr::Eye_Right, this->m_settings.perspective.bounds.x, this->m_settings.perspective.bounds.y ), 1 );
		return;
	}
#endif
	if ( this->m_settings.mode < 0 ) {
		// Maintain aspect ratio
		if ( this->m_settings.ortho.lr.x == this->m_settings.ortho.bt.x && this->m_settings.ortho.lr.y == this->m_settings.ortho.bt.y && this->m_settings.ortho.bt.x == -this->m_settings.ortho.bt.y ) {
			pod::Math::num_t raidou = this->m_settings.perspective.size.x / this->m_settings.perspective.size.y;
			pod::Math::num_t range = this->m_settings.ortho.bt.y - this->m_settings.ortho.bt.x;
			pod::Math::num_t correction = raidou * range / 2.0;
			this->setProjection(uf::matrix::ortho(
				-correction, correction,
				this->m_settings.ortho.bt.x, this->m_settings.ortho.bt.y, 
				this->m_settings.ortho.nf.x, this->m_settings.ortho.nf.y
			));
			return;
		}
		this->setProjection(uf::matrix::ortho(
			this->m_settings.ortho.lr.x, this->m_settings.ortho.lr.y, 
			this->m_settings.ortho.bt.x, this->m_settings.ortho.bt.y, 
			this->m_settings.ortho.nf.x, this->m_settings.ortho.nf.y
		));
		return;
	}
	float fov = this->m_settings.perspective.fov * (3.14159265358f / 180.0f);
	float raidou = (float) this->m_settings.perspective.size.x / (float) this->m_settings.perspective.size.y;
	float f = 1.0f / tan( 0.5f * fov );
	
	this->setProjection({
		f / raidou, 	0.0f, 	 0.0f, 	0.0f,
		0.0f, 			-f, 	 0.0f, 	0.0f,
		0.0f,       	0.0f,    0.0f, 	1.0f,
		0.0f,       	0.0f,   this->m_settings.perspective.bounds.x, 	0.0f
	});
}