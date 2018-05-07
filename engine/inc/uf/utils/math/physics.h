#pragma once

#include <uf/config.h>
#include <uf/utils/math/transform.h>
#include <uf/utils/time/time.h>

namespace pod {
	struct UF_API Physics {
		struct {
			pod::Vector3 velocity;
			pod::Vector3 acceleration;
		} linear;
		struct {
			pod::Quaternion<> velocity;
			pod::Quaternion<> acceleration;
		} rotational;
		pod::Transform<> previous;
	};
}

namespace uf {
	namespace physics {
		namespace time {
			UF_API uf::Timer<> timer;
			UF_API double current, previous, delta;
		}
		void UF_API tick();
		template<typename T> pod::Transform<T>& update( pod::Transform<T>& transform, pod::Physics& physics );
		template<typename T> pod::Transform<T>& update( pod::Physics& physics, pod::Transform<T>& transform );
	}
}

#include "physics/pod.inl"