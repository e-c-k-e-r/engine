#include <uf/utils/math/physics.h>

uf::Timer<> uf::physics::time::timer;
double uf::physics::time::current;
double uf::physics::time::previous;
double uf::physics::time::delta;
double uf::physics::time::clamp;

void UF_API uf::physics::tick() {
	uf::physics::time::previous = uf::physics::time::current;
	uf::physics::time::current = uf::physics::time::timer.elapsed().asDouble();
	uf::physics::time::delta = uf::physics::time::current - uf::physics::time::previous;
	if ( uf::physics::time::delta > uf::physics::time::clamp ) uf::physics::time::delta = uf::physics::time::clamp;
}