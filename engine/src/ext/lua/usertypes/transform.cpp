#include <uf/ext/lua/lua.h>

#include <uf/utils/math/transform.h>

UF_LUA_REGISTER_USERTYPE(pod::Transform<>,
	UF_LUA_REGISTER_USERTYPE_MEMBER(pod::Transform<>::position),
	UF_LUA_REGISTER_USERTYPE_MEMBER(pod::Transform<>::scale),
	UF_LUA_REGISTER_USERTYPE_MEMBER(pod::Transform<>::up),
	UF_LUA_REGISTER_USERTYPE_MEMBER(pod::Transform<>::right),
	UF_LUA_REGISTER_USERTYPE_MEMBER(pod::Transform<>::forward),
	UF_LUA_REGISTER_USERTYPE_MEMBER(pod::Transform<>::orientation),
	UF_LUA_REGISTER_USERTYPE_MEMBER(pod::Transform<>::model),
	UF_LUA_REGISTER_USERTYPE_DEFINE(move, []( pod::Transform<>& self, sol::variadic_args va ) {
		auto it = va.begin();
		if ( va.size() == 1 ) {
			pod::Vector3f delta = *it++;
			self = uf::transform::move( self, delta );
		} else if ( va.size() == 2 ) {
			pod::Vector3f axis = *it++;
			double delta = *it++;
			self = uf::transform::move( self, axis, delta );
		}
	}),
	UF_LUA_REGISTER_USERTYPE_DEFINE(rotate, []( pod::Transform<>& self, sol::variadic_args va ) {
		auto it = va.begin();
		if ( va.size() == 1 ) {
			pod::Quaternion<> delta = *it++;
			self = uf::transform::rotate( self, delta );
		} else if ( va.size() == 2 ) {
			pod::Vector3f axis = *it++;
			double delta = *it++;
			self = uf::transform::rotate( self, axis, delta );
		}
	}),
	UF_LUA_REGISTER_USERTYPE_DEFINE(flatten, []( const pod::Transform<>& t ) {
		return uf::transform::flatten(t);
	}),
	UF_LUA_REGISTER_USERTYPE_DEFINE(reorient, []( const pod::Transform<>& t ) {
		return uf::transform::reorient( t );
	}),
	UF_LUA_REGISTER_USERTYPE_DEFINE(getReference, []( pod::Transform<>& t ) {
		return t.reference ? *t.reference : t;
	}),
	UF_LUA_REGISTER_USERTYPE_DEFINE(lookAt, []( const pod::Transform<>& t, pod::Vector3f& at ) {
		auto transform = t;
		return uf::transform::lookAt( transform, at );
	}),
	UF_LUA_REGISTER_USERTYPE_DEFINE(getModel, []( const pod::Transform<>& t ) {
		return uf::transform::model( t );
	})
)