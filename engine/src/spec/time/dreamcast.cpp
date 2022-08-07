#include <uf/spec/time/time.h>
#ifdef UF_ENV_DREAMCAST
#include <arch/timer.h>

spec::uni::Time::time_t spec::dreamcast::Time::getTime() {
	return timer_us_gettime64();
}
spec::uni::Time& spec::dreamcast::Time::getUniversal() {
	return (spec::uni::Time&) *this;
}

spec::dreamcast::Time spec::time;
spec::dreamcast::Time::exp_t spec::dreamcast::Time::unit = -6;
#endif