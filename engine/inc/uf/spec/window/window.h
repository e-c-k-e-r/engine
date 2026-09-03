#pragma once

#include <uf/config.h>
#include <uf/spec/universal.h>

// include universal
#include "universal.h"
// the null backend is always available; headless mode is a runtime choice now
#include "null.h"
// defines which implementation to use
#include UF_ENV_HEADER
//

namespace uf {
	// backend is picked at runtime by uni::Window::create_instance (see uf::headless)
	using Window = spec::uni::Window;
}
