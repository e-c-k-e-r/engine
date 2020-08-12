#include "sprite.h"

#include <uf/utils/hook/hook.h>
#include <uf/utils/time/time.h>
#include <uf/utils/serialize/serializer.h>
#include <uf/utils/userdata/userdata.h>
#include <uf/utils/mesh/mesh.h>
#include <uf/utils/window/window.h>
#include <uf/utils/camera/camera.h>
#include <uf/ext/vulkan/graphics/base.h>
#include <uf/ext/vulkan/device.h>
#include <uf/ext/vulkan/swapchain.h>
#include <uf/ext/vulkan/vulkan.h>
#include <uf/utils/image/image.h>

#include "../world.h"
#include <uf/engine/asset/asset.h>
#include "battle.h"
#include ".h"

namespace {
	ext::World* world;

	uf::Serializer masterTableGet( const std::string& table ) {
		if ( !world ) world = (ext::World*) uf::Entity::globalFindByName("World");
		if ( !world ) return std::string();
		uf::Serializer& mastertable = world->getComponent<uf::Serializer>();
		return mastertable["system"]["mastertable"][table];
	}
	uf::Serializer masterDataGet( const std::string& table, const std::string& key ) {
		if ( !world ) world = (ext::World*) uf::Entity::globalFindByName("World");
		if ( !world ) return std::string();
		uf::Serializer& mastertable = world->getComponent<uf::Serializer>();
		return mastertable["system"]["mastertable"][table][key];
	}
	inline int64_t parseInt( const std::string& str ) {
		return atoi(str.c_str());
	}
}

EXT_OBJECT_REGISTER_CPP(HousamoSprite)

void ext::HousamoSprite::initialize() {	
	ext::Craeture::initialize();

	// alias Mesh types
	{
	//	this->addAlias<uf::Mesh, uf::MeshBase>();
	}

	ext::World& world = this->getRootParent<ext::World>();
	uf::Serializer& metadata = this->getComponent<uf::Serializer>();
	uf::Serializer& masterdata = world.getComponent<uf::Serializer>();
	uf::Asset& assetLoader = world.getComponent<uf::Asset>();

	this->addHook( "graphics:Assign.%UID%", [&](const std::string& event)->std::string{	
		uf::Serializer json = event;
		std::string filename = json["filename"].asString();
		metadata["system"]["control"] = false;

		if ( uf::string::extension(filename) != "png" ) return "false";

		uf::Scene& scene = this->getRootParent<uf::Scene>();
		uf::Asset& assetLoader = scene.getComponent<uf::Asset>();
		const uf::Image* imagePointer = NULL;
		try { imagePointer = &assetLoader.get<uf::Image>(filename); } catch ( ... ) {}
		if ( !imagePointer ) return "false";

		uf::Image image = *imagePointer;
		uf::Mesh& mesh = this->getComponent<uf::Mesh>();
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
		mesh.initialize(true);
		mesh.graphic.texture.loadFromImage( image );
		mesh.graphic.bindUniform<uf::StereoMeshDescriptor>();
		mesh.graphic.initializeShaders({
			{"./data/shaders/base.stereo.vert.spv", VK_SHADER_STAGE_VERTEX_BIT},
			{"./data/shaders/base.frag.spv", VK_SHADER_STAGE_FRAGMENT_BIT}
		});
		mesh.graphic.initialize();
		mesh.graphic.autoAssign();

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

void ext::HousamoSprite::tick() {
	ext::Craeture::tick();
}

void ext::HousamoSprite::destroy() {	
	if ( this->hasComponent<uf::Mesh>() ) {
		auto& mesh = this->getComponent<uf::Mesh>();
		mesh.graphic.destroy();
		mesh.destroy();
	}
	ext::Craeture::destroy();
}

void ext::HousamoSprite::render() {	
	uf::Serializer& metadata = this->getComponent<uf::Serializer>();
	if ( !metadata["system"]["loaded"].asBool() ) return;

	ext::Craeture::render();
	/* Update uniforms */ if ( this->hasComponent<uf::Mesh>() ) {
		auto& mesh = this->getComponent<uf::Mesh>();
		auto& scene = uf::scene::getCurrentScene();
		auto& controller = *scene.getController();
		auto& camera = controller.getComponent<uf::Camera>();
		auto& transform = this->getComponent<pod::Transform<>>();
		if ( !mesh.generated ) return;
		//auto& uniforms = mesh.graphic.uniforms<uf::StereoMeshDescriptor>();
		auto& uniforms = mesh.graphic.uniforms<uf::StereoMeshDescriptor>();
		uniforms.matrices.model = uf::transform::model( transform );
		for ( std::size_t i = 0; i < 2; ++i ) {
			uniforms.matrices.view[i] = camera.getView( i );
			uniforms.matrices.projection[i] = camera.getProjection( i );
		}
		uniforms.color[0] = metadata["color"][0].asFloat();
		uniforms.color[1] = metadata["color"][1].asFloat();
		uniforms.color[2] = metadata["color"][2].asFloat();
		uniforms.color[3] = metadata["color"][3].asFloat();
		mesh.graphic.updateBuffer( uniforms, 0, false );
	};
}