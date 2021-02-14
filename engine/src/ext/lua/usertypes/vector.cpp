#include <uf/ext/lua/lua.h>
#if UF_USE_LUA
#include <uf/utils/math/vector.h>

UF_LUA_REGISTER_USERTYPE(pod::Vector3f,
	sol::call_constructor, sol::initializers( 
		[]( pod::Vector3f& self ) {
			return self = pod::Vector3f{};
		},
		[]( pod::Vector3f& self, const pod::Vector3f& copy ) {
			return self = copy;
		},
		[]( pod::Vector3f& self, float x, float y, float z ) {
			return self = uf::vector::create(x, y, z);
		}
	),
	UF_LUA_REGISTER_USERTYPE_MEMBER(pod::Vector3f::x),
	UF_LUA_REGISTER_USERTYPE_MEMBER(pod::Vector3f::y),
	UF_LUA_REGISTER_USERTYPE_MEMBER(pod::Vector3f::z),
	UF_LUA_REGISTER_USERTYPE_DEFINE( v, []( pod::Vector3f& self, const size_t& index ) {
		return self[index];
	}),
	UF_LUA_REGISTER_USERTYPE_DEFINE( distance, []( pod::Vector3f& self, const pod::Vector3f& other ) {
		return uf::vector::distance( self, other );
	}),
	sol::meta_function::addition, []( const pod::Vector3f& left, const pod::Vector3f& right ) {
		return uf::vector::add( left, right );
	},
	sol::meta_function::subtraction, []( const pod::Vector3f& left, const pod::Vector3f& right ) {
		return uf::vector::subtract( left, right );
	},
	sol::meta_function::multiplication, []( const pod::Vector3f& left, const pod::Vector3f& right ) {
		return uf::vector::multiply( left, right );
	},
	sol::meta_function::multiplication, []( const pod::Vector3f& left, double right ) {
		return uf::vector::multiply( left, right );
	},
	sol::meta_function::division, []( const pod::Vector3f& left, const pod::Vector3f& right ) {
		return uf::vector::divide( left, right );
	},
	sol::meta_function::division, []( const pod::Vector3f& left, double right ) {
		return uf::vector::divide( left, right );
	},
	UF_LUA_REGISTER_USERTYPE_DEFINE( lerp, []( const pod::Vector3f& start, const pod::Vector3f& end, double delta ) {
		return uf::vector::lerp( start, end, delta );
	}),
	UF_LUA_REGISTER_USERTYPE_DEFINE( normalize, []( const pod::Vector3f& self ) {
		return uf::vector::normalize( self );
	}),
	UF_LUA_REGISTER_USERTYPE_DEFINE( magnitude, []( const pod::Vector3f& self ) {
		return uf::vector::magnitude( self );
	}),
	UF_LUA_REGISTER_USERTYPE_DEFINE( dot, []( const pod::Vector3f& left, const pod::Vector3f& right ) {
		return uf::vector::dot( left, right );
	}),
	UF_LUA_REGISTER_USERTYPE_DEFINE( __tostring, []( pod::Vector3f& self ) {
		return uf::string::toString( self );
	})
)
UF_LUA_REGISTER_USERTYPE(pod::Vector4f,
	sol::call_constructor, sol::initializers( 
		[]( pod::Vector4f& self ) {
			return self = pod::Vector4f{};
		},
		[]( pod::Vector4f& self, const pod::Vector4f& copy ) {
			return self = copy;
		},
		[]( pod::Vector4f& self, float x, float y, float z, float w ) {
			return self = uf::vector::create(x, y, z, w);
		}
	),
	UF_LUA_REGISTER_USERTYPE_MEMBER(pod::Vector4f::x),
	UF_LUA_REGISTER_USERTYPE_MEMBER(pod::Vector4f::y),
	UF_LUA_REGISTER_USERTYPE_MEMBER(pod::Vector4f::z),
	UF_LUA_REGISTER_USERTYPE_MEMBER(pod::Vector4f::w),
	UF_LUA_REGISTER_USERTYPE_DEFINE( v, []( pod::Vector4f& self, const size_t& index ) {
		return self[index];
	}),
	UF_LUA_REGISTER_USERTYPE_DEFINE( distance, []( pod::Vector4f& self, const pod::Vector4f& other ) {
		return uf::vector::distance( self, other );
	}),
	sol::meta_function::addition, []( const pod::Vector4f& left, const pod::Vector4f& right ) {
		return uf::vector::add( left, right );
	},
	sol::meta_function::subtraction, []( const pod::Vector4f& left, const pod::Vector4f& right ) {
		return uf::vector::subtract( left, right );
	},
	sol::meta_function::multiplication, []( const pod::Vector4f& left, const pod::Vector4f& right ) {
		return uf::vector::multiply( left, right );
	},
	sol::meta_function::multiplication, []( const pod::Vector4f& left, double right ) {
		return uf::vector::multiply( left, right );
	},
	sol::meta_function::division, []( const pod::Vector4f& left, const pod::Vector4f& right ) {
		return uf::vector::divide( left, right );
	},
	sol::meta_function::division, []( const pod::Vector4f& left, double right ) {
		return uf::vector::divide( left, right );
	},
	UF_LUA_REGISTER_USERTYPE_DEFINE( lerp, []( const pod::Vector4f& start, const pod::Vector4f& end, double delta ) {
		return uf::vector::lerp( start, end, delta );
	}),
	UF_LUA_REGISTER_USERTYPE_DEFINE( slerp, []( const pod::Vector4f& start, const pod::Vector4f& end, double delta ) {
		return uf::vector::slerp( start, end, delta );
	}),
	UF_LUA_REGISTER_USERTYPE_DEFINE( normalize, []( const pod::Vector3f& self ) {
		return uf::vector::normalize( self );
	}),
	UF_LUA_REGISTER_USERTYPE_DEFINE( magnitude, []( const pod::Vector3f& self ) {
		return uf::vector::magnitude( self );
	}),
	UF_LUA_REGISTER_USERTYPE_DEFINE( dot, []( const pod::Vector3f& left, const pod::Vector3f& right ) {
		return uf::vector::dot( left, right );
	}),
	UF_LUA_REGISTER_USERTYPE_DEFINE( __tostring, []( pod::Vector4f& self ) {
		return uf::string::toString( self );
	})
)
#endif