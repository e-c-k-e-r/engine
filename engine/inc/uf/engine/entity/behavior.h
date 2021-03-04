#pragma once

namespace uf {
	namespace EntityBehavior {
		UF_BEHAVIOR_DEFINE_TYPE;
		void UF_API attach( uf::Entity& );
		void UF_API initialize( uf::Object& );
		void UF_API tick( uf::Object& );
		void UF_API render( uf::Object& );
		void UF_API destroy( uf::Object& );
	}
}