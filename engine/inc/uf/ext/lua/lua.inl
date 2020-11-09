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


/*
#define UF_NS_LUA_REGISTER_USERTYPE_BEGIN(k, v) {\
	std::string name = #v;\
	sol::usertype<k::v> usertype = state.new_usertype<k::v>(name);

#define POD_LUA_REGISTER_USERTYPE_BEGIN(x) UF_NS_LUA_REGISTER_USERTYPE_BEGIN(pod, x)
#define UF_LUA_REGISTER_USERTYPE_BEGIN(x) UF_NS_LUA_REGISTER_USERTYPE_BEGIN(uf, x)
#define EXT_LUA_REGISTER_USERTYPE_BEGIN(x) UF_NS_LUA_REGISTER_USERTYPE_BEGIN(ext, x)

#define UF_LUA_REGISTER_USERTYPE_DEFINE(k, v) usertype[#k] = v;
#define POD_LUA_REGISTER_USERTYPE_DEFINE(k, v) UF_LUA_REGISTER_USERTYPE_DEFINE(k, v)
#define EXT_LUA_REGISTER_USERTYPE_DEFINE(k, v) UF_LUA_REGISTER_USERTYPE_DEFINE(k, v)

#define UF_NS_LUA_REGISTER_USERTYPE_MEMBER(ns, x, y) UF_LUA_REGISTER_USERTYPE_DEFINE(y, &ns::x::y)
#define POD_LUA_REGISTER_USERTYPE_MEMBER(x, y) UF_NS_LUA_REGISTER_USERTYPE_MEMBER(pod, x, y)
#define UF_LUA_REGISTER_USERTYPE_MEMBER(x, y) UF_NS_LUA_REGISTER_USERTYPE_MEMBER(uf, x, y)
#define EXT_LUA_REGISTER_USERTYPE_MEMBER(x, y) UF_NS_LUA_REGISTER_USERTYPE_MEMBER(ext, x, y)

#define UF_LUA_REGISTER_USERTYPE_END() }
#define POD_LUA_REGISTER_USERTYPE_END() UF_LUA_REGISTER_USERTYPE_END()
#define EXT_LUA_REGISTER_USERTYPE_END() UF_LUA_REGISTER_USERTYPE_END()
*/