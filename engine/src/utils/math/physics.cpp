#include <uf/utils/math/physics.h>

void UF_API uf::physics::tick() {
	static float clamp = 1 / 15.0; if ( uf::physics::time::clamp != clamp ) uf::physics::time::clamp = clamp;
	uf::physics::time::previous = uf::physics::time::current;
	uf::physics::time::current = uf::physics::time::timer.elapsed().asDouble();
	uf::physics::time::delta = uf::physics::time::current - uf::physics::time::previous;
	if ( uf::physics::time::delta > uf::physics::time::clamp ) uf::physics::time::delta = uf::physics::time::clamp;
}