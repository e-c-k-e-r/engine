#include <uf/spec/controller/controller.h>

void spec::uni::controller::initialize() {}
void spec::uni::controller::tick() {}
void spec::uni::controller::terminate() {}
bool spec::uni::controller::connected( size_t ) { return false; }
bool spec::uni::controller::pressed( const std::string&, size_t ) { return false; }
float spec::uni::controller::analog( const std::string&, size_t ) { return 0; }