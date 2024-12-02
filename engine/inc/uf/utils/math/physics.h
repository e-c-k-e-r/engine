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
		typedef pod::Math::num_t num_t;
		namespace time = uf::time;
		
		void UF_API initialize();
		void UF_API tick();
		void UF_API terminate();

		void UF_API initialize( uf::Object& );
		void UF_API tick( uf::Object& );
		void UF_API terminate( uf::Object& );
		template<typename T> pod::Transform<T>& update( pod::Transform<T>& transform, pod::Physics& physics );
		template<typename T> pod::Transform<T>& update( pod::Physics& physics, pod::Transform<T>& transform );
	}
}

#include "physics/pod.inl"