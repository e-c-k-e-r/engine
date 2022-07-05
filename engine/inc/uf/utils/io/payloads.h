#pragma once

#include <uf/utils/hook/payloads.h>
namespace pod {
	namespace payloads {
		struct Log {
			uf::stl::string message;
			uf::stl::string category;
		};
	}
}