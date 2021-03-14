#define UF_NS_GET_LAST(name) uf::string::replace( uf::string::split( #name, "::" ).back(), "<>", "" )

#define TOKEN__PASTE(x, y) x ## y
#define TOKEN_PASTE(x, y) TOKEN__PASTE(x, y)

#define UF_LUA_REGISTER_USERTYPE_BEGIN(type) \
namespace {\
	static uf::StaticInitialization TOKEN_PASTE(STATIC_INITIALIZATION_, __LINE__)( []{\
		ext::lua::onInitialization( []{\
			std::string name = UF_NS_GET_LAST(type);\
			auto usertype = ext::lua::state.new_usertype<type>(name);

#define UF_LUA_REGISTER_USERTYPE_DEFINE_RT(k, v) usertype[#k] = v;
#define UF_LUA_REGISTER_USERTYPE_MEMBER_RT(member) usertype[UF_NS_GET_LAST(member)] = &member;
#define UF_LUA_REGISTER_USERTYPE_MEMBER_FUN_RT(member) usertype[UF_NS_GET_LAST(member)] = UF_LUA_C_FUN(member);

#define UF_LUA_REGISTER_USERTYPE_END() \
		});\
	});\
}


#define UF_LUA_REGISTER_USERTYPE(type, ...) \
namespace {\
	static uf::StaticInitialization TOKEN_PASTE(STATIC_INITIALIZATION_, __LINE__)( []{\
		ext::lua::onInitialization( []{\
			ext::lua::state.new_usertype<type>(UF_NS_GET_LAST(type), __VA_ARGS__);\
		});\
	});\
}

#define UF_LUA_REGISTER_USERTYPE_DEFINE(k, v) #k, v
#define UF_LUA_REGISTER_USERTYPE_MEMBER(member) UF_NS_GET_LAST(member), &member
#define UF_LUA_REGISTER_USERTYPE_MEMBER_FUN(member) UF_NS_GET_LAST(member), UF_LUA_C_FUN(member)

#define UF_LUA_C_FUN(x) sol::c_call<decltype(&x), &x>
#define UF_LUA_WRAP_FUN(x) sol::c_call<sol::wrap<decltype(&x), &x>>