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

EXT_BEHAVIOR_REGISTER_CPP(HousamoSpriteBehavior)
#define this (&self)
void ext::HousamoSpriteBehavior::initialize( uf::Object& self ) {
	uf::Scene& scene = uf::scene::getCurrentScene();
	uf::Serializer& metadata = this->getComponent<uf::Serializer>();
	uf::Serializer& masterdata = scene.getComponent<uf::Serializer>();
	uf::Asset& assetLoader = scene.getComponent<uf::Asset>();

	this->addHook( "graphics:Assign.%UID%", [&](const std::string& event)->std::string{	
		uf::Serializer json = event;
		std::string filename = json["filename"].asString();
		metadata["system"]["control"] = false;

		if ( uf::io::extension(filename) != "png" ) return "false";

		uf::Scene& scene = this->getRootParent<uf::Scene>();
		uf::Asset& assetLoader = scene.getComponent<uf::Asset>();
		const uf::Image* imagePointer = NULL;
		try { imagePointer = &assetLoader.get<uf::Image>(filename); } catch ( ... ) {}
		if ( !imagePointer ) return "false";

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
		graphic.initializeGeometry( mesh );

		auto& texture = graphic.material.textures.emplace_back();
		texture.loadFromImage( image );

		graphic.material.attachShader("./data/shaders/base.stereo.vert.spv", VK_SHADER_STAGE_VERTEX_BIT);
		graphic.material.attachShader("./data/shaders/base.frag.spv", VK_SHADER_STAGE_FRAGMENT_BIT);

		metadata["system"]["control"] = true;
		metadata["system"]["loaded"] = true;
		return "true";
	});
	this->addHook( "asset:Load.%UID%", [&](const std::string& event)->std::string{
		this->queueHook("graphics:Assign.%UID%", event, 0.0f);
		return "true";
	});

	{
		std::string id = metadata[""]["party"][0].asString();
		uf::Serializer member = metadata[""]["transients"][id];
		uf::Serializer cardData = masterDataGet("Card", id);
		std::string name = cardData["filename"].asString();
		if ( member["skin"].isString() ) name += "_" + member["skin"].asString();
		std::string filename = "https://cdn..xyz//unity/Android/fg/fg_" + name + ".png";
		if ( cardData["internal"].asBool() ) {
			filename = "./data/smtsamo/fg/fg_" + name + ".png";
		}
		
		metadata["orientation"] = cardData["orientation"].asString() == "" ? "portrait" : "landscape";
		
		assetLoader.load(filename, "asset:Load." + std::to_string(this->getUid()) );
	}
}
void ext::HousamoSpriteBehavior::tick( uf::Object& self ) {}
void ext::HousamoSpriteBehavior::render( uf::Object& self ){
	uf::Serializer& metadata = this->getComponent<uf::Serializer>();
	if ( !metadata["system"]["loaded"].asBool() ) return;

	/* Update uniforms */ if ( this->hasComponent<uf::Graphic>() ) {
		auto& mesh = this->getComponent<uf::Mesh>();
		auto& scene = uf::scene::getCurrentScene();
		auto& graphic = this->getComponent<uf::Graphic>();
		auto& transform = this->getComponent<pod::Transform<>>();
		auto& camera = scene.getController().getComponent<uf::Camera>();		
		if ( !graphic.initialized ) return;
		auto& uniforms = graphic.material.shaders.front().uniforms.front().get<uf::StereoMeshDescriptor>();
		uniforms.matrices.model = uf::transform::model( transform );
		for ( std::size_t i = 0; i < 2; ++i ) {
			uniforms.matrices.view[i] = camera.getView( i );
			uniforms.matrices.projection[i] = camera.getProjection( i );
		}
	
		uniforms.color[0] = metadata["color"][0].asFloat();
		uniforms.color[1] = metadata["color"][1].asFloat();
		uniforms.color[2] = metadata["color"][2].asFloat();
		uniforms.color[3] = metadata["color"][3].asFloat();
		graphic.material.shaders.front().updateBuffer( uniforms, 0, false );
	};
}
void ext::HousamoSpriteBehavior::destroy( uf::Object& self ){}
#undef this