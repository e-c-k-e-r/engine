#define UF_NS_GET_LAST(name) uf::string::namespaceGetLast(#name)
#define UF_NS_REMOVE_FIRST(name) uf::string::namespaceRemoveFirst(#name)

#define TOKEN__PASTE(x, y) x ## y
#define TOKEN_PASTE(x, y) TOKEN__PASTE(x, y)

#define UF_LUA_REGISTER_USERTYPE_BEGIN(type) \
namespace {\
	static uf::StaticInitialization TOKEN_PASTE(STATIC_INITIALIZATION_, __LINE__)( []{\
		ext::lua::onInitialization( []{\
			uf::stl::string name = UF_NS_GET_LAST(type);\
			auto usertype = ext::lua::state.new_usertype<type>(name);

#define UF_LUA_REGISTER_USERTYPE_DEFINE_RT(k, v) usertype[#k] = v;
#define UF_LUA_REGISTER_USERTYPE_MEMBER_RT(member) usertype[UF_NS_GET_LAST(member)] = &member;
#define UF_LUA_REGISTER_USERTYPE_MEMBER_FUN_RT(member) usertype[UF_NS_GET_LAST(member)] = UF_LUA_C_FUN(member);

#define UF_LUA_REGISTER_USERTYPE_END() \
		});\
	});\
}
/*

namespace sol {\
	template <>\
	struct is_automagical<type> : std::false_type {};\
}\

*/

#define UF_LUA_REGISTER_USERTYPE(type, ...) \
namespace {\
	static uf::StaticInitialization TOKEN_PASTE(STATIC_INITIALIZATION_, __LINE__)( []{\
		ext::lua::onInitialization( []{\
			ext::lua::state.new_usertype<type>(UF_NS_GET_LAST(type), __VA_ARGS__);\
			ext::lua::componentGetters[UF_NS_REMOVE_FIRST(type)] = &ext::lua::getComponent<type>;\
		});\
	});\
}
#define UF_LUA_REGISTER_COMPONENT(type, ...) \
namespace {\
	static uf::StaticInitialization TOKEN_PASTE(STATIC_INITIALIZATION_, __LINE__)( []{\
		ext::lua::onInitialization( []{\
			ext::lua::componentGetters[UF_NS_REMOVE_FIRST(type)] = &ext::lua::getComponent<type>;\
		});\
	});\
}
#define UF_LUA_REGISTER_USERTYPE_AND_COMPONENT(type, ...) \
namespace {\
	static uf::StaticInitialization TOKEN_PASTE(STATIC_INITIALIZATION_, __LINE__)( []{\
		ext::lua::onInitialization( []{\
			ext::lua::state.new_usertype<type>(UF_NS_GET_LAST(type), __VA_ARGS__);\
			ext::lua::componentGetters[UF_NS_REMOVE_FIRST(type)] = &ext::lua::getComponent<type>;\
		});\
	});\
}


#define UF_LUA_REGISTER_USERTYPE_DEFINE(k, v) #k, v
#define UF_LUA_REGISTER_USERTYPE_MEMBER(member) UF_NS_GET_LAST(member), &member
#define UF_LUA_REGISTER_USERTYPE_MEMBER_FUN(member) UF_NS_GET_LAST(member), UF_LUA_C_FUN(member)

#define UF_LUA_REGISTER_ENUM(type, ...) \
namespace {\
	static uf::StaticInitialization TOKEN_PASTE(STATIC_INITIALIZATION_, __LINE__)( []{\
		ext::lua::onInitialization( []{\
			ext::lua::state.new_enum(UF_NS_GET_LAST(type), __VA_ARGS__);\
		});\
	});\
}

#define UF_LUA_C_FUN(x) &x
#define UF_LUA_WRAP_FUN(x) UF_LUA_C_FUN(x)