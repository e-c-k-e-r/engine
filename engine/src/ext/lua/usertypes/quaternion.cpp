#include <uf/ext/lua/lua.h>
#if UF_USE_LUA
#include <uf/utils/math/quaternion.h>

namespace {
	typedef pod::Quaternion<> Quaternion;
}

namespace binds {
	float index( const ::Quaternion& self, size_t index ) {
		return self[index];
	}
	::Quaternion lookAt( const pod::Vector3f& at, const pod::Vector3f& up ) {
		return uf::quaternion::lookAt( at, up );
	}
	::Quaternion normalize( const ::Quaternion& self ) {
		return uf::quaternion::normalize( self );
	}
	::Quaternion multiply( const ::Quaternion& left, const ::Quaternion& right ) {
		return uf::quaternion::multiply( left, right );
	}
	::Quaternion axisAngle( sol::object arg, float angle ){
		if ( arg.is<pod::Vector3f>() ) {
			return uf::quaternion::axisAngle( arg.as<pod::Vector3f>(), angle );
		} else if ( arg.is<sol::table>() ) {
			sol::table table = arg.as<sol::table>();
			return uf::quaternion::axisAngle( pod::Vector3f{ table[0], table[1], table[2] }, angle );
		}
		return ::Quaternion{};
	}
	pod::Vector3f rotate( const ::Quaternion& left, const pod::Vector3f& right ) {
		return uf::quaternion::rotate( left, right );
	}
	::Quaternion eulerAngles( const ::Quaternion& quaternion ) {
		return uf::quaternion::eulerAngles( quaternion );
	}
	::Quaternion conjugate( const ::Quaternion& quaternion ) {
		return uf::quaternion::conjugate( quaternion );
	}
	::Quaternion inverse( const ::Quaternion& quaternion ) {
		return uf::quaternion::inverse( quaternion );
	}
	float pitch( const ::Quaternion& quaternion ) {
		return uf::quaternion::pitch( quaternion );
	}
	float yaw( const ::Quaternion& quaternion ) {
		return uf::quaternion::yaw( quaternion );
	}
	float roll( const ::Quaternion& quaternion ) {
		return uf::quaternion::roll( quaternion );
	}
	::Quaternion slerp( const ::Quaternion& left, const ::Quaternion& right, float a ) {
		return uf::quaternion::slerp( left, right, a );
	}
	pod::Matrix4f matrix( const ::Quaternion& q ) {
		return uf::quaternion::matrix( q );
	}
	uf::stl::string __tostring( const ::Quaternion& self ) {
		return uf::string::toString( self );
	}
}

UF_LUA_REGISTER_USERTYPE(::Quaternion,
	sol::call_constructor, sol::initializers( 
		[]( ::Quaternion& self ) {
			return self = {0,0,0,1};
		},
		[]( ::Quaternion& self, const ::Quaternion& copy ) {
			return self = copy;
		},
		[]( ::Quaternion& self, float x, float y, float z, float w ) {
			return self = uf::vector::create(x, y, z, w);
		}
	),

	UF_LUA_REGISTER_USERTYPE_MEMBER(::Quaternion::x),
	UF_LUA_REGISTER_USERTYPE_MEMBER(::Quaternion::y),
	UF_LUA_REGISTER_USERTYPE_MEMBER(::Quaternion::z),
	UF_LUA_REGISTER_USERTYPE_MEMBER(::Quaternion::w),

	UF_LUA_REGISTER_USERTYPE_DEFINE( v, UF_LUA_C_FUN(::binds::index) ),
	UF_LUA_REGISTER_USERTYPE_DEFINE( lookAt, UF_LUA_C_FUN(::binds::lookAt) ),
	UF_LUA_REGISTER_USERTYPE_DEFINE( normalize, UF_LUA_C_FUN(::binds::normalize) ),
	UF_LUA_REGISTER_USERTYPE_DEFINE( multiply, UF_LUA_C_FUN(::binds::multiply) ),
	sol::meta_function::multiplication, UF_LUA_C_FUN(::binds::multiply),
	UF_LUA_REGISTER_USERTYPE_DEFINE(axisAngle, UF_LUA_C_FUN(::binds::axisAngle) ),
	UF_LUA_REGISTER_USERTYPE_DEFINE( rotate, UF_LUA_C_FUN( ::binds::rotate ) ),
	UF_LUA_REGISTER_USERTYPE_DEFINE( eulerAngles, UF_LUA_C_FUN( ::binds::eulerAngles ) ),
	UF_LUA_REGISTER_USERTYPE_DEFINE( conjugate, UF_LUA_C_FUN( ::binds::conjugate ) ),
	UF_LUA_REGISTER_USERTYPE_DEFINE( inverse, UF_LUA_C_FUN( ::binds::inverse ) ),
	UF_LUA_REGISTER_USERTYPE_DEFINE( pitch, UF_LUA_C_FUN( ::binds::pitch ) ),
	UF_LUA_REGISTER_USERTYPE_DEFINE( yaw, UF_LUA_C_FUN( ::binds::yaw ) ), 
	UF_LUA_REGISTER_USERTYPE_DEFINE( roll, UF_LUA_C_FUN( ::binds::roll ) ),
	UF_LUA_REGISTER_USERTYPE_DEFINE( slerp, UF_LUA_C_FUN( ::binds::slerp ) ),
	UF_LUA_REGISTER_USERTYPE_DEFINE( matrix, UF_LUA_C_FUN( ::binds::matrix ) ),
	UF_LUA_REGISTER_USERTYPE_DEFINE( __tostring, UF_LUA_C_FUN( ::binds::__tostring ) )
)
#endif