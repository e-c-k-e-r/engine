#pragma once

#include <uf/config.h>
#include "universal.h"

#if defined(UF_ENV_WINDOWS)

namespace spec {
	namespace win32 {
		class UF_API Time : public spec::uni::Time {
		public:
			static Time::exp_t unit;
			spec::uni::Time& UF_API_CALL getUniversal();

			spec::uni::Time::time_t UF_API_CALL getTime();
		};
	}
	typedef win32::Time Time;
	extern UF_API Time time;
}

#endif