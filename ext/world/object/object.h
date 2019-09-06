#pragma once

#include <uf/config.h>
#include <uf/ext/ext.h>
#include <uf/engine/entity/entity.h>
/*
#include <uf/gl/shader/shader.h>
#include <uf/gl/mesh/mesh.h>
#include <uf/gl/mesh/skeletal/mesh.h>
#include <uf/gl/texture/texture.h>
*/
#include <uf/utils/camera/camera.h>

#include <uf/utils/math/transform.h>
#include <uf/utils/math/physics.h>
#include <uf/utils/math/collision.h>
#include <uf/utils/thread/thread.h>
#include <uf/utils/hook/hook.h>

namespace ext {
	class EXT_API Object : public uf::Entity {
	public:
		virtual void initialize();
		virtual void destroy();
		virtual void tick();
		virtual void render();

		bool load( const std::string& );
		bool load( const uf::Serializer& );
		std::size_t loadChild( const std::string&, bool = true );
		std::size_t loadChild( const uf::Serializer&, bool = true );

		void queueHook( const std::string&, const std::string& = "", double = 0 );
		std::vector<std::string> callHook( const std::string&, const std::string& = "" );
		std::size_t addHook( const std::string&, const uf::HookHandler::Readable::function_t& );
	};
}