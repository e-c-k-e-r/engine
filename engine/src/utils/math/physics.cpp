#include <uf/utils/math/physics.h>

void UF_API uf::physics::tick() {
	uf::physics::time::previous = uf::physics::time::current;
	uf::physics::time::current = uf::physics::time::timer.elapsed().asDouble();
	uf::physics::time::delta = uf::physics::time::current - uf::physics::time::previous;
}