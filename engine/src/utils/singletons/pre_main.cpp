#include <uf/utils/singletons/pre_main.h>

uf::StaticInitialization::StaticInitialization( std::function<void()> fun ) {
#if UF_ENV_DREAMCAST
#else
	fun();
#endif
}