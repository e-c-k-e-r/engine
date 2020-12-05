#include <uf/ext/lua/lua.h>

#include <uf/utils/math/matrix.h>

UF_LUA_REGISTER_USERTYPE(pod::Matrix4f,
	sol::call_constructor, sol::initializers( 
		[]( pod::Matrix4f& self ) {
			return self = uf::matrix::identity();
		},
		[]( pod::Matrix4f& self, const pod::Matrix4f& copy ) {
			return self = copy;
		}
	),
	sol::meta_function::multiplication, []( const pod::Matrix4f& left, const pod::Matrix4f& right ) {
		return uf::matrix::multiply( left, right );
	},
	UF_LUA_REGISTER_USERTYPE_DEFINE( m, []( pod::Matrix4f& self, const size_t& index ) {
		return self[index];
	}),
	UF_LUA_REGISTER_USERTYPE_DEFINE( multiply, []( const pod::Matrix4f& left, const pod::Matrix4f& right ) {
		return uf::matrix::multiply( left, right );
	}),
	UF_LUA_REGISTER_USERTYPE_DEFINE( inverse, []( const pod::Matrix4f& m ) {
		return uf::matrix::inverse( m );
	}),
	UF_LUA_REGISTER_USERTYPE_DEFINE( translate, []( const pod::Vector3f& v ) {
		return uf::matrix::translate( uf::matrix::identity(), v );
	}),
	UF_LUA_REGISTER_USERTYPE_DEFINE( scale, []( const pod::Vector3f& v ) {
		return uf::matrix::scale( uf::matrix::identity(), v );
	}),
	UF_LUA_REGISTER_USERTYPE_DEFINE( identity, []( ) {
		return uf::matrix::identity();
	})
)
