#include "behavior.h"
#include "terrain.h"
#include "generator.h"
#include "region.h"
#include "maze.h"

#include <uf/utils/math/transform.h>
#include <uf/utils/math/physics.h>

#include <uf/engine/object/object.h>

#include <uf/utils/window/window.h>
#include <uf/utils/graphic/mesh.h>
#include <uf/utils/graphic/graphic.h>
#include <uf/utils/camera/camera.h>
#include <uf/utils/math/collision.h>
#include <uf/utils/thread/thread.h>
#include <uf/utils/renderer/renderer.h>
#include <uf/utils/string/hash.h>

#include <sys/stat.h>

EXT_BEHAVIOR_ENTITY_CPP_BEGIN(Terrain)
#define this ((ext::Terrain*) &self)
void ext::TerrainBehavior::initialize( uf::Object& self ) {
	// alias Mesh types
	uf::Serializer& metadata = this->getComponent<uf::Serializer>();

	std::size_t seed;
	if ( metadata["terrain"]["seed"].isUInt64() ) {
		seed = metadata["terrain"]["seed"].asUInt64();
	} else if ( metadata["terrain"]["seed"].isString() ) {
		seed = std::hash<std::string>{}( metadata["terrain"]["seed"].asString() );
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

		try {
			std::string save = "./data/save/" + metadata["system"]["hash"].asString() + "/";
			int status = mkdir(save.c_str());
		} catch ( ... ) {

		}
		try {
			std::string save = "./data/save/" + metadata["system"]["hash"].asString() + "/regions/";
			int status = mkdir(save.c_str());
		} catch ( ... ) {

		}
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

	this->addHook( "system:TickRate.Restore", [&](const std::string& event)->std::string{	
		std::cout << "Returning limiter from " << uf::thread::limiter << " to " << metadata["system"]["limiter"].asFloat() << std::endl;
		uf::thread::limiter = metadata["system"]["limiter"].asFloat();
		return "true";
	});
	this->addHook( "terrain:Post-Initialize.%UID%", [&](const std::string& event)->std::string{	
		this->generate();
	/*
		auto& parent = this->getParent().as<uf::Object>();
		auto& metadata = parent.getComponent<uf::Serializer>();
		parent.queueHook("system:Load.Finished.%UID%");
	*/
		return "true";
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
		//	std::cout << "Transitioning: " << metadata["system"]["state"].asString() << " -> ";
			metadata["system"]["state"] = unresolved ? next : "open";
			metadata["system"]["modified"] = unresolved;
		//	std::cout << metadata["system"]["state"].asString() << std::endl;
	//	return 0;}, true );
	};
	// lambda to transition into a resolving state
	auto transitionResolvingState = [&]( ext::Terrain& that ){
		uf::Serializer& metadata = that.getComponent<uf::Serializer>();
	//	std::cout << "Resolving: " << metadata["system"]["state"].asString() << " -> ";
		metadata["system"]["state"] = "resolving:" + metadata["system"]["state"].asString();
	//	std::cout << metadata["system"]["state"].asString() << std::endl;
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
					(int) transform.position.x / metadata["region"]["size"][0].asInt(),
					(int) transform.position.y / metadata["region"]["size"][1].asInt(),
					(int) transform.position.z / metadata["region"]["size"][2].asInt()
				};
				// location too far
				bool degenerate = !this->inBounds( location );
				// keep if an entity requests for it
				for ( uf::Entity* entity : region->getChildren() ) { if ( !entity ) continue;
					uf::Serializer& metadata = entity->getComponent<uf::Serializer>();
					if ( metadata["region"].isObject() && metadata["region"]["persist"].asBool() ) {
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
			//	if ( metadata["region"]["location"].isObject() )
					regions.push_back((uf::Object*) region);
			}
			// sort by closest to farthest
			sortRegions(controller, regions);
			// initialize uninitialized regions
			for ( uf::Object* region : regions ) {
			//	uf::thread::add( uf::thread::fetchWorker(), [&]() -> int {
				uf::Serializer& metadata = region->getComponent<uf::Serializer>();
				if ( !metadata["region"]["initialized"].asBool() ) region->initialize();
				if ( !metadata["region"]["generated"].asBool() ) region->callHook("region:Generate.%UID%", "");
				if ( !metadata["region"]["rasterized"].asBool() ) region->callHook("region:Rasterize.%UID%", "");
			//	return 0;}, true );
			}
			// move to next state:
			transitionState(*this, "open", !regions.empty());
		return 0;}, true );
	}
	// check if we need to relocate entities
	this->relocateChildren();
}
void ext::TerrainBehavior::render( uf::Object& self ){}
void ext::TerrainBehavior::destroy( uf::Object& self ){}
#undef this
EXT_BEHAVIOR_ENTITY_CPP_END(Terrain)