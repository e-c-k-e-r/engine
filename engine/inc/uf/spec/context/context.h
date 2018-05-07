#pragma once

#include <uf/config.h>
#include <uf/spec/universal.h>

// include universal
#include "universal.h"
// defines which implementation to use
#include UF_ENV_HEADER

#if defined(UF_USE_SFML) && UF_USE_SFML == 1
	#include "unknown.h"
#endif