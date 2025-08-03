#include <uf/utils/time/time.h> // time(r)

namespace {
	uf::Time<> zero = spec::time.getTime(); 	// Program epoch, used for getting time deltas (without uf::Timer)
}
// extern UF_API uf::Timer<> timer; 				// System timer, used for getting time deltas

uf::Timer<> uf::time::timer;
size_t uf::time::frame = 0;
double uf::time::current = 0;
double uf::time::previous = 0;
float uf::time::delta = 0;
float uf::time::clamp = 0;

size_t uf::time::time() {
	return spec::time.unixTime();
}