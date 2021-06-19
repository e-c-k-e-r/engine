#pragma once

namespace uf {
	namespace SceneBehavior {
		UF_BEHAVIOR_DEFINE_TYPE;
		void UF_API attach( uf::Entity& );
		void UF_API initialize( uf::Object& );
		void UF_API tick( uf::Object& );
		void UF_API render( uf::Object& );
		void UF_API destroy( uf::Object& );
		struct Metadata {
			std::vector<uf::Entity*> graph;
			bool invalidationQueued = false;

			std::function<void()> serialize;
			std::function<void()> deserialize;
		};
	}
}