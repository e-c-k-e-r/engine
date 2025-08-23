#include <uf/spec/time/time.h>

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

/*
spec::uni::Time::time_t spec::uni::Time::unixTime() {
	return std::chrono::duration_cast<std::chrono::microseconds>(getTimePoint().time_since_epoch()).count();
}
spec::uni::Time::time_t spec::uni::Time::getTime() {
	std::chrono::duration<double> elapsed = getTimePoint() - startPoint();
	return elapsed.count() * 1000000;
}

spec::uni::Time::exp_t spec::uni::Time::unit = -6;
*/