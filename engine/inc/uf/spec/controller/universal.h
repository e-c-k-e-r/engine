#pragma once

#include <uf/config.h>
#include <uf/utils/memory/string.h>
namespace spec {
	namespace uni {
		namespace controller {
			void UF_API initialize();
			void UF_API tick();
			void UF_API terminate();

			bool UF_API connected( size_t = 0 );
		//	bool UF_API pressed( const uf::stl::string&, size_t = 0 );
		//	float UF_API analog( const uf::stl::string&, size_t = 0 );
		};
	};
}