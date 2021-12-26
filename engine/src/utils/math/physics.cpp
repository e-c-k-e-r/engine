#include <uf/utils/math/physics.h>
#include <iostream>

uf::Timer<> uf::physics::time::timer;
double uf::physics::time::current = 0;
double uf::physics::time::previous = 0;
float uf::physics::time::delta = 0;
float uf::physics::time::clamp = 0;


void UF_API uf::physics::initialize() {
	uf::physics::impl::initialize();
}
void UF_API uf::physics::tick() {
	uf::physics::time::previous = uf::physics::time::current;
	uf::physics::time::current = uf::physics::time::timer.elapsed();
	
	uf::physics::time::delta = uf::physics::time::current - uf::physics::time::previous;
	if ( uf::physics::time::delta > uf::physics::time::clamp ) {
		uf::physics::time::delta = uf::physics::time::clamp;
	}
	uf::physics::impl::tick();
}
void UF_API uf::physics::terminate() {
	uf::physics::impl::terminate();
}