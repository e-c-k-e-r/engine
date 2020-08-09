#include <uf/utils/camera/camera.h>

#include <uf/ext/openvr/openvr.h>

namespace {
	struct {
		pod::Matrix4 left, right;
	} projection, eye;
	pod::Matrix4 head;
}

uf::Camera::Camera() : 
	m_modified(false)
{
	this->m_settings.perspective.fov = 100;
	this->m_settings.perspective.size = {1280, 720};
	this->m_settings.perspective.bounds = {0.001, 32};
	this->m_settings.offset = {0, 0, 0};
	this->m_settings.mode = 1;
	
	this->m_matrices.view = uf::matrix::identity();
	this->m_matrices.projection = uf::matrix::identity();
	
	this->m_transform = uf::transform::initialize(this->m_transform);
	this->m_transform.position = {0,1.725,0};
}
/*
uf::Camera::Camera( const pod::Math::num_t& fov, const pod::Vector2& size, const pod::Vector2& bounds, const pod::Vector3& offset, const pod::Vector2& tops ) : 
	m_modified(false),
	m_settings({.fov=fov, .size=size, .bounds=bounds, .offset=offset, .tops=tops})
{
	this->m_matrices.view = uf::matrix::identity();
	this->m_matrices.projection = uf::matrix::identity();
}
uf::Camera::Camera( Camera&& move ) : 
	m_modified(std::move(move.m_modified)),
	m_settings(std::move(move.m_settings)),
	m_matrices(std::move(move.m_matrices)),
	m_transform(std::move(move.m_transform))
{

}
uf::Camera::Camera( const Camera& copy ) :
	m_modified(copy.m_modified),
	m_settings(copy.m_settings),
	m_matrices(copy.m_matrices),
	m_transform(copy.m_transform)
{

}
*/

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
	if ( ext::openvr::context ) {
		return eye == 0 ? ::eye.left : ::eye.right;
	}
	return this->m_matrices.view;
}
pod::Matrix4& uf::Camera::getProjection( size_t eye ) {
	if ( ext::openvr::context ) {
		return eye == 0 ? ::projection.left : ::projection.right;
	}
	return this->m_matrices.projection;
}
pod::Matrix4& uf::Camera::getModel() {
	return this->m_matrices.model;
}

const pod::Matrix4& uf::Camera::getView( size_t eye ) const {
	if ( ext::openvr::context ) {
		return eye == 0 ? ::eye.left : ::eye.right;
	}
	return this->m_matrices.view;
}
const pod::Matrix4& uf::Camera::getProjection( size_t eye ) const {
	if ( ext::openvr::context ) {
		return eye == 0 ? ::projection.left : ::projection.right;
	}
	return this->m_matrices.projection;
}
const pod::Matrix4& uf::Camera::getModel() const {
	return this->m_matrices.model;
}

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
void uf::Camera::setView( const pod::Matrix4& mat ) {
	this->m_matrices.view = mat;
}
void uf::Camera::setProjection( const pod::Matrix4& mat ) {
	this->m_matrices.projection = mat;
}
void uf::Camera::setModel( const pod::Matrix4& mat ) {
	this->m_matrices.model = mat;
}

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

