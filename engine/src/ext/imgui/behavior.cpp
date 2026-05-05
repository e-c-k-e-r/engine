#include "behavior.h"

#include <uf/utils/hook/hook.h>
#include <uf/utils/io/fmt.h>
#include <uf/utils/io/payloads.h>
#include <uf/utils/io/console.h>
#include <uf/utils/window/payloads.h>
#include <uf/utils/thread/thread.h>

#if UF_USE_IMGUI
	#include <uf/ext/imgui/imgui.h>
	#include <imgui/imgui.h>
	#include <imgui/imgui_stdlib.h>

	#include "consoleWindow.inl"
	#include "threadMetrics.inl"
#endif

UF_BEHAVIOR_REGISTER_CPP(ext::ImguiBehavior)
UF_BEHAVIOR_TRAITS_CPP(ext::ImguiBehavior, ticks = false, renders = false, thread = "")
#define this (&self)
void ext::ImguiBehavior::initialize( uf::Object& self ) {
	auto& metadata = this->getComponent<ext::ImguiBehavior::Metadata>();
	auto& metadataJson = this->getComponent<uf::Serializer>();

#if UF_USE_IMGUI
	this->addHook( "gui:IMGUI.tick", [&](){
		tick( self );
	} );
#endif

	::consoleWindow.position.x = uf::renderer::settings::width - ::consoleWindow.size.x;

	UF_BEHAVIOR_METADATA_BIND_SERIALIZER_HOOKS(metadata, metadataJson);
}
void ext::ImguiBehavior::tick( uf::Object& self ) {
#if UF_USE_IMGUI
	// console window
	{
		bool opened;
		::consoleWindow.Draw("Console", opened);
	}
	// thread metrics window
	{
		bool opened = true;
		::threadMetrics.Draw("Thread Metrics", opened);
	}
#endif
}
void ext::ImguiBehavior::render( uf::Object& self ){

}
void ext::ImguiBehavior::destroy( uf::Object& self ){}
void ext::ImguiBehavior::Metadata::serialize( uf::Object& self, uf::Serializer& serializer ){
}
void ext::ImguiBehavior::Metadata::deserialize( uf::Object& self, uf::Serializer& serializer ){
}
#undef this