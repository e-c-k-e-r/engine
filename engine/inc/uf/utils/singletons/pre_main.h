#pragma once

#include <uf/config.h>
#include <functional>

namespace uf {
	struct UF_API StaticInitialization {
	public:
		StaticInitialization(std::function<void()>);
		static void UF_API runAll();
	};
}