#include <uf/ext/lua/lua.h>

#include <uf/utils/math/vector.h>

UF_LUA_REGISTER_USERTYPE(pod::Vector3f,
	sol::call_constructor, sol::initializers( 
		[]( pod::Vector3f& self, const pod::Vector3f& copy = {} ) {
			self = copy;
		},
		[]( pod::Vector3f& self, float x, float y, float z ) {
			self = uf::vector::create(x, y, z);
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
	UF_LUA_REGISTER_USERTYPE_DEFINE( lerp, []( const pod::Vector3f& start, const pod::Vector3f& end, double delta ) {
		return uf::vector::lerp( start, end, delta );
	}),
	UF_LUA_REGISTER_USERTYPE_DEFINE( __tostring, []( pod::Vector3f& self ) {
		std::stringstream ss;
		ss << "Vector(" << self.x << ", " << self.y << ", " << self.z << ")";
		return ss.str();
	})
)
UF_LUA_REGISTER_USERTYPE(pod::Vector4f,
	sol::call_constructor, sol::initializers( 
		[]( pod::Vector4f& self, const pod::Vector4f& copy = {} ) {
			self = copy;
		},
		[]( pod::Vector4f& self, float x, float y, float z, float w ) {
			self = uf::vector::create(x, y, z, w);
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
	UF_LUA_REGISTER_USERTYPE_DEFINE( lerp, []( const pod::Vector4f& start, const pod::Vector4f& end, double delta ) {
		return uf::vector::lerp( start, end, delta );
	}),
	UF_LUA_REGISTER_USERTYPE_DEFINE( __tostring, []( pod::Vector4f& self ) {
		std::stringstream ss;
		ss << "Vector(" << self.x << ", " << self.y << ", " << self.z << ", " << self.w << ")";
		return ss.str();
	})
)