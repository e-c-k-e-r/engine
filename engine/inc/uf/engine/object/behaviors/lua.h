#pragma once

#include <uf/config.h>
#include <uf/engine/entity/entity.h>
#include <uf/engine/scene/scene.h>

namespace uf {
	namespace LuaBehavior {
		UF_BEHAVIOR_DEFINE_TYPE;
		void UF_API attach( uf::Object& );
		void UF_API initialize( uf::Object& );
		void UF_API tick( uf::Object& );
		void UF_API render( uf::Object& );
		void UF_API destroy( uf::Object& );
	}
}