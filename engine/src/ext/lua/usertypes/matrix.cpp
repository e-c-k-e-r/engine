#include <uf/ext/lua/lua.h>
#if UF_USE_LUA
#include <uf/utils/math/matrix.h>

namespace binds {
	float index( const pod::Matrix4f& self, const size_t& index ) {
		return self[index];
	}
	pod::Matrix4f translate( const pod::Vector3f& v ) {
		return uf::matrix::translate( uf::matrix::identity(), v );
	}
	pod::Matrix4f scale( const pod::Vector3f& v ) {
		return uf::matrix::scale( uf::matrix::identity(), v );
	}
	pod::Matrix4f multiply( const pod::Matrix4f& left, const pod::Matrix4f& right ) {
		return uf::matrix::multiply( left, right );
	}
	pod::Matrix4f inverse( const pod::Matrix4f& m ) {
		return uf::matrix::inverse( m );
	}
	pod::Matrix4f identity( const pod::Matrix4f& ) {
		return uf::matrix::identity();
	}
}

UF_LUA_REGISTER_USERTYPE(pod::Matrix4f,
	sol::call_constructor, sol::initializers( 
		[]( pod::Matrix4f& self ) {
			return self = uf::matrix::identity();
		},
		[]( pod::Matrix4f& self, const pod::Matrix4f& copy ) {
			return self = copy;
		}
	),
	sol::meta_function::multiplication, UF_LUA_C_FUN( ::binds::multiply ),
	UF_LUA_REGISTER_USERTYPE_DEFINE( m, UF_LUA_C_FUN( ::binds::index ) ),
	UF_LUA_REGISTER_USERTYPE_DEFINE( multiply, UF_LUA_C_FUN( ::binds::multiply ) ),
	UF_LUA_REGISTER_USERTYPE_DEFINE( inverse, UF_LUA_C_FUN( ::binds::inverse ) ),
	UF_LUA_REGISTER_USERTYPE_DEFINE( translate, UF_LUA_C_FUN( ::binds::translate ) ),
	UF_LUA_REGISTER_USERTYPE_DEFINE( scale, UF_LUA_C_FUN( ::binds::scale ) ),
	UF_LUA_REGISTER_USERTYPE_DEFINE( identity, UF_LUA_C_FUN(uf::matrix::identity<float>) )
)
#endif