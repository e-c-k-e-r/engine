#include <uf/spec/time/time.h>

#include <chrono>

spec::uni::Time::exp_t spec::uni::Time::unit = -6;

namespace {
	typedef std::chrono::time_point<std::chrono::system_clock> chrono_time_t;
	chrono_time_t getTimePoint() {
		return std::chrono::system_clock::now();
	}
	chrono_time_t start = getTimePoint();
}

spec::uni::Time::time_t spec::uni::Time::getTime() {
	std::chrono::duration<double> elapsed = getTimePoint() - start;
	return elapsed.count() * 1000000;
}