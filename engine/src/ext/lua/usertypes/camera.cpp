#include <uf/ext/lua/lua.h>
#if UF_USE_LUA
#include <uf/utils/camera/camera.h>
UF_LUA_REGISTER_USERTYPE(uf::Camera,
	UF_LUA_REGISTER_USERTYPE_DEFINE( getTransform, []( uf::Camera& self ) {
		return self.getTransform();
	}),
	UF_LUA_REGISTER_USERTYPE_DEFINE( getView, []( const uf::Camera& self, sol::variadic_args va ) {
		auto it = va.begin();
		if ( va.size() > 0 ) {
			size_t i = *it++;
			return self.getView(i);
		}
		return self.getView();
	}),
	UF_LUA_REGISTER_USERTYPE_DEFINE( getProjection, []( const uf::Camera& self, sol::variadic_args va ) {
		auto it = va.begin();
		if ( va.size() > 0 ) {
			size_t i = *it++;
			return self.getProjection(i);
		}
		return self.getProjection();
	}),
	UF_LUA_REGISTER_USERTYPE_DEFINE( setView, []( uf::Camera& self, const pod::Matrix4f& matrix, sol::variadic_args va ) {
		auto it = va.begin();
		if ( va.size() > 0 ) {
			size_t i = *it++;
			return self.setView(matrix, i);
		}
		return self.setView(matrix);
	}),
	UF_LUA_REGISTER_USERTYPE_DEFINE( setProjection, []( uf::Camera& self, const pod::Matrix4f& matrix, sol::variadic_args va ) {
		auto it = va.begin();
		if ( va.size() > 0 ) {
			size_t i = *it++;
			return self.setProjection(matrix, i);
		}
		return self.setProjection(matrix);
	}),
	UF_LUA_REGISTER_USERTYPE_MEMBER( uf::Camera::update )
)
#endif