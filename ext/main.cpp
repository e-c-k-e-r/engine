#include <uf/config.h>
#include <uf/ext/ext.h>

// perform unit tests
#include <uf/utils/tests/tests.h>

void EXT_API ext::initialize() {
	uf::unitTests::execute();
}
void EXT_API ext::tick() {
	
}
void EXT_API ext::render() {
	
}
void EXT_API ext::terminate() {
	
}