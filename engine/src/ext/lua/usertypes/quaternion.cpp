#include <uf/ext/lua/lua.h>

#include <uf/utils/math/quaternion.h>

UF_LUA_REGISTER_USERTYPE(pod::Quaternion<>,
	sol::call_constructor, sol::initializers( 
		[]( pod::Quaternion<>& self, const pod::Quaternion<>& copy = {} ) {
			self = copy;
		},
		[]( pod::Quaternion<>& self, float x, float y, float z, float w ) {
			self = uf::vector::create(x, y, z, w);
		}
	),
	UF_LUA_REGISTER_USERTYPE_MEMBER(pod::Quaternion<>::x),
	UF_LUA_REGISTER_USERTYPE_MEMBER(pod::Quaternion<>::y),
	UF_LUA_REGISTER_USERTYPE_MEMBER(pod::Quaternion<>::z),
	UF_LUA_REGISTER_USERTYPE_MEMBER(pod::Quaternion<>::w),
	UF_LUA_REGISTER_USERTYPE_DEFINE(v, []( pod::Quaternion<>& self, const size_t& index ) {
		return self[index];
	}),
	UF_LUA_REGISTER_USERTYPE_DEFINE( normalize, []( const pod::Quaternion<>& self ) {
		return uf::quaternion::normalize( self );
	}),
	UF_LUA_REGISTER_USERTYPE_DEFINE( multiply, []( const pod::Quaternion<>& left, const pod::Quaternion<>& right ) {
		return uf::quaternion::multiply( left, right );
	}),
	UF_LUA_REGISTER_USERTYPE_DEFINE(axisAngle, []( sol::object arg, float angle ){
		if ( arg.is<pod::Vector3f>() ) {
			return uf::quaternion::axisAngle( arg.as<pod::Vector3f>(), angle );
		} else if ( arg.is<sol::table>() ) {
			sol::table table = arg.as<sol::table>();
			return uf::quaternion::axisAngle( pod::Vector3f{ table[0], table[1], table[2] }, angle );
		}
		return pod::Quaternion<>{};
	}),
	UF_LUA_REGISTER_USERTYPE_DEFINE( rotate, []( const pod::Quaternion<>& left, const pod::Vector3f& right ) {
		return uf::quaternion::rotate( left, right );
	}),
	UF_LUA_REGISTER_USERTYPE_DEFINE( eulerAngles, []( const pod::Quaternion<>& quaternion ) {
		return uf::quaternion::eulerAngles( quaternion );
	}),
	UF_LUA_REGISTER_USERTYPE_DEFINE( pitch, []( const pod::Quaternion<>& quaternion ) {
		return uf::quaternion::pitch( quaternion );
	}),
	UF_LUA_REGISTER_USERTYPE_DEFINE( yaw, []( const pod::Quaternion<>& quaternion ) {
		return uf::quaternion::yaw( quaternion );
	}),
	UF_LUA_REGISTER_USERTYPE_DEFINE( roll, []( const pod::Quaternion<>& quaternion ) {
		return uf::quaternion::roll( quaternion );
	}),
	UF_LUA_REGISTER_USERTYPE_DEFINE( slerp, []( const pod::Quaternion<>& left, const pod::Quaternion<>& right, float a ) {
		return uf::quaternion::slerp( left, right, a );
	}),
	UF_LUA_REGISTER_USERTYPE_DEFINE( __tostring, []( const pod::Quaternion<>& self ) {
		return uf::string::toString( self );
	})
)