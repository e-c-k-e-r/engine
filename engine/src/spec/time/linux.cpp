#include <uf/spec/time/time.h>

#if 0 && UF_ENV_LINUX
namespace {
	typedef std::chrono::time_point<std::chrono::system_clock> chrono_time_t;
	chrono_time_t getTimePoint() {
		return std::chrono::system_clock::now();
	}
	inline chrono_time_t& startPoint() {
		static chrono_time_t start = getTimePoint(); // constructed on 1st use
		return start;
	}
}

spec::uni::Time::time_t spec::linux::Time::getTime() {
	std::chrono::duration<double> elapsed = getTimePoint() - startPoint();
	return elapsed.count() * 1000000;
}
spec::uni::Time& spec::linux::Time::getUniversal() {
	return (spec::uni::Time&) *this;
}

spec::linux::Time spec::time;
spec::linux::Time::exp_t spec::linux::Time::unit = -6;
#endif