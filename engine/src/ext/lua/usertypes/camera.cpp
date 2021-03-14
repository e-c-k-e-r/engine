#include <uf/ext/lua/lua.h>
#if UF_USE_LUA
#include <uf/utils/camera/camera.h>

namespace binds {
	pod::Transform<>& getTransform( uf::Camera& self ) {
		return self.getTransform();
	}
	pod::Matrix4f getView( const uf::Camera& self, sol::variadic_args va ) {
		auto it = va.begin();
		if ( va.size() > 0 ) {
			size_t i = *it++;
			return self.getView(i);
		}
		return self.getView();
	}
	pod::Matrix4f getProjection( const uf::Camera& self, sol::variadic_args va ) {
		auto it = va.begin();
		if ( va.size() > 0 ) {
			size_t i = *it++;
			return self.getProjection(i);
		}
		return self.getProjection();
	}
	void setView( uf::Camera& self, const pod::Matrix4f& matrix, sol::variadic_args va ) {
		auto it = va.begin();
		if ( va.size() > 0 ) {
			size_t i = *it++;
			self.setView(matrix, i);
		} else {
			self.setView(matrix);
		}
	}
	void setProjection( uf::Camera& self, const pod::Matrix4f& matrix, sol::variadic_args va ) {
		auto it = va.begin();
		if ( va.size() > 0 ) {
			size_t i = *it++;
			self.setProjection(matrix, i);
		} else {
			self.setProjection(matrix);
		}
	}
	void update( uf::Camera& self, bool force = true ) {
		self.update(force);
	}
}

UF_LUA_REGISTER_USERTYPE(uf::Camera,
	UF_LUA_REGISTER_USERTYPE_DEFINE( getTransform, UF_LUA_C_FUN(::binds::getTransform) ),
	UF_LUA_REGISTER_USERTYPE_DEFINE( getView, UF_LUA_C_FUN(::binds::getView) ),
	UF_LUA_REGISTER_USERTYPE_DEFINE( setView, UF_LUA_C_FUN(::binds::setView) ),
	UF_LUA_REGISTER_USERTYPE_DEFINE( getProjection, UF_LUA_C_FUN(::binds::getProjection) ),
	UF_LUA_REGISTER_USERTYPE_DEFINE( setProjection, UF_LUA_C_FUN(::binds::setProjection) ),
	UF_LUA_REGISTER_USERTYPE_DEFINE( update, UF_LUA_C_FUN(::binds::update) )
)
#endif