#define POD_LUA_REGISTER_USERTYPE_BEGIN(x)\
	state.new_usertype<pod::x>(#x, 
#define UF_LUA_REGISTER_USERTYPE_BEGIN(x)\
	state.new_usertype<uf::x>(#x, 
#define EXT_LUA_REGISTER_USERTYPE_BEGIN(x)\
	state.new_usertype<ext::x>(#x, 

#define POD_LUA_REGISTER_USERTYPE_MEMBER(x, y) #y, &pod::x::y
#define UF_LUA_REGISTER_USERTYPE_MEMBER(x, y) #y, &uf::x::y
#define EXT_LUA_REGISTER_USERTYPE_MEMBER(x, y) #y, &ext::x::y

#define UF_LUA_REGISTER_USERTYPE_END(x) );
#define POD_LUA_REGISTER_USERTYPE_END(x) UF_LUA_REGISTER_USERTYPE_END(x)
#define EXT_LUA_REGISTER_USERTYPE_END(x) UF_LUA_REGISTER_USERTYPE_END(x)