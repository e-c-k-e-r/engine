#pragma once

#include <uf/config.h>

namespace spec {
	namespace uni {
		class UF_API Time {
		public:
			typedef long long time_t;
			typedef int exp_t;
			static Time::exp_t unit;
		protected:
			
		public:
			spec::uni::Time::time_t UF_API_CALL unixTime();
			spec::uni::Time::time_t UF_API_CALL getTime();
		};
	};
}