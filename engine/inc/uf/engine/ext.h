#include <uf/config.h>
#include <uf/utils/memory/vector.h>
#include <uf/utils/memory/string.h>
#include <uf/utils/serialize/serializer.h>

namespace uf {
	extern bool UF_API ready;
	extern uf::stl::vector<uf::stl::string> UF_API arguments;
	extern uf::Serializer UF_API config;
	
	extern void UF_API load();
	extern void UF_API load( ext::json::Value& );
	extern void UF_API initialize();
	extern bool UF_API running();
	extern void UF_API tick();
	extern void UF_API render();
	extern void UF_API terminate();
}