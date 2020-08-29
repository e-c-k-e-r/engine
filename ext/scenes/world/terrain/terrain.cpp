#include "terrain.h"

#include "../../../ext.h"
#include <uf/engine/asset/asset.h>
#include "../../world/world.h"
#include "generator.h"
#include "region.h"

#include <uf/utils/window/window.h>
#include <uf/utils/graphic/mesh.h>
#include <uf/utils/graphic/graphic.h>
#include <uf/utils/camera/camera.h>
#include <uf/utils/thread/thread.h>
#include <uf/ext/vulkan/graphic.h>
#include <uf/ext/vulkan/vulkan.h>

#include <uf/utils/string/hash.h>

#include <sys/stat.h>

namespace {
	std::unordered_map<pod::Vector3i, ext::Region*> region_table;
}

EXT_OBJECT_REGISTER_CPP(Terrain)
void ext::Terrain::destroy() {
	uf::Object::destroy();
}
void ext::Terrain::initialize() {
	uf::Object::initialize();

	// alias Mesh types
	{
	//	this->addAlias<ext::TerrainGenerator::mesh_t, uf::MeshBase>();
		this->addAlias<ext::TerrainGenerator::mesh_t, uf::Mesh>();
	}

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

	this->m_name = "Terrain";

	metadata["system"]["state"] = "preinit";
	metadata["system"]["modified"] = true;

	this->addHook( "system:TickRate.Restore", [&](const std::string& event)->std::string{	
		std::cout << "Returning limiter from " << uf::thread::limiter << " to " << metadata["system"]["limiter"].asFloat() << std::endl;
		uf::thread::limiter = metadata["system"]["limiter"].asFloat();
		return "true";
	});
	this->addHook( "terrain:Post-Initialize.%UID%", [&](const std::string& event)->std::string{	
		this->generate();
		return "true";
	});
}

