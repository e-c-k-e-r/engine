#include <uf/utils/singletons/pre_main.h>

#include <uf/utils/memory/vector.h>

namespace {
	// queue of deferred initializers; the pointer is constant-initialized to NULL, so it is safe to touch during static initialization
	uf::stl::vector<std::function<void()>>* queuedInitializations = NULL;
}

uf::StaticInitialization::StaticInitialization( std::function<void()> fun ) {
	if ( queuedInitializations == NULL ) queuedInitializations = new uf::stl::vector<std::function<void()>>;
	queuedInitializations->emplace_back( std::move(fun) );
}

void uf::StaticInitialization::runAll() {
	if ( queuedInitializations == NULL ) return;
	auto& queue = *queuedInitializations;
	for ( auto& initializer : queue ) if ( initializer ) initializer();
	queue.clear();
}