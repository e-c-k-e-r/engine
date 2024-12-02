#include <uf/utils/math/physics.h>
#include <uf/engine/scene/scene.h>
#include <iostream>

/*
uf::Timer<> uf::physics::time::timer;
double uf::physics::time::current = 0;
double uf::physics::time::previous = 0;
float uf::physics::time::delta = 0;
float uf::physics::time::clamp = 0;
*/


void uf::physics::initialize( ) {
	return uf::physics::initialize( uf::scene::getCurrentScene() );
}
void uf::physics::initialize( uf::Object& scene ) {
	uf::physics::impl::initialize( scene );
}
void uf::physics::tick( ) {
	return uf::physics::tick( uf::scene::getCurrentScene() );
}
void uf::physics::tick( uf::Object& scene ) {
	++uf::physics::time::frame;
	
	uf::physics::time::previous = uf::physics::time::current;
	uf::physics::time::current = uf::physics::time::timer.elapsed();
	
	uf::physics::time::delta = uf::physics::time::current - uf::physics::time::previous;
	if ( uf::physics::time::delta > uf::physics::time::clamp ) {
		uf::physics::time::delta = uf::physics::time::clamp;
	}
	uf::physics::impl::tick( scene );
}
void uf::physics::terminate( ) {
	return uf::physics::terminate( uf::scene::getCurrentScene() );
}
void uf::physics::terminate( uf::Object& scene ) {
	uf::physics::impl::terminate( scene );
}