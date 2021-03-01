#pragma once

#include <uf/config.h>
#include "universal.h"

#if UF_ENV_DREAMCAST

namespace spec {
	namespace dreamcast {
		class UF_API Time : public spec::uni::Time {
		public:
			static Time::exp_t unit;
			spec::uni::Time& UF_API_CALL getUniversal();

			spec::uni::Time::time_t UF_API_CALL getTime();
		};
	}
	typedef dreamcast::Time Time;
	extern UF_API Time time;
}

#endif