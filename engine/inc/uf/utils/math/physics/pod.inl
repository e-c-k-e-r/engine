#include <iostream>
template<typename T> pod::Transform<T>& uf::physics::update( pod::Transform<T>& transform, pod::Physics& physics ) {
//	physics.internal.previous = transform;

	if ( physics.acceleration != pod::Vector3t<T>{0,0,0} ) 
		physics.velocity += (physics.acceleration * uf::physics::time::delta);
	if ( physics.angularAcceleration != pod::Quaternion<T>{0,0,0,0} ) {
		physics.angularVelocity = uf::quaternion::multiply(physics.angularVelocity, physics.angularAcceleration*uf::physics::time::delta);
	}

	transform = uf::transform::move( transform, physics.velocity*uf::physics::time::delta );
	transform = uf::transform::rotate( transform, uf::vector::normalize(physics.angularVelocity*uf::physics::time::delta));

	return transform;
}

template<typename T> pod::Transform<T>& uf::physics::update( pod::Physics& physics, pod::Transform<T>& transform ) {
	return uf::physics::update(transform, physics);
}