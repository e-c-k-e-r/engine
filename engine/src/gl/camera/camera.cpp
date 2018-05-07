#include <uf/gl/camera/camera.h>

uf::Camera::Camera() : 
	m_modified(false)
{
	this->m_settings.perspective.fov = 100;
	this->m_settings.perspective.size = {1280, 720};
	this->m_settings.perspective.bounds = {0.01, 32};
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

pod::Matrix4& uf::Camera::getView() {
	return this->m_matrices.view;
}
pod::Matrix4& uf::Camera::getProjection() {
	return this->m_matrices.projection;
}
pod::Matrix4& uf::Camera::getModel() {
	return this->m_matrices.model;
}

const pod::Matrix4& uf::Camera::getView() const {
	return this->m_matrices.view;
}
const pod::Matrix4& uf::Camera::getProjection() const {
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
//	this->m_matrices.view = uf::transform::view(this->m_transform, this->m_settings.offset);
	pod::Transform<>& t = this->getTransform();
	uf::Matrix4t<> translation, rotation;
	pod::Transform<> flatten = uf::transform::flatten(t, true);
	rotation = uf::quaternion::matrix( flatten.orientation );
	flatten.position += uf::quaternion::rotate( flatten.orientation, this->getOffset() );
	translation = uf::matrix::translate( uf::matrix::identity(), -flatten.position );

	pod::Matrix4 m = rotation * translation;
	this->setView(m);
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

	pod::Vector2& size = this->m_settings.perspective.size;
	pod::Math::num_t lower = this->m_settings.perspective.bounds.x;
	pod::Math::num_t upper = this->m_settings.perspective.bounds.y;
	pod::Math::num_t raidou = (pod::Math::num_t) size.x / (pod::Math::num_t) size.y;
	pod::Math::num_t fov = this->m_settings.perspective.fov * (3.14159265358 / 180.0);
	pod::Math::num_t range = lower - upper;
	pod::Math::num_t speshul = tanf( fov / 2.0 );

	pod::Math::num_t Sx = 1.0 / (speshul * raidou);
	pod::Math::num_t Sy = 1.0 / speshul;
	pod::Math::num_t Sz = (-lower - upper) / range;
	pod::Math::num_t Pz = 2.0 * upper * lower / range;

	pod::Math::num_t mat[] = {
		Sx, 	 0, 	 0, 	  0,
		 0, 	Sy, 	 0, 	  0,
		 0, 	 0, 	Sz, 	  1,
		 0, 	 0, 	Pz, 	  0
	};
	uf::matrix::copy(this->m_matrices.projection, mat);
}