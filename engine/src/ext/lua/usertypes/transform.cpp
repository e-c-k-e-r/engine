#include <uf/ext/lua/lua.h>
#if UF_USE_LUA
#include <uf/utils/math/transform.h>

namespace binds {
	void move( pod::Transform<>& self, sol::variadic_args va ) {
		auto it = va.begin();
		if ( va.size() == 1 ) {
			pod::Vector3f delta = *it++;
			self = uf::transform::move( self, delta );
		} else if ( va.size() == 2 ) {
			pod::Vector3f axis = *it++;
			double delta = *it++;
			self = uf::transform::move( self, axis, delta );
		}
	}
	void rotate( pod::Transform<>& self, sol::variadic_args va ) {
		auto it = va.begin();
		if ( va.size() == 1 ) {
			pod::Quaternion<> delta = *it++;
			self = uf::transform::rotate( self, delta );
		} else if ( va.size() == 2 ) {
			pod::Vector3f axis = *it++;
			double delta = *it++;
			self = uf::transform::rotate( self, axis, delta );
		}
	}
	pod::Transform<> flatten( const pod::Transform<>& t ) {
		return uf::transform::flatten( t );
	}
	pod::Transform<> reorient( const pod::Transform<>& t ) {
		return uf::transform::reorient( t );
	}
	pod::Transform<> getReference( pod::Transform<>& t ) {
		return t.reference ? *t.reference : t;
	}
	void setReference( pod::Transform<>& self, pod::Transform<>& t ) {
		self.reference = &t;
	}
	void unreference( pod::Transform<>& self ) {
		self.reference = NULL;
	}
	pod::Transform<> lookAt( const pod::Transform<>& t, pod::Vector3f& at ) {
		auto transform = t;
		return uf::transform::lookAt( transform, at );
	}
	pod::Matrix4f getModel( const pod::Transform<>& t ) {
		return uf::transform::model( t );
	}
}

UF_LUA_REGISTER_USERTYPE(pod::Transform<>,
	UF_LUA_REGISTER_USERTYPE_MEMBER(pod::Transform<>::position),
	UF_LUA_REGISTER_USERTYPE_MEMBER(pod::Transform<>::scale),
	UF_LUA_REGISTER_USERTYPE_MEMBER(pod::Transform<>::up),
	UF_LUA_REGISTER_USERTYPE_MEMBER(pod::Transform<>::right),
	UF_LUA_REGISTER_USERTYPE_MEMBER(pod::Transform<>::forward),
	UF_LUA_REGISTER_USERTYPE_MEMBER(pod::Transform<>::orientation),
	UF_LUA_REGISTER_USERTYPE_MEMBER(pod::Transform<>::model),
	
	UF_LUA_REGISTER_USERTYPE_DEFINE(move, UF_LUA_C_FUN(::binds::move) ),
	UF_LUA_REGISTER_USERTYPE_DEFINE(rotate, UF_LUA_C_FUN(::binds::rotate)),
	UF_LUA_REGISTER_USERTYPE_DEFINE(flatten, UF_LUA_C_FUN(::binds::flatten)),
	UF_LUA_REGISTER_USERTYPE_DEFINE(reorient, UF_LUA_C_FUN(::binds::reorient)),
	UF_LUA_REGISTER_USERTYPE_DEFINE(getReference, UF_LUA_C_FUN(::binds::getReference)),
	UF_LUA_REGISTER_USERTYPE_DEFINE(setReference, UF_LUA_C_FUN(::binds::setReference)),
	UF_LUA_REGISTER_USERTYPE_DEFINE(unreference, UF_LUA_C_FUN(::binds::unreference)),
	UF_LUA_REGISTER_USERTYPE_DEFINE(lookAt, UF_LUA_C_FUN(::binds::lookAt)),
	UF_LUA_REGISTER_USERTYPE_DEFINE(getModel, UF_LUA_C_FUN(::binds::getModel))
)
#endif