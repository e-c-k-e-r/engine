#include <uf/ext/lua/lua.h>
#if UF_USE_LUA
#include <uf/utils/math/physics.h>

namespace binds {
	double elapsed( uf::Timer<>& self ) { return self.elapsed().asDouble(); }
	void start( uf::Timer<>& self ) { return self.start(); }
	void stop( uf::Timer<>& self ) { return self.stop(); }
	void reset( uf::Timer<>& self ) { return self.reset(); }
	void update( uf::Timer<>& self ) { return self.update(); }
	bool running( uf::Timer<>& self ) { return self.running(); }
}

UF_LUA_REGISTER_USERTYPE(uf::Timer<>,
	UF_LUA_REGISTER_USERTYPE_DEFINE( elapsed, UF_LUA_C_FUN(::binds::elapsed) ),
	UF_LUA_REGISTER_USERTYPE_MEMBER_FUN( uf::Timer<>::start ),
	UF_LUA_REGISTER_USERTYPE_MEMBER_FUN( uf::Timer<>::stop ),
	UF_LUA_REGISTER_USERTYPE_MEMBER_FUN( uf::Timer<>::reset ),
	UF_LUA_REGISTER_USERTYPE_MEMBER_FUN( uf::Timer<>::update ),
	UF_LUA_REGISTER_USERTYPE_MEMBER_FUN( uf::Timer<>::running )
)
#endif