#include <uf/spec/controller/controller.h>

void spec::uni::controller::initialize() {}
void spec::uni::controller::tick() {}
void spec::uni::controller::terminate() {}
bool spec::uni::controller::connected( size_t ) { return false; }
bool spec::uni::controller::pressed( const uf::stl::string&, size_t ) { return false; }
float spec::uni::controller::analog( const uf::stl::string&, size_t ) { return 0; }