#if 0
#include "behavior.h"

#include <uf/utils/hook/hook.h>
#include <uf/utils/time/time.h>
#include <uf/utils/serialize/serializer.h>
#include <uf/utils/userdata/userdata.h>
#include <uf/utils/graphic/mesh.h>
#include <uf/utils/graphic/graphic.h>
#include <uf/utils/window/window.h>
#include <uf/utils/camera/camera.h>

#include <uf/utils/renderer/renderer.h>

#include <uf/utils/image/image.h>

#include <uf/engine/asset/asset.h>
#include "../../scenes/worldscape//battle.h"
#include "../../scenes/worldscape//.h"

namespace {
	uf::Serializer masterTableGet( const std::string& table ) {
		uf::Scene& scene = uf::scene::getCurrentScene();
		uf::Serializer& mastertable = scene.getComponent<uf::Serializer>();
		return mastertable["system"]["mastertable"][table];
	}
	uf::Serializer masterDataGet( const std::string& table, const std::string& key ) {
		uf::Scene& scene = uf::scene::getCurrentScene();
		uf::Serializer& mastertable = scene.getComponent<uf::Serializer>();
		return mastertable["system"]["mastertable"][table][key];
	}
	inline int64_t parseInt( const std::string& str ) {
		return atoi(str.c_str());
	}
}

UF_BEHAVIOR_REGISTER_CPP(ext::HousamoSpriteBehavior)
#define this (&self)
void ext::HousamoSpriteBehavior::initialize( uf::Object& self ) {
	uf::Scene& scene = uf::scene::getCurrentScene();
	uf::Serializer& metadata = this->getComponent<uf::Serializer>();
	uf::Serializer& masterdata = scene.getComponent<uf::Serializer>();
	uf::Asset& assetLoader = scene.getComponent<uf::Asset>();

	this->addHook( "graphics:Assign.%UID%", [&](ext::json::Value& json){
		metadata["system"]["control"] = false;
		std::string filename = json["filename"].as<std::string>();
		std::string category = json["category"].as<std::string>();

		if ( category != "" && category != "images" ) return;
		if ( category == "" && uf::io::extension(filename) != "png" && uf::io::extension(filename) != "jpg" && uf::io::extension(filename) != "jpeg" ) return;

		uf::Scene& scene = this->getRootParent<uf::Scene>();
		uf::Asset& assetLoader = scene.getComponent<uf::Asset>();
		const uf::Image* imagePointer = NULL;
		if ( !assetLoader.has<uf::Image>(filename) ) return;
		imagePointer = &assetLoader.get<uf::Image>(filename);
		if ( !imagePointer ) return;

		uf::Image image = *imagePointer;
		uf::Mesh& mesh = this->getComponent<uf::Mesh>();
		auto& graphic = this->getComponent<uf::Graphic>();
		mesh.vertices = {
			{{-1*-0.5f, 0.0f, 0.0f}, {1.0f, 0.0f}, { 0.0f, 0.0f, -1.0f } },
			{{-1*0.5f, 0.0f, 0.0f}, {0.0f, 0.0f}, { 0.0f, 0.0f, -1.0f } },
			{{-1*0.5f, 1.0f, 0.0f}, {0.0f, 1.0f}, { 0.0f, 0.0f, -1.0f } },
			{{-1*0.5f, 1.0f, 0.0f}, {0.0f, 1.0f}, { 0.0f, 0.0f, -1.0f } },
			{{-1*-0.5f, 1.0f, 0.0f}, {1.0f, 1.0f}, { 0.0f, 0.0f, -1.0f } },
			{{-1*-0.5f, 0.0f, 0.0f}, {1.0f, 0.0f}, { 0.0f, 0.0f, -1.0f } },

			{{-1*0.5f, 1.0f, 0.0f}, {0.0f, 1.0f}, { 0.0f, 0.0f, 1.0f } },
			{{-1*0.5f, 0.0f, 0.0f}, {0.0f, 0.0f}, { 0.0f, 0.0f, 1.0f } },
			{{-1*-0.5f, 0.0f, 0.0f}, {1.0f, 0.0f}, { 0.0f, 0.0f, 1.0f } },
			{{-1*-0.5f, 0.0f, 0.0f}, {1.0f, 0.0f}, { 0.0f, 0.0f, 1.0f } },
			{{-1*-0.5f, 1.0f, 0.0f}, {1.0f, 1.0f}, { 0.0f, 0.0f, 1.0f } },
			{{-1*0.5f, 1.0f, 0.0f}, {0.0f, 1.0f}, { 0.0f, 0.0f, 1.0f } },
		};

		graphic.initialize();
		graphic.initializeMesh( mesh );

		auto& texture = graphic.material.textures.emplace_back();
		texture.loadFromImage( image );

		graphic.material.attachShader(uf::io::root+"/shaders/base/vert.spv", uf::renderer::enums::Shader::VERTEX);
		graphic.material.attachShader(uf::io::root+"/shaders/base/frag.spv", uf::renderer::enums::Shader::FRAGMENT);

		metadata["system"]["control"] = true;
		metadata["system"]["loaded"] = true;
	});
	this->addHook( "asset:Load.%UID%", [&](ext::json::Value& json){
		this->queueHook("graphics:Assign.%UID%", json, 0.0f);
	});

	{
		std::string id = metadata[""]["party"][0].as<std::string>();
		uf::Serializer member = metadata[""]["transients"][id];
		uf::Serializer cardData = masterDataGet("Card", id);
		std::string name = cardData["filename"].as<std::string>();
		if ( member["skin"].is<std::string>() ) name += "_" + member["skin"].as<std::string>();
		std::string filename = "https://cdn..xyz//unity/Android/fg/fg_" + name + ".png";
		if ( cardData["internal"].as<bool>() ) {
			filename = uf::io::root+"/smtsamo/fg/fg_" + name + ".png";
		}
		
		metadata["orientation"] = cardData["orientation"].as<std::string>() == "" ? "portrait" : "landscape";
		
		assetLoader.load(filename, "asset:Load." + std::to_string(this->getUid()) );
	}
}
void ext::HousamoSpriteBehavior::tick( uf::Object& self ) {}
void ext::HousamoSpriteBehavior::render( uf::Object& self ){}
void ext::HousamoSpriteBehavior::destroy( uf::Object& self ){}
#undef this
#endif