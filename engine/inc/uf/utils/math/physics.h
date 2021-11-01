#pragma once

#include <uf/config.h>
#include <uf/utils/math/transform.h>
#include <uf/utils/time/time.h>

#if UF_USE_BULLET
#include <uf/ext/bullet/bullet.h>
#elif UF_USE_REACTPHYSICS
#include <uf/ext/reactphysics/reactphysics.h>
#endif

namespace uf {
	namespace physics {
	#if UF_USE_BULLET
		namespace impl = ext::bullet;
	#elif UF_USE_REACTPHYSICS
		namespace impl = ext::reactphysics;
	#endif

		typedef pod::Math::num_t num_t;
		namespace time {

			extern UF_API uf::Timer<> timer;
			extern UF_API uf::physics::num_t current;
			extern UF_API uf::physics::num_t previous;
			extern UF_API uf::physics::num_t delta;
			extern UF_API uf::physics::num_t clamp;
		}
		void UF_API initialize();
		void UF_API tick();
		void UF_API terminate();
		template<typename T> pod::Transform<T>& update( pod::Transform<T>& transform, pod::Physics& physics );
		template<typename T> pod::Transform<T>& update( pod::Physics& physics, pod::Transform<T>& transform );
	}
}

#include "physics/pod.inl"