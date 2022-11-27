#include <uf/ext/lua/lua.h>
#if UF_USE_LUA
#include <uf/utils/math/vector.h>

namespace binds {
	namespace v3f {
		float index( const pod::Vector3f& self, size_t index ) {
			return self[index];
		}
		float distance( pod::Vector3f& self, const pod::Vector3f& other ) {
			return uf::vector::distance( self, other );
		}
		pod::Vector3f add( const pod::Vector3f& left, const pod::Vector3f& right ) {
			return uf::vector::add( left, right );
		}
		pod::Vector3f sub( const pod::Vector3f& left, const pod::Vector3f& right ) {
			return uf::vector::subtract( left, right );
		}
		pod::Vector3f mul( const pod::Vector3f& left, const pod::Vector3f& right ) {
			return uf::vector::multiply( left, right );
		}
		pod::Vector3f div( const pod::Vector3f& left, const pod::Vector3f& right ) {
			return uf::vector::divide( left, right );
		}
		pod::Vector3f mulS( const pod::Vector3f& left, float right ) {
			return uf::vector::multiply( left, right );
		}
		pod::Vector3f divS( const pod::Vector3f& left, float right ) {
			return uf::vector::divide( left, right );
		}
		pod::Vector3f lerp( const pod::Vector3f& start, const pod::Vector3f& end, double delta ) {
			return uf::vector::lerp( start, end, delta );
		}
		pod::Vector3f normalize( const pod::Vector3f& self ) {
			return uf::vector::normalize( self );
		}
		float magnitude( const pod::Vector3f& self ) {
			return uf::vector::magnitude( self );
		}
		float norm( const pod::Vector3f& self ) {
			return uf::vector::norm( self );
		}
		float dot( const pod::Vector3f& left, const pod::Vector3f& right ) {
			return uf::vector::dot( left, right );
		}
		uf::stl::string toString( pod::Vector3f& self ) {
			return uf::string::toString( self );
		}
	}
	namespace v4f {
		float index( const pod::Vector4f& self, size_t index ) {
			return self[index];
		}
		float distance( pod::Vector4f& self, const pod::Vector4f& other ) {
			return uf::vector::distance( self, other );
		}
		pod::Vector4f add( const pod::Vector4f& left, const pod::Vector4f& right ) {
			return uf::vector::add( left, right );
		}
		pod::Vector4f sub( const pod::Vector4f& left, const pod::Vector4f& right ) {
			return uf::vector::subtract( left, right );
		}
		pod::Vector4f mul( const pod::Vector4f& left, const pod::Vector4f& right ) {
			return uf::vector::multiply( left, right );
		}
		pod::Vector4f div( const pod::Vector4f& left, const pod::Vector4f& right ) {
			return uf::vector::divide( left, right );
		}
		pod::Vector4f mulS( const pod::Vector4f& left, float right ) {
			return uf::vector::multiply( left, right );
		}
		pod::Vector4f divS( const pod::Vector4f& left, float right ) {
			return uf::vector::divide( left, right );
		}
		pod::Vector4f lerp( const pod::Vector4f& start, const pod::Vector4f& end, double delta ) {
			return uf::vector::lerp( start, end, delta );
		}
		pod::Vector4f normalize( const pod::Vector4f& self ) {
			return uf::vector::normalize( self );
		}
		float magnitude( const pod::Vector4f& self ) {
			return uf::vector::magnitude( self );
		}
		float norm( const pod::Vector4f& self ) {
			return uf::vector::norm( self );
		}
		float dot( const pod::Vector4f& left, const pod::Vector4f& right ) {
			return uf::vector::dot( left, right );
		}
		uf::stl::string toString( pod::Vector4f& self ) {
			return uf::string::toString( self );
		}
	}
}

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
	
	UF_LUA_REGISTER_USERTYPE_DEFINE( v, UF_LUA_C_FUN( ::binds::v3f::index )),
	UF_LUA_REGISTER_USERTYPE_DEFINE( distance, UF_LUA_C_FUN( ::binds::v3f::distance )),
	sol::meta_function::addition, UF_LUA_C_FUN( ::binds::v3f::add ),
	sol::meta_function::subtraction, UF_LUA_C_FUN( ::binds::v3f::sub ),
	sol::meta_function::multiplication, UF_LUA_C_FUN( ::binds::v3f::mul ),
	sol::meta_function::multiplication, UF_LUA_C_FUN( ::binds::v3f::mulS ),
	sol::meta_function::division, UF_LUA_C_FUN( ::binds::v3f::div ),
	sol::meta_function::division, UF_LUA_C_FUN( ::binds::v3f::divS ),
	UF_LUA_REGISTER_USERTYPE_DEFINE( lerp, UF_LUA_C_FUN(::binds::v3f::lerp) ),
	UF_LUA_REGISTER_USERTYPE_DEFINE( normalize, UF_LUA_C_FUN(::binds::v3f::normalize) ),
	UF_LUA_REGISTER_USERTYPE_DEFINE( magnitude, UF_LUA_C_FUN(::binds::v3f::magnitude) ),
	UF_LUA_REGISTER_USERTYPE_DEFINE( norm, UF_LUA_C_FUN(::binds::v3f::norm) ),
	UF_LUA_REGISTER_USERTYPE_DEFINE( dot, UF_LUA_C_FUN(::binds::v3f::dot) ),
	UF_LUA_REGISTER_USERTYPE_DEFINE( __tostring, UF_LUA_C_FUN(::binds::v3f::toString) )
)
/*
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

	UF_LUA_REGISTER_USERTYPE_DEFINE( v, UF_LUA_C_FUN( ::binds::v4f::index )),
	UF_LUA_REGISTER_USERTYPE_DEFINE( distance, UF_LUA_C_FUN( ::binds::v4f::distance )),
	sol::meta_function::addition, UF_LUA_C_FUN( ::binds::v4f::add ),
	sol::meta_function::subtraction, UF_LUA_C_FUN( ::binds::v4f::sub ),
	sol::meta_function::multiplication, UF_LUA_C_FUN( ::binds::v4f::mul ),
	sol::meta_function::multiplication, UF_LUA_C_FUN( ::binds::v4f::mulS ),
	sol::meta_function::division, UF_LUA_C_FUN( ::binds::v4f::div ),
	sol::meta_function::division, UF_LUA_C_FUN( ::binds::v4f::divS ),
	UF_LUA_REGISTER_USERTYPE_DEFINE( lerp, UF_LUA_C_FUN(::binds::v4f::lerp) ),
	UF_LUA_REGISTER_USERTYPE_DEFINE( normalize, UF_LUA_C_FUN(::binds::v4f::normalize) ),
	UF_LUA_REGISTER_USERTYPE_DEFINE( magnitude, UF_LUA_C_FUN(::binds::v4f::magnitude) ),
	UF_LUA_REGISTER_USERTYPE_DEFINE( norm, UF_LUA_C_FUN(::binds::v4f::norm) ),
	UF_LUA_REGISTER_USERTYPE_DEFINE( dot, UF_LUA_C_FUN(::binds::v4f::dot) ),
	UF_LUA_REGISTER_USERTYPE_DEFINE( __tostring, UF_LUA_C_FUN(::binds::v4f::toString) )
)
*/
#endif