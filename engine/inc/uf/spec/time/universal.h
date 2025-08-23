#pragma once

#include <uf/config.h>

namespace spec {
	namespace uni {
		namespace time {
			typedef long long time_t;
			typedef int exp_t;
			static constexpr exp_t unit = -6;

			time_t UF_API unixTime();
			time_t UF_API getTime();
		}
	}

	namespace time = uni::time;
}