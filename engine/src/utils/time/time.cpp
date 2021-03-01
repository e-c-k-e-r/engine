#include <uf/utils/time/time.h> // time(r)

namespace {
	uf::Time<> zero = spec::time.getTime(); 	// Program epoch, used for getting time deltas (without uf::Timer)
}
extern UF_API uf::Timer<> timer; 				// System timer, used for getting time deltas