#include <uf/spec/time/time.h>

#ifdef UF_ENV_WINDOWS

namespace {
	LARGE_INTEGER getFrequency() {
		LARGE_INTEGER freq;
		QueryPerformanceFrequency(&freq);
		return freq;
	}
}


spec::uni::Time::time_t spec::win32::Time::getTime() {
	HANDLE curThread = GetCurrentThread();
	DWORD_PTR prevMask = SetThreadAffinityMask(curThread, 1);
	static LARGE_INTEGER freq = getFrequency();
	LARGE_INTEGER time;
	QueryPerformanceCounter(&time);
	SetThreadAffinityMask(curThread, prevMask);

	return 1000000 * time.QuadPart / freq.QuadPart;
}
spec::uni::Time& spec::win32::Time::getUniversal() {
	return (spec::uni::Time&) *this;
}

spec::win32::Time spec::time;
spec::win32::Time::exp_t spec::win32::Time::unit = -6;
#endif