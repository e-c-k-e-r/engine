#include <uf/utils/math/physics.h>

uf::Timer<> uf::physics::time::timer;
uf::physics::num_t uf::physics::time::current;
uf::physics::num_t uf::physics::time::previous;
uf::physics::num_t uf::physics::time::delta;
uf::physics::num_t uf::physics::time::clamp;

#include <iostream>

void UF_API uf::physics::tick() {
	uf::physics::time::previous = uf::physics::time::current;
	uf::physics::time::current = uf::physics::time::timer.elapsed();
	uf::physics::time::delta = uf::physics::time::current - uf::physics::time::previous;
	if ( uf::physics::time::delta > uf::physics::time::clamp ) {
		uf::physics::time::delta = uf::physics::time::clamp;
	}
}