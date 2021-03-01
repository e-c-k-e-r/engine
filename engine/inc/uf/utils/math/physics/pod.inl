#include <iostream>
template<typename T> pod::Transform<T>& uf::physics::update( pod::Transform<T>& transform, pod::Physics& physics ) {
	physics.previous = transform;

	if ( physics.linear.acceleration != pod::Vector3t<T>{0,0,0} ) 
		physics.linear.velocity += (physics.linear.acceleration*uf::physics::time::delta);
	if ( physics.rotational.acceleration != pod::Quaternion<T>{0,0,0,0} ) {
		physics.rotational.velocity = uf::quaternion::multiply(physics.rotational.velocity, physics.rotational.acceleration*uf::physics::time::delta);
	}

	transform = uf::transform::move( transform, physics.linear.velocity*uf::physics::time::delta );
	transform = uf::transform::rotate( transform, uf::vector::normalize(physics.rotational.velocity*uf::physics::time::delta));

	return transform;
}

template<typename T> pod::Transform<T>& uf::physics::update( pod::Physics& physics, pod::Transform<T>& transform ) {
	return uf::physics::update(transform, physics);
}