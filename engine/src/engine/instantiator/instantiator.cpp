#include <uf/engine/instantiator/instantiator.h>
#include <assert.h>

std::unordered_map<std::type_index, std::string>* uf::instantiator::names = NULL;
std::unordered_map<std::string, uf::instantiator::function_t>* uf::instantiator::map = NULL;

uf::Entity* uf::instantiator::instantiate( const std::string& name ) {
	auto& map = *uf::instantiator::map;
	// assert it's already in map
	assert( map.count(name) > 0 );
	return map[name]();
}