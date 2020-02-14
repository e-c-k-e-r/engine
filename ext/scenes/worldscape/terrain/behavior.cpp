#include "behavior.h"
#include "terrain.h"
#include "generator.h"
#include "region.h"
#include "maze.h"

#include <uf/utils/math/transform.h>
#include <uf/utils/math/physics.h>

#include <uf/engine/object/object.h>
#include <uf/engine/asset/asset.h>

#include <uf/utils/window/window.h>
#include <uf/utils/graphic/mesh.h>
#include <uf/utils/graphic/graphic.h>
#include <uf/utils/camera/camera.h>
#include <uf/utils/math/collision.h>
#include <uf/utils/thread/thread.h>
#include <uf/utils/renderer/renderer.h>
#include <uf/utils/string/hash.h>

#include <sys/stat.h>

UF_BEHAVIOR_ENTITY_CPP_BEGIN(ext::Terrain)
#define this ((ext::Terrain*) &self)
void ext::TerrainBehavior::initialize( uf::Object& self ) {
	// alias Mesh types
	uf::Serializer& metadata = this->getComponent<uf::Serializer>();

	std::size_t seed = 0;
	if ( metadata["terrain"]["seed"].is<size_t>() ) {
		seed = metadata["terrain"]["seed"].as<size_t>();
	} else if ( metadata["terrain"]["seed"].is<std::string>() ) {
		seed = std::hash<std::string>{}( metadata["terrain"]["seed"].as<std::string>() );
	}
	{
		std::cout << "Seed: " << seed << std::endl;
		ext::TerrainGenerator::noise.seed( seed );
	}

	/* setup folder */ {
		uf::Serializer modifiers;
		modifiers["terrain"]["seed"] = metadata["terrain"]["seed"];
		modifiers["region"]["size"] = metadata["region"]["size"];
		modifiers["region"]["subdivisions"] = metadata["region"]["subdivisions"];
		modifiers["region"]["modulus"] = metadata["region"]["modulus"];
		std::string input = modifiers;
		metadata["system"]["hash"] = uf::string::sha256( input );
	#if !UF_ENV_DREAMCAST
		try {
			std::string save = "./data/save/" + metadata["system"]["hash"].as<std::string>() + "/";
			int status = mkdir(save.c_str());
		} catch ( ... ) {

		}
		try {
			std::string save = "./data/save/" + metadata["system"]["hash"].as<std::string>() + "/regions/";
			int status = mkdir(save.c_str());
		} catch ( ... ) {

		}
	#endif
	}

	// setup maze
	{
		ext::Maze& maze = this->getComponent<ext::Maze>();
		maze.initialize(8, 8, 1, 0.5, 0.5);
		maze.seed = seed;
		maze.build();
		maze.print();
	}

	metadata["system"]["state"] = "preinit";
	metadata["system"]["modified"] = true;

	this->addHook( "system:TickRate.Restore", [&](ext::json::Value& json){
		std::cout << "Returning limiter from " << uf::thread::limiter << " to " << metadata["system"]["limiter"].as<float>() << std::endl;
		uf::thread::limiter = metadata["system"]["limiter"].as<float>();
	});
	this->addHook( "terrain:GenerateMesh.%UID%", [&](ext::json::Value& json){

		auto& graphic = this->getComponent<uf::Graphic>();
		auto& mesh = this->getComponent<ext::TerrainGenerator::mesh_t>();

		graphic.initializeGeometry( mesh );
		graphic.getPipeline().update( graphic );
	//	graphic.updatePipelines();
		graphic.process = true;
		uf::renderer::states::rebuild = true;
	});
	this->addHook( "terrain:Post-Initialize.%UID%", [&](ext::json::Value& json){
		if ( metadata["terrain"]["unified"].as<bool>() ) {
			std::string textureFilename = ""; {
				uf::Asset assetLoader;
				for ( uint i = 0; i < metadata["system"]["assets"].size(); ++i ) {
					if ( textureFilename != "" ) break;
					std::string filename = this->grabURI( metadata["system"]["assets"][i].as<std::string>(), metadata["system"]["root"].as<std::string>() );
					textureFilename = assetLoader.cache( filename );
				}
			}
			auto& graphic = this->getComponent<uf::Graphic>();

			graphic.initialize();
			graphic.process = false;

			auto& texture = graphic.material.textures.emplace_back();
			texture.sampler.descriptor.filter.min = uf::renderer::enums::Filter::NEAREST;
			texture.sampler.descriptor.filter.mag = uf::renderer::enums::Filter::NEAREST;
			texture.loadFromFile( textureFilename );

			std::string suffix = ""; {
				std::string _ = this->getRootParent<uf::Scene>().getComponent<uf::Serializer>()["shaders"]["region"]["suffix"].as<std::string>();
				if ( _ != "" ) suffix = _ + ".";
			}
			graphic.material.initializeShaders({
				{"./data/shaders/terrain.vert.spv", uf::renderer::enums::Shader::VERTEX},
				{"./data/shaders/terrain.frag.spv", uf::renderer::enums::Shader::FRAGMENT}
			});
			// uf::renderer::rebuildOnTickStart = false;
		}

		this->generate();
	/*
		auto& parent = this->getParent().as<uf::Object>();
		auto& metadata = parent.getComponent<uf::Serializer>();
		parent.queueHook("system:Load.Finished.%UID%");
	*/
	});
}
void ext::TerrainBehavior::tick( uf::Object& self ) {
	auto& scene = uf::scene::getCurrentScene();
	uf::Object& controller = scene.getController<uf::Object>();
	uf::Serializer& metadata = this->getComponent<uf::Serializer>();
	pod::Thread& mainThread = uf::thread::has("Main") ? uf::thread::get("Main") : uf::thread::create( "Main", false, true );
	// lambda to transition from a resolving state
	auto transitionState = [&]( ext::Terrain& that, const std::string& next, bool unresolved ){
		uf::Serializer& metadata = that.getComponent<uf::Serializer>();
	//	uf::thread::add( mainThread, [&]() -> int {
		//	std::cout << "Transitioning: " << metadata["system"]["state"].as<std::string>() << " -> ";
			metadata["system"]["state"] = unresolved ? next : "open";
			metadata["system"]["modified"] = unresolved;
		//	std::cout << metadata["system"]["state"].as<std::string>() << std::endl;
	//	return 0;}, true );
	};
	// lambda to transition into a resolving state
	auto transitionResolvingState = [&]( ext::Terrain& that ){
		uf::Serializer& metadata = that.getComponent<uf::Serializer>();
	//	std::cout << "Resolving: " << metadata["system"]["state"].as<std::string>() << " -> ";
		metadata["system"]["state"] = "resolving:" + metadata["system"]["state"].as<std::string>();
	//	std::cout << metadata["system"]["state"].as<std::string>() << std::endl;
	};
	// lambda for sorting regions, nearest to farthest, from the controller
	auto sortRegions = [&]( uf::Object& controller, std::vector<uf::Object*>& regions ){
		const pod::Vector3& position = controller.getComponent<pod::Transform<>>().position;
		std::sort( regions.begin(), regions.end(), [&]( const uf::Object* l, const uf::Object* r ){
			if ( !l ) return false; if ( !r ) return true;
			if ( !l->hasComponent<pod::Transform<>>() ) return false; if ( !r->hasComponent<pod::Transform<>>() ) return true;
			return uf::vector::magnitude( uf::vector::subtract( l->getComponent<pod::Transform<>>().position, position ) ) < uf::vector::magnitude( uf::vector::subtract( r->getComponent<pod::Transform<>>().position, position ) );
		} );
	};
	// generate initial terrain
	if ( metadata["system"]["state"] == "preinit" ) {
		uf::Entity* controller = scene.findByName("Player");
		if ( controller ) {
			transitionState(*this, "initialize", true);
			this->generate(true);
			this->relocateChildren();
		//	this->queueHook("terrain:Post-Initialize.%UID%", "", 1.0f);
			this->callHook("terrain:Post-Initialize.%UID%");
		}
	}
	// process only if there's child regions
	if ( this->getChildren().empty() ) return;

	// multipurpose timer
	static uf::Timer<long long> timer(false);
	// open gamestate, look for work
	if ( metadata["system"]["state"] == "open" ) { transitionResolvingState(*this);
		// cleanup orphans
		{

		}
		// remove regions too far
		{
			std::vector<pod::Vector3i> locations;
			for ( uf::Entity* region : this->getChildren() ) { if ( !region || region->getName() != "Region" ) continue;
				const pod::Transform<>& transform = region->getComponent<pod::Transform<>>();
				pod::Vector3i location = { 
					(int) transform.position.x / metadata["region"]["size"][0].as<int>(),
					(int) transform.position.y / metadata["region"]["size"][1].as<int>(),
					(int) transform.position.z / metadata["region"]["size"][2].as<int>()
				};
				// location too far
				bool degenerate = !this->inBounds( location );
				// keep if an entity requests for it
				for ( uf::Entity* entity : region->getChildren() ) { if ( !entity ) continue;
					uf::Serializer& metadata = entity->getComponent<uf::Serializer>();
					if ( ext::json::isObject( metadata["region"] ) && metadata["region"]["persist"].as<bool>() ) {
						degenerate = false;
						break;
					}
				}
				if ( degenerate )
					locations.push_back(location);
			}
			// load new regions, remove old regions
			if ( !locations.empty() ) {
				this->generate();
				for ( pod::Vector3i& location : locations )
					this->degenerate(location);
			}
			// move to next state:
			transitionState(*this, "initialize", !locations.empty());
		}
	}
	// initialize new regions
	if ( metadata["system"]["state"] == "initialize" ) { transitionResolvingState(*this);
		uf::thread::add( uf::thread::fetchWorker(), [&]() -> int {
			std::vector<uf::Object*> regions;
			// retrieve changed regions
			for ( uf::Entity* region : this->getChildren() ) { if ( !region ) continue;
				uf::Serializer& metadata = region->getComponent<uf::Serializer>();
			//	if ( ext::json::isObject( metadata["region"]["location"] ) )
					regions.push_back((uf::Object*) region);
			}
			// sort by closest to farthest
			sortRegions(controller, regions);
			if ( metadata["terrain"]["unified"].as<bool>() ) {
				auto& graphic = this->getComponent<uf::Graphic>();
				auto& mesh = this->getComponent<ext::TerrainGenerator::mesh_t>();
				mesh.destroy();
				for ( uf::Object* region : regions ) {
					uf::Serializer& metadata = region->getComponent<uf::Serializer>();
					if ( !metadata["region"]["initialized"].as<bool>() ) region->initialize();
					if ( !metadata["region"]["generated"].as<bool>() ) region->callHook("region:Generate.%UID%");
					if ( !metadata["region"]["rasterized"].as<bool>() ) {
						region->queueHook("region:Finalize.%UID%");
						region->queueHook("region:Populate.%UID%");
					}
					auto& generator = region->getComponent<ext::TerrainGenerator>();
					generator.rasterize(mesh.vertices, *region, false);
				}
				this->queueHook("terrain:GenerateMesh.%UID%");
			} else {
			// initialize uninitialized regions
				for ( uf::Object* region : regions ) {
				//	uf::thread::add( uf::thread::fetchWorker(), [&]() -> int {
					uf::Serializer& metadata = region->getComponent<uf::Serializer>();
					if ( !metadata["region"]["initialized"].as<bool>() ) region->initialize();
					if ( !metadata["region"]["generated"].as<bool>() ) region->callHook("region:Generate.%UID%");
					if ( !metadata["region"]["rasterized"].as<bool>() ) region->callHook("region:Rasterize.%UID%");
				//	return 0;}, true );
				}
			}

			// move to next state:
			transitionState(*this, "open", !regions.empty());
		return 0;}, true );
	}
	// check if we need to relocate entities
	this->relocateChildren();
}
void ext::TerrainBehavior::render( uf::Object& self ){
	uf::Scene& scene = uf::scene::getCurrentScene();
	uf::Serializer& metadata = this->getComponent<uf::Serializer>();
	
	if ( !metadata["terrain"]["unified"].as<bool>() ) return;
	/* Update uniforms */ if ( this->hasComponent<uf::Graphic>() ) {
		auto& scene = uf::scene::getCurrentScene();
		auto& graphic = this->getComponent<uf::Graphic>();
		auto& camera = scene.getController().getComponent<uf::Camera>();		
		if ( !graphic.initialized ) return;
		auto& uniforms = graphic.material.shaders.front().uniforms.front().get<uf::MeshDescriptor<>>();
		uniforms.matrices.model = uf::matrix::identity();
		for ( std::size_t i = 0; i < uf::renderer::settings::maxViews; ++i ) {
			uniforms.matrices.view[i] = camera.getView( i );
			uniforms.matrices.projection[i] = camera.getProjection( i );
		}
		graphic.material.shaders.front().updateBuffer( uniforms, 0, true );
	}
}
void ext::TerrainBehavior::destroy( uf::Object& self ){}
#undef this
UF_BEHAVIOR_ENTITY_CPP_END(Terrain)