#pragma once

#include <uf/config.h>
#include <uf/ext/ext.h>
#include <uf/engine/entity/entity.h>
#include <uf/engine/scene/scene.h>

namespace ext {
	class EXT_API PortalsBehavior {
	public:
		static void attach( uf::Object& );
		static void initialize( uf::Object& );
		static void tick( uf::Object& );
		static void render( uf::Object& );
		static void destroy( uf::Object& );
	};
	class EXT_API PortalBehavior {
	public:
		static void attach( uf::Object& );
		static void initialize( uf::Object& );
		static void tick( uf::Object& );
		static void render( uf::Object& );
		static void destroy( uf::Object& );
	};
}