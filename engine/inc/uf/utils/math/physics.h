#pragma once

#include <uf/config.h>
#include <uf/utils/math/transform.h>
#include <uf/utils/time/time.h>
#include <uf/engine/object/object.h>

#if UF_USE_REACTPHYSICS
	#include <uf/ext/reactphysics/reactphysics.h>
#else
	#include "physics/impl.h"
#endif

namespace uf {
	namespace physics {
		typedef pod::Math::num_t num_t;
		namespace time = uf::time; // to-do: have separate values from the physics system
		
		void UF_API initialize();
		void UF_API tick();
		void UF_API terminate();

		void UF_API initialize( uf::Object& );
		void UF_API tick( uf::Object& );
		void UF_API terminate( uf::Object& );
	#if 0
		template<typename T> pod::Transform<T>& update( pod::Transform<T>& transform, pod::Physics& physics );
		template<typename T> pod::Transform<T>& update( pod::Physics& physics, pod::Transform<T>& transform );
	#endif
	}
}

#if 0
#include "physics/pod.inl"
#endif