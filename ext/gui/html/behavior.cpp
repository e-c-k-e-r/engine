#include "behavior.h"
#include "../gui.h"

#include <uf/utils/hook/hook.h>
#include <uf/utils/time/time.h>
#include <uf/utils/serialize/serializer.h>
#include <uf/utils/userdata/userdata.h>
#include <uf/utils/window/window.h>
#include <uf/utils/camera/camera.h>
#include <uf/utils/graphic/mesh.h>
#include <uf/utils/graphic/graphic.h>
#include <uf/utils/string/ext.h>
#include <uf/utils/math/physics.h>

#include <uf/utils/text/glyph.h>
#include <uf/engine/asset/asset.h>
#include <uf/engine/scene/scene.h>

#include <unordered_map>
#include <locale>
#include <codecvt>

#include <uf/utils/renderer/renderer.h>
#include <uf/ext/openvr/openvr.h>

#include <uf/utils/http/http.h>
#include <uf/utils/audio/audio.h>
#include <sys/stat.h>
#include <fstream>

#include <regex>
#include <uf/ext/ultralight/ultralight.h>

UF_BEHAVIOR_REGISTER_CPP(ext::GuiHtmlBehavior)
#define this ((uf::Object*) &self)
void ext::GuiHtmlBehavior::initialize( uf::Object& self ) {
#if UF_USE_ULTRALIGHT
	auto& page = this->getComponent<pod::HTML>();
	auto& metadata = this->getComponent<uf::Serializer>();
	pod::Vector2ui size = { 1, 1 };
	if ( ext::json::isArray( metadata["size"] ) ) {
		size = {
			metadata["size"][0].as<size_t>(),
			metadata["size"][1].as<size_t>(),
		};
	} else {		
		size = ext::gui::size.current;
	}
	if ( size.x <= 0 && size.y <= 0 ) {
		size.x = ext::config["window"]["size"]["x"].as<size_t>();
		size.y = ext::config["window"]["size"]["y"].as<size_t>();
	}
	page = ext::ultralight::create( page, metadata["html"].as<std::string>(), size);
	std::string onLoad = this->formatHookName("html:Load.%UID%");
	if ( metadata["wait for load"].as<bool>() ) {	
		ext::ultralight::on(page, "load", onLoad);
		this->addHook( onLoad, [&](ext::json::Value& json){
			auto image = ext::ultralight::capture( page );
			this->as<ext::Gui>().load( image );
		});
	} else {
		auto image = ext::ultralight::capture( page );
		this->as<ext::Gui>().load( image );
	}

	this->addHook( "window:Resized", [&](ext::json::Value& json){
		if ( !this->hasComponent<uf::GuiMesh>() ) return;

		pod::Vector2ui size; {
			size.x = json["window"]["size"]["x"].as<size_t>();
			size.y = json["window"]["size"]["y"].as<size_t>();
		};

		metadata["size"][0] = size.x;
		metadata["size"][1] = size.y;

		ext::ultralight::resize( page, size );
	});
	
	this->addHook( "window:Key", [&](ext::json::Value& json){
		if ( json["type"].as<std::string>() == "window:Text.Entered" ) return;
		if ( metadata["ignore inputs"].as<bool>() ) return;
		ext::ultralight::input( page, json );
	});
	this->addHook( "window:Text.Entered", [&](ext::json::Value& json){
		if ( metadata["ignore inputs"].as<bool>() ) return;
		ext::ultralight::input( page, json );
	});
	this->addHook( "window:Mouse.Wheel", [&](ext::json::Value& json){
		if ( metadata["ignore inputs"].as<bool>() ) return;
		ext::ultralight::input( page, json );
	});
	this->addHook( "gui:Mouse.Clicked.%UID%", [&](ext::json::Value& json){
		if ( metadata["ignore inputs"].as<bool>() ) return;
		ext::ultralight::input( page, json );
	});
	this->addHook( "gui:Mouse.Moved.%UID%", [&](ext::json::Value& json){
		if ( metadata["ignore inputs"].as<bool>() ) return;
		ext::ultralight::input( page, json );
	});
#endif
}
void ext::GuiHtmlBehavior::tick( uf::Object& self ) {
#if UF_USE_ULTRALIGHT
	auto& metadata = this->getComponent<uf::Serializer>();
	auto& page = this->getComponent<pod::HTML>();

	bool should = metadata["update"].as<bool>() || page.pending;
	if ( !this->hasComponent<uf::Graphic>() || !should ) return;
	auto& graphic = this->getComponent<uf::Graphic>();
	auto& texture = graphic.material.textures.front();
	auto image = ext::ultralight::capture( page );
	texture.update( image );
//	page.pending = false;
#endif
}
void ext::GuiHtmlBehavior::render( uf::Object& self ){
}
void ext::GuiHtmlBehavior::destroy( uf::Object& self ){
}
#undef this