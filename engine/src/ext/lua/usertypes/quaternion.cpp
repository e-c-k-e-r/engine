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
	UF_LUA_REGISTER_USERTYPE_DEFINE( __tostring, []( pod::Quaternion<>& self ) {
		std::stringstream ss;
		ss << "Quaterion(" << self.x << ", " << self.y << ", " << self.z << ", " << self.w << ")";
		return ss.str();
	}),
	UF_LUA_REGISTER_USERTYPE_DEFINE(axisAngle, []( sol::object arg, float angle ){
		if ( arg.is<pod::Vector3f>() ) {
			return uf::quaternion::axisAngle( arg.as<pod::Vector3f>(), angle );
		} else if ( arg.is<sol::table>() ) {
			sol::table table = arg.as<sol::table>();
			return uf::quaternion::axisAngle( pod::Vector3f{ table[0], table[1], table[2] }, angle );
		}
		return pod::Quaternion<>{};
	})
)