void uf::Camera::update(bool override) {
	if ( !override && !this->modified() ) return;
	this->updateView();
	this->updateProjection();
	this->m_modified = true;
}
void uf::Camera::updateView() {
	if ( ext::openvr::context ) {
		pod::Transform<>& transform = this->getTransform();
		pod::Vector3t<> position = transform.position;
		if ( transform.reference ) position += transform.reference->position;
		transform.orientation = uf::quaternion::identity();

		pod::Transform<> flatten = uf::transform::flatten( transform );
		pod::Matrix4t<> translation = uf::matrix::translate( uf::matrix::identity(), -position );
		pod::Matrix4t<> rotation = uf::quaternion::matrix( flatten.orientation );

		this->m_matrices.view = rotation * translation; //uf::matrix::inverse( translation * rotation );
		transform.orientation = ext::openvr::hmdQuaternion();
		::eye.left = ext::openvr::hmdViewMatrix(vr::Eye_Left, this->m_matrices.view);
		::eye.right = ext::openvr::hmdViewMatrix(vr::Eye_Right, this->m_matrices.view);

	//	::eye.left = ext::openvr::hmdEyePositionMatrix(vr::Eye_Left) * ext::openvr::hmdHeadPositionMatrix() * this->m_matrices.view;
	//	::eye.right = ext::openvr::hmdEyePositionMatrix(vr::Eye_Right) * ext::openvr::hmdHeadPositionMatrix() * this->m_matrices.view;
		// transform.orientation = ext::openvr::hmdQuaternion() * pod::Vector4f{1, 1, 1, -1};
	/*
		pod::Transform<>& transform = this->getTransform();
		transform.orientation = ext::openvr::hmdQuaternion() * pod::Vector4f{1, 1, 1, -1};
		transform = uf::transform::reorient( transform ); // set our forwards and stuff
		pod::Vector3f position = transform.position;
		pod::Quaternion<> orientation = transform.orientation;
		if ( transform.reference ) position += transform.reference->position;
		if ( transform.reference ) orientation = uf::quaternion::multiply( orientation, transform.reference->orientation * pod::Vector4f{1, 1, 1, -1} );
		{
			pod::Matrix4t<> translation = uf::matrix::translate( uf::matrix::identity(), (position + ext::openvr::hmdPosition(vr::Eye_Left)) * -1 );
			pod::Matrix4t<> rotation = uf::quaternion::matrix( orientation );
			this->setView(rotation * translation);
			::eye.left = this->m_matrices.view;
		}
		{
			pod::Matrix4t<> translation = uf::matrix::translate( uf::matrix::identity(), (position + ext::openvr::hmdPosition(vr::Eye_Left)) * -1 );
			pod::Matrix4t<> rotation = uf::quaternion::matrix( orientation );
			this->setView(rotation * translation);
			::eye.right = this->m_matrices.view;
		}
	*/
	/*
		pod::Matrix4t<> translationLeft = uf::matrix::translate( uf::matrix::identity(), (position + ext::openvr::hmdPosition(vr::Eye_Left)) * pod::Vector3f{-1, -1, -1} );
		pod::Matrix4t<> translationRight = uf::matrix::translate( uf::matrix::identity(), (position + ext::openvr::hmdPosition(vr::Eye_Right)) * pod::Vector3f{-1, -1, -1} );
		pod::Quaternion<> orientation = transform.reference ? transform.reference->orientation : transform.orientation;
		orientation.w = -orientation.w;
		pod::Matrix4t<> rotation = uf::quaternion::matrix( orientation );
		
		this->setView(rotation * translation);

		::eye.left = rotation * translationLeft;// ext::openvr::hmdEyePositionMatrix(vr::Eye_Left) * ext::openvr::hmdHeadPositionMatrix(); //ext::openvr::hmdViewMatrix(vr::Eye_Left) * this->m_matrices.view;
		::eye.right = rotation * translationRight;// ext::openvr::hmdEyePositionMatrix(vr::Eye_Right) * ext::openvr::hmdHeadPositionMatrix(); //ext::openvr::hmdViewMatrix(vr::Eye_Right) * this->m_matrices.view;
	*/
	} else {
		pod::Transform<>& transform = this->getTransform();
		pod::Vector3t<> position = transform.position;
		if ( transform.reference ) position += transform.reference->position;
		pod::Matrix4t<> translation = uf::matrix::translate( uf::matrix::identity(), position * -1 );
		pod::Transform<> flatten = uf::transform::flatten(transform, true);
		pod::Matrix4t<> rotation = uf::quaternion::matrix( flatten.orientation );

		this->setView(rotation * translation);
	/*
		pod::Transform<> transform = this->getTransform();
		if ( transform.reference ) transform.position += transform.reference->position;

		pod::Transform<> flatten = uf::transform::flatten( transform );
		pod::Matrix4t<> translation = uf::matrix::translate( uf::matrix::identity(), transform.position );
		pod::Matrix4t<> rotation = uf::quaternion::matrix( flatten.orientation * pod::Vector4f{ 1, 1, 1, -1 } );

		this->m_matrices.view = uf::matrix::inverse( translation * rotation );
	*/
	}
}
void uf::Camera::updateProjection() {
	if ( this->m_settings.mode < 0 ) {
		// Maintain aspect ratio
		if ( this->m_settings.ortho.lr.x == this->m_settings.ortho.bt.x && this->m_settings.ortho.lr.y == this->m_settings.ortho.bt.y && this->m_settings.ortho.bt.x == -this->m_settings.ortho.bt.y ) {
			pod::Math::num_t raidou = this->m_settings.perspective.size.x / this->m_settings.perspective.size.y;
			pod::Math::num_t range = this->m_settings.ortho.bt.y - this->m_settings.ortho.bt.x;
			pod::Math::num_t correction = raidou * range / 2.0;
			this->m_matrices.projection = uf::matrix::ortho(
				-correction, correction,
				this->m_settings.ortho.bt.x, this->m_settings.ortho.bt.y, 
				this->m_settings.ortho.nf.x, this->m_settings.ortho.nf.y
			);
			return;
		}
		this->m_matrices.projection = uf::matrix::ortho(
			this->m_settings.ortho.lr.x, this->m_settings.ortho.lr.y, 
			this->m_settings.ortho.bt.x, this->m_settings.ortho.bt.y, 
			this->m_settings.ortho.nf.x, this->m_settings.ortho.nf.y
		);
		return;
	}
	float fov = this->m_settings.perspective.fov * (3.14159265358f / 180.0f);
	float raidou = (float) this->m_settings.perspective.size.x / (float) this->m_settings.perspective.size.y;
	float f = 1.0f / tan( 0.5f * fov );
	this->m_matrices.projection = {
		f / raidou, 	0.0f, 	 0.0f, 	0.0f,
		0.0f, 			-f, 	 0.0f, 	0.0f,
		0.0f,       	0.0f,    0.0f, 	1.0f,
		0.0f,       	0.0f,   this->m_settings.perspective.bounds.x, 	0.0f
	};
	if ( ext::openvr::context ) {
		::projection.left = ext::openvr::hmdProjectionMatrix( vr::Eye_Left, this->m_settings.perspective.bounds.x, this->m_settings.perspective.bounds.y );
		::projection.right = ext::openvr::hmdProjectionMatrix( vr::Eye_Right, this->m_settings.perspective.bounds.x, this->m_settings.perspective.bounds.y );
	}
}