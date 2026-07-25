#include <uf/spec/time/time.h>
#if UF_ENV_DREAMCAST
#include <sys/time.h>
#include <stddef.h>

namespace {
	inline spec::uni::time::time_t get_time_us() {
		struct timeval tv;
		gettimeofday(&tv, NULL);
		return static_cast<spec::uni::time::time_t>(tv.tv_sec) * 1000000ULL + tv.tv_usec;
	}

	inline spec::uni::time::time_t& startPoint() {
		static spec::uni::time::time_t start = get_time_us();
		return start;
	}
}

spec::uni::time::time_t spec::uni::time::unixTime() {
	return get_time_us();
}

spec::uni::time::time_t spec::uni::time::getTime() {
	return get_time_us() - startPoint();
}
#else
#include <chrono>

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

spec::uni::time::time_t spec::uni::time::unixTime() {
	return std::chrono::duration_cast<std::chrono::microseconds>(getTimePoint().time_since_epoch()).count();
}
spec::uni::time::time_t spec::uni::time::getTime() {
	std::chrono::duration<double> elapsed = getTimePoint() - startPoint();
	return elapsed.count() * 1000000;
}
#endif