void ext::Terrain::tick() {
	uf::Object::tick();

	uf::Scene& root = this->getRootParent<uf::Scene>();
	uf::Object& controller = root.getController<uf::Object>();
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
		uf::Entity* controller = root.findByName("Player");
		if ( controller ) {
			transitionState(*this, "initialize", true);
			this->generate(true);
			this->relocateChildren();
		//	this->queueHook("terrain:Post-Initialize.%UID%", "", 1.0f);
			this->callHook("terrain:Post-Initialize.%UID%");
		}
	}
	// process only if there's child regions
	if ( this->m_children.empty() ) return;

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
			for ( uf::Entity* region : this->m_children ) { if ( !region || region->getName() != "Region" ) continue;
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
			for ( uf::Entity* region : this->m_children ) { if ( !region ) continue;
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
/*
	// initialize newly generated regions
	if ( metadata["system"]["state"] == "initialize" ) { transitionResolvingState(*this);
		uf::thread::add( uf::thread::fetchWorker(), [&]() -> int {
			std::vector<uf::Object*> regions;
			// retrieve changed regions
			for ( uf::Entity* region : this->m_children ) { if ( !region ) continue;
				uf::Serializer& metadata = region->getComponent<uf::Serializer>();
				if ( !metadata["region"]["initialized"].asBool() ) regions.push_back((uf::Object*) region);
			}
			// sort by closest to farthest
			sortRegions(controller, regions);
			// initialize uninitialized regions
			for ( uf::Object* region : regions ) region->initialize();
			// move to next state:
			transitionState(*this, "generate", !regions.empty());
		return 0;}, true );
	}
	// generate terrain inside region
	if ( metadata["system"]["state"] == "generate" ) { transitionResolvingState(*this);
		uf::thread::add( uf::thread::fetchWorker(), [&]() -> int {
			std::vector<uf::Object*> regions;
			// retrieve changed regions
			for ( uf::Entity* region : this->m_children ) { if ( !region || region->getName() != "Region" ) continue;
				uf::Serializer& metadata = region->getComponent<uf::Serializer>();
				if ( !metadata["region"]["generated"].asBool() ) regions.push_back((uf::Object*) region);
			}
			// sort by closest to farthest
			sortRegions(controller, regions);
			// generate 
			for ( uf::Object* region : regions ) region->callHook("region:Generate.%UID%", "");
			// move to next state:
			transitionState(*this, "rasterize", !regions.empty());
		return 0;}, true );
	}
	if ( metadata["system"]["state"] == "rasterize" ) { transitionResolvingState(*this);
		uf::thread::add( uf::thread::fetchWorker(), [&]() -> int {
			std::vector<uf::Object*> regions;
			// retrieve changed regions
			for ( uf::Entity* region : this->m_children ) { if ( !region || region->getName() != "Region" ) continue;
				uf::Serializer& metadata = region->getComponent<uf::Serializer>();
				if ( !metadata["region"]["rasterized"].asBool() ) regions.push_back((uf::Object*) region);
			}
			// sort by closest to farthest
			sortRegions(controller, regions);
			// generate 
			for ( uf::Object* region : regions ) region->callHook("region:Rasterize.%UID%", "");
			// move to open state
			transitionState(*this, "open", !regions.empty());
		return 0;}, true );
	}
*/
	// check if we need to relocate entities
	this->relocateChildren();
}

void ext::Terrain::render() {
	uf::Object::render();
}

void ext::Terrain::relocateChildren() {
	uf::Scene& root = this->getRootParent<uf::Scene>();
	std::vector<uf::Entity*> entities;
	root.process( [&]( uf::Entity* entity ) {
		if ( !entity || entity->getUid() == 0 ) return;
		uf::Serializer& metadata = entity->getComponent<uf::Serializer>();
		if ( metadata["region"].isObject() && metadata["region"]["track"].asBool() ) {
			if ( std::find( entities.begin(), entities.end(), entity ) == entities.end() ) {
				entities.push_back(entity);
			}
		}
	} );
	
	uf::Serializer& metadata = this->getComponent<uf::Serializer>();
	pod::Vector3ui size = {
		metadata["region"]["size"][0].asUInt(),
		metadata["region"]["size"][1].asUInt(),
		metadata["region"]["size"][2].asUInt(),
	};
	for ( uf::Entity* e : entities ) {
		uf::Entity& entity = *e;
		const pod::Transform<>& transform = entity.getComponent<pod::Transform<>>();
		pod::Vector3 pointf = uf::vector::divide(transform.position, {size.x, size.y, size.z});
		pod::Vector3i point = {
			(int) (pointf.x + (pointf.x > 0 ? 0.5 : -0.5)),
			(int) (pointf.y + (pointf.y > 0 ? 0.5 : -0.5)),
			(int) (pointf.z + (pointf.z > 0 ? 0.5 : -0.5)),
		};
		if ( !this->exists(point) ) continue; // oops
		ext::Region& region = *this->at(point);
		bool should = false;
		if ( entity.getParent().getName() != "Region" ) should = true;
		else {
			ext::Region& current = entity.getParent<ext::Region>();
			const pod::Transform<>& t = current.getComponent<pod::Transform<>>();
			pod::Vector3i location = {
				(int) (t.position.x / size.x),
				(int) (t.position.y / size.y),
				(int) (t.position.z / size.z),
			};
			if ( !uf::vector::equals( point, location ) ) should = true;
		}
		if ( should ) {
			region.moveChild(entity);
		//	uf::iostream << "Relocating " << &entity << ": " << entity.getName() << " to (" << point.x << ", " << point.y << ", " << point.z << ")" << "\n";
		}
	}
}

bool ext::Terrain::exists( const pod::Vector3i& position ) const {
	const uf::Serializer& metadata = this->getComponent<uf::Serializer>();
	pod::Vector3i size = {
		metadata["region"]["size"][0].asInt(),
		metadata["region"]["size"][1].asInt(),
		metadata["region"]["size"][2].asInt(),
	};
	if ( this->m_children.empty() ) return false;
	for ( auto it = this->m_children.begin(); it != this->m_children.end(); ++it ) { uf::Entity* kv = *it;
		const pod::Transform<>& transform = kv->getComponent<pod::Transform<>>();
		pod::Vector3i location = {
			(int) (transform.position.x / size.x),
			(int) (transform.position.y / size.y),
			(int) (transform.position.z / size.z),
		};
		if ( uf::vector::equals( location, position ) ) return true;
	}
	return false;
}

bool ext::Terrain::inBounds( const pod::Vector3i& position ) const {
	const ext::World& parent = this->getRootParent<ext::World>();
	const pod::Transform<>& player = parent.getController()->getComponent<pod::Transform<>>();
	const uf::Serializer& metadata = this->getComponent<uf::Serializer>();
	pod::Vector3ui size = {
		metadata["region"]["size"][0].asUInt(),
		metadata["region"]["size"][1].asUInt(),
		metadata["region"]["size"][2].asUInt(),
	};
	pod::Vector3ui threshold = {
		metadata["terrain"]["radius"][0].asUInt(),
		metadata["terrain"]["radius"][1].asUInt(),
		metadata["terrain"]["radius"][2].asUInt(),
	};
	pod::Vector3i point = {
		(int) (player.position.x / size.x),
		(int) (player.position.y / size.y),
		(int) (player.position.z / size.z),
	};
	pod::Vector3i difference = uf::vector::subtract( point, position );

	return !(abs(difference.x) >= threshold.x || abs(difference.y) >= threshold.y || abs(difference.z) >= threshold.z);
}

void ext::Terrain::generate( bool single ) {
	uf::Serializer& metadata = this->getComponent<uf::Serializer>();
	pod::Vector3i radius = {
		metadata["terrain"]["radius"][0].asInt(),
		metadata["terrain"]["radius"][1].asInt(),
		metadata["terrain"]["radius"][2].asInt(),
	};
	pod::Vector3i size = {
		metadata["region"]["size"][0].asInt(),
		metadata["region"]["size"][1].asInt(),
		metadata["region"]["size"][2].asInt(),
	};

	ext::World& parent = this->getRootParent<ext::World>();
	const pod::Transform<>& player = parent.getController()->getComponent<pod::Transform<>>();
	pod::Vector3i location = {
		(int) player.position.x / size.x,
		(int) player.position.y / size.y,
		(int) player.position.z / size.z,
	};
	if ( single ) 
		this->generate( {location.x, location.y, location.z} );
	else
		for ( int y = 0; y < radius.y; ++y ) {
		for ( int z = 0; z < radius.z; ++z ) {
		for ( int x = 0; x < radius.x; ++x ) {
			this->generate( {location.x + x, location.y + y, location.z + z} );
			this->generate( {location.x - x, location.y - y, location.z - z} );
			this->generate( {location.x + x, location.y + y, location.z - z} );
			this->generate( {location.x - x, location.y + y, location.z + z} );
			this->generate( {location.x - x, location.y + y, location.z - z} );
			this->generate( {location.x + x, location.y - y, location.z + z} );
			this->generate( {location.x + x, location.y - y, location.z - z} );
			this->generate( {location.x - x, location.y - y, location.z + z} );
		}
		}
		}
}
ext::Region* ext::Terrain::at( const pod::Vector3i& position ) const {
	if ( ::region_table.count(position) > 0 ) return ::region_table.at(position);

	const uf::Serializer& metadata = this->getComponent<uf::Serializer>();
	pod::Vector3i size = {
		metadata["region"]["size"][0].asInt(),
		metadata["region"]["size"][1].asInt(),
		metadata["region"]["size"][2].asInt(),
	};
	if ( this->m_children.empty() ) return NULL;
	for ( auto it = this->m_children.begin(); it != this->m_children.end(); ++it ) { uf::Entity* kv = *it;
		const pod::Transform<>& transform = kv->getComponent<pod::Transform<>>();
		pod::Vector3i location = {
			(int) (transform.position.x / size.x),
			(int) (transform.position.y / size.y),
			(int) (transform.position.z / size.z),
		};
		if ( uf::vector::equals( location, position ) ) return (ext::Region*) kv;
	}
	return NULL;
}
void ext::Terrain::generate( const pod::Vector3i& position ) {
	uf::Serializer& metadata = this->getComponent<uf::Serializer>();
	pod::Vector3i size = {
		metadata["region"]["size"][0].asInt(),
		metadata["region"]["size"][1].asInt(),
		metadata["region"]["size"][2].asInt(),
	};

	if ( this->exists(position) ) return;
	if ( !this->inBounds(position) ) return;

	uf::Entity* base = new ext::Region; this->addChild(*base); {
		ext::Region& region = *((ext::Region*)base);
		uf::Serializer& m = region.getComponent<uf::Serializer>(); // = metadata;
		m["system"]["root"] = metadata["system"]["root"];
		m["system"]["source"] = metadata["system"]["source"];
		m["region"] = metadata["region"];
		m["terrain"] = metadata["terrain"];

		pod::Transform<>& transform = region.getComponent<pod::Transform<>>();
		transform.position.x = position.x * size.x;
		transform.position.y = position.y * size.y;
		transform.position.z = position.z * size.z;

		m["region"]["location"][0] = position.x;
		m["region"]["location"][1] = position.y;
		m["region"]["location"][2] = position.z;
		
		::region_table[position] = &region;
	}
}
void ext::Terrain::degenerate( const pod::Vector3i& position ) {
	if ( !this->exists( position ) ) return;
	ext::Region* region = this->at( position );
	if ( !region ) return;
	this->removeChild( *region );
	region->destroy();
	delete region;
	::region_table.erase( position );
}