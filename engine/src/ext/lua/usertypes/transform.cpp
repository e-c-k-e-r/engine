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

	pod::Transform<> fromMatrix( const pod::Matrix4f& matrix ) {
		return uf::transform::fromMatrix( matrix );
	}
	pod::Transform<>& reference( pod::Transform<>& transform, const pod::Transform<>& parent, sol::optional<bool> reorient ) {
		return uf::transform::reference( transform, parent, reorient.value_or(true) );
	}
	pod::Transform<> interpolate( const pod::Transform<>& from, const pod::Transform<>& to, float factor, sol::optional<bool> reorient ) {
		return uf::transform::interpolate( from, to, factor, reorient.value_or(true) );
	}
	pod::Transform<> inverse(const pod::Transform<>& t) {
		return uf::transform::inverse( t );
	}
	pod::Vector3f apply( const pod::Transform<>& transform, const pod::Vector3f& point ) {
		return uf::transform::apply( transform, point );
	}
	pod::Vector3f applyInverse(const pod::Transform<>& t, const pod::Vector3f& worldPoint) {
		return uf::transform::applyInverse( t, worldPoint );
	}
	pod::Transform<> relative(const pod::Transform<>& a, const pod::Transform<>& b) {
		return uf::transform::relative( a, b );
	}
}

#include <uf/ext/lua/component.h>
UF_LUA_REGISTER_USERTYPE_AND_COMPONENT(pod::Transform<>,
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
	UF_LUA_REGISTER_USERTYPE_DEFINE(getModel, UF_LUA_C_FUN(::binds::getModel)),
	UF_LUA_REGISTER_USERTYPE_DEFINE(fromMatrix, UF_LUA_C_FUN(::binds::fromMatrix)),
	UF_LUA_REGISTER_USERTYPE_DEFINE(reference, UF_LUA_C_FUN(::binds::reference)),
	UF_LUA_REGISTER_USERTYPE_DEFINE(interpolate, UF_LUA_C_FUN(::binds::interpolate)),
	UF_LUA_REGISTER_USERTYPE_DEFINE(inverse, UF_LUA_C_FUN(::binds::inverse)),
	UF_LUA_REGISTER_USERTYPE_DEFINE(apply, UF_LUA_C_FUN(::binds::apply)),
	UF_LUA_REGISTER_USERTYPE_DEFINE(applyInverse, UF_LUA_C_FUN(::binds::applyInverse)),
	UF_LUA_REGISTER_USERTYPE_DEFINE(relative, UF_LUA_C_FUN(::binds::relative))
)
#endif