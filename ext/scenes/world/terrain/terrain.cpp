#include "terrain.h"

#include "../../../ext.h"
#include <uf/engine/asset/asset.h>
#include "../../world/world.h"
#include "generator.h"
#include "region.h"

#include <uf/utils/window/window.h>
#include <uf/utils/mesh/mesh.h>
#include <uf/utils/camera/camera.h>
#include <uf/utils/thread/thread.h>
#include <uf/ext/vulkan/graphics/mesh.h>
#include <uf/ext/vulkan/vulkan.h>

#include <uf/utils/string/hash.h>

#include <sys/stat.h>

namespace {
	std::function<uf::Entity*(uf::Entity*, const std::string&)> findByName = []( uf::Entity* parent, const std::string& name ) {
		for ( uf::Entity* entity : parent->getChildren() ) {
			if ( entity->getName() == name ) return entity;
			uf::Entity* p = findByName(entity, name);
			if ( p ) return p;
		}
		return (uf::Entity*) NULL;
	};
	std::function<uf::Entity*(uf::Entity*, uint)> findByUid = []( uf::Entity* parent, uint id ) {
		for ( uf::Entity* entity : parent->getChildren() ) {
			if ( entity->getUid() == id ) return entity;
			uf::Entity* p = findByUid(entity, id);
			if ( p ) return p;
		}
		return (uf::Entity*) NULL;
	};

	std::unordered_map<pod::Vector3i, ext::Region*> region_table;
}

EXT_OBJECT_REGISTER_CPP(Terrain)

void ext::Terrain::destroy() {
	if ( this->hasComponent<ext::vulkan::RTGraphic>() ) {
		this->getComponent<ext::vulkan::RTGraphic>().destroy();
	}
	if ( this->hasComponent<ext::TerrainGenerator::mesh_t>() ) {
		auto& mesh = this->getComponent<ext::TerrainGenerator::mesh_t>();
		mesh.graphic.destroy();
		mesh.destroy();
	}
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


	if ( metadata["terrain"]["raytrace"].asBool() ) {
		this->addComponent<ext::vulkan::RTGraphic>();
	} else {
		this->addComponent<ext::TerrainGenerator::mesh_t>();
	}

	/* setup folder */ {
		uf::Serializer modifiers;
		modifiers["terrain"]["seed"] = metadata["terrain"]["seed"];
		modifiers["region"]["size"] = metadata["region"]["size"];
		modifiers["region"]["modulus"] = metadata["region"]["modulus"];
		std::string input = modifiers;
		metadata["_config"]["hash"] = uf::string::sha256( input );

		try {
			std::string save = "./data/save/" + metadata["_config"]["hash"].asString() + "/";
			int status = mkdir(save.c_str());
		} catch ( ... ) {

		}
		try {
			std::string save = "./data/save/" + metadata["_config"]["hash"].asString() + "/regions/";
			int status = mkdir(save.c_str());
		} catch ( ... ) {

		}
	}

	// setup maze
	{
		ext::Maze& maze = this->getComponent<ext::Maze>();
		maze.initialize(16, 16, 1, 0.5, 0.5);
		maze.seed = seed;
		maze.build();
		maze.print();
	}
	
	if ( this->hasComponent<ext::vulkan::RTGraphic>() ) {
		const uint32_t SSBO_CUBES_MAX = ( 1024 * 1024 * 2 ) / sizeof(ext::vulkan::ComputeGraphic::Cube); // 2MB of data
		const uint32_t SSBO_LIGHTS_MAX = ( 256 ); // 256 lights
		const uint32_t SSBO_TREES_MAX = ( 1024 * 1024 * 2 ) / sizeof(ext::vulkan::ComputeGraphic::Tree); // 2MB of data
		auto& rt = this->getComponent<ext::vulkan::RTGraphic>();
		std::vector<ext::vulkan::ComputeGraphic::Cube> primitives;
		std::vector<ext::vulkan::ComputeGraphic::Light> lights;
		std::vector<ext::vulkan::ComputeGraphic::Tree> trees;
		
		for ( int i = 0; i < SSBO_CUBES_MAX - primitives.size(); ++i ) {	
			ext::vulkan::ComputeGraphic::Cube primitive;
			primitive.type = pod::Primitive::EMPTY;
			primitive.position = { 0, 0, 0, 0 };
			primitives.push_back(primitive);
		}
		for ( int i = 0; i < SSBO_LIGHTS_MAX - lights.size(); ++i ) {	
			ext::vulkan::ComputeGraphic::Light light;
			light.position = { 0, 0, 0 };
			light.color = { 0, 0, 0 };
			lights.push_back(light);
		}
		for ( int i = 0; i < SSBO_CUBES_MAX - trees.size(); ++i ) {	
			ext::vulkan::ComputeGraphic::Tree tree;
			tree.type = pod::Primitive::EMPTY;
			tree.position = { 0, 0, 0, 0 };
			trees.push_back(tree);
		}
		rt.compute.diffuseTexture.loadFromFile( "./data/textures/texture.png" );
		rt.compute.setStorageBuffers( ext::vulkan::device, primitives, lights, trees );
		rt.compute.initializeShaders({
			{"./data/shaders/raytracing.comp.spv", VK_SHADER_STAGE_COMPUTE_BIT}
		});
		rt.autoAssign();
	}

	this->m_name = "Terrain";

	//metadata["terrain"]["state"] = "modified:1";
	metadata["terrain"]["state"] = "preinit";

	this->addHook( "terrain:Initialized.%UID%", [&](const std::string& event)->std::string{	
		metadata["terrain"]["state"] = "modified:1";
		std::cout << "Initialized" << std::endl;
		return "true";
	});
	this->addHook( "system:TickRate.Restore", [&](const std::string& event)->std::string{	
		std::cout << "Returning limiter from " << uf::thread::limiter << " to " << metadata["_system"]["limiter"].asFloat() << std::endl;
		uf::thread::limiter = metadata["_system"]["limiter"].asFloat();
		return "true";
	});
	this->addHook( "terrain:Reload.%UID%", [&](const std::string& event)->std::string{	
		uf::Serializer json = event;

		if ( !this->hasComponent<ext::TerrainGenerator::mesh_t>() ) return "false";

		ext::TerrainGenerator::mesh_t& newMesh = this->getComponent<ext::TerrainGenerator::mesh_t>();
		newMesh.destroy(true);
		newMesh.graphic.destroy();
		for ( uf::Entity* kv : this->m_children ) { if ( !kv ) continue; if ( kv->getName() != "Region" ) continue;
			uf::Serializer& rMetadata = kv->getComponent<uf::Serializer>();
			ext::TerrainGenerator::mesh_t& mesh = kv->getComponent<ext::TerrainGenerator::mesh_t>();
			newMesh.vertices.insert( newMesh.vertices.end(), mesh.vertices.begin(), mesh.vertices.end() );
			mesh.destroy();
			rMetadata["region"]["unified"] = true;
		}
		newMesh.initialize(true);
		newMesh.graphic.bindUniform<uf::StereoMeshDescriptor>();

		std::string suffix = ""; {
			std::string _ = this->getRootParent<ext::World>().getComponent<uf::Serializer>()["shaders"]["region"]["suffix"].asString();
			if ( _ != "" ) suffix = _ + ".";
		}
		newMesh.graphic.initializeShaders({
			{"./data/shaders/terrain.stereo.vert.spv", VK_SHADER_STAGE_VERTEX_BIT},
			{"./data/shaders/terrain."+suffix+"frag.spv", VK_SHADER_STAGE_FRAGMENT_BIT}
		});
		std::string texture = ""; {
			uf::Asset assetLoader;
			for ( uint i = 0; i < metadata["_config"]["assets"].size(); ++i ) {
				if ( texture != "" ) break;
				std::string filename = metadata["_config"]["assets"][i].asString();
				texture = assetLoader.cache( filename );
			}
		}
		newMesh.graphic.texture.loadFromFile( texture );
		newMesh.graphic.initialize();
		newMesh.graphic.autoAssign();

		return "true";
	});
}
#define UF_EASY_TICK 1
#if !UF_EASY_TICK
void ext::Terrain::tick() {
	uf::Object::tick();


	ext::World& parent = this->getRootParent<ext::World>();
	const pod::Transform<>& player = parent.getController()->getComponent<pod::Transform<>>();
	
	uf::Serializer& metadata = this->getComponent<uf::Serializer>();

	/* Debug Information */ if ( true ) {
		/* Terrain Metadata */ {
			static uf::Timer<long long> timer(false);
			if ( !timer.running() ) timer.start();
			if ( uf::Window::isKeyPressed("L") && timer.elapsed().asDouble() >= 1 ) { timer.reset();
				uf::iostream << metadata << "\n";
			}
		}
		/* Terrain Reset */ if ( true ) {
			static uf::Timer<long long> timer(false);
			if ( !timer.running() ) timer.start();
			if ( uf::Window::isKeyPressed("G") && timer.elapsed().asDouble() >= 1 ) { timer.reset();
				uf::iostream << "Regenerating ";
				uint i = 0;
				/* Retrieve changed regions */ for ( uf::Entity* kv : this->m_children ) { if ( !kv ) continue; if ( kv->getName() != "Region" ) continue;
					uf::Serializer& rMetadata = kv->getComponent<uf::Serializer>();
					if ( !rMetadata["region"]["initialized"].asBool() ) continue;
					rMetadata["region"]["modified"] = true;
					rMetadata["region"]["rasterized"] = false;
					++i;
				}
				metadata["terrain"]["modified"] = true;
				uf::iostream << i << " regions" << "\n";
			}
		}
	}
	// Open Gamestate: Handles creating and destroying region entities
	if ( metadata["terrain"]["state"] == "preinit" ) {
		std::function<uf::Entity*(uf::Entity*, int)> recurse = [&]( uf::Entity* parent, int indent ) {
			for ( uf::Entity* entity : parent->getChildren() ) {
				if ( entity->getName() == "Player" ) return entity;
				uf::Entity* p = recurse(entity, indent + 1);
				if ( p ) return p;
			}
			return (uf::Entity*) NULL;
		};
		uf::Entity* pointer = recurse(&parent, 0);
		if ( pointer ) {
			this->generate();
			this->relocatePlayer();
			this->queueHook("terrain:Initialized.%UID%", "", 0.5);
		}
	}
	
	static uf::Timer<long long> generationTimer(false);

	if ( this->m_children.empty() ) return;
	pod::Thread& mainThread = uf::thread::has("Main") ? uf::thread::get("Main") : uf::thread::create( "Main", false, true );
	if ( metadata["terrain"]["state"] == "open" && 
		( (generationTimer.running() && generationTimer.elapsed().asDouble() > 3) || !generationTimer.running() )
	) { metadata["terrain"]["state"] = "resolving:" + metadata["terrain"]["state"].asString();
		if ( !generationTimer.running() ) generationTimer.start();
		else generationTimer.reset();

		/* Create/Destroy Regions */ {
			/* Cleanup orphans */ uf::thread::add( mainThread, [&]() -> int {
				std::vector<uf::Entity*> orphans;
				for ( uf::Entity* e : uf::Entity::entities ) {
					if ( !e ) continue;
					if ( e->hasParent() ) continue;
					if ( e->getName() != "Region" ) continue;
					orphans.push_back(e);
				}
				for ( uf::Entity* e : orphans ) {
					e->destroy<ext::Region>();
				}
			return 0;}, true );

			std::vector<pod::Vector3i> locations;
			for ( uf::Entity* kv : this->m_children ) {
				if ( !kv ) continue;
				if ( kv->getName() != "Region" ) continue;
				const pod::Transform<>& transform = kv->getComponent<pod::Transform<>>();
				pod::Vector3i location = { (int) transform.position.x / metadata["region"]["size"][0].asInt(), (int) transform.position.y / metadata["region"]["size"][1].asInt(), (int) transform.position.z / metadata["region"]["size"][2].asInt() };
				bool degenerate = false;
				if ( !this->inBounds( location ) ) degenerate = true;

				for ( uf::Entity* _kv : kv->getChildren() ) {
					if ( !_kv ) continue;
					uf::Serializer& _metadata = _kv->getComponent<uf::Serializer>();
					if ( _metadata["region"].isObject() && _metadata["region"]["persist"].asBool() ) {
						degenerate = false;
						break;
					}
				}
				{
					uf::Serializer& rMetadata = kv->getComponent<uf::Serializer>();
					if ( !rMetadata["region"]["rasterized"].asBool() ) degenerate = false;
				}

				if ( degenerate ) locations.push_back(location);
			}

			bool resolved = !metadata["terrain"]["modified"].asBool();
			if ( !locations.empty() ) {
				this->generate();
				for ( pod::Vector3i& location : locations ) this->degenerate(location);
				resolved = false;
			}
		//	uf::thread::add( mainThread, [&]() -> int {
				metadata["terrain"]["state"] = resolved ? "open" : "modified:1";
				metadata["terrain"]["modified"] = !resolved;
		//	return 0;}, true );
		}
	}
	// Handles initializing regions
	if ( metadata["terrain"]["state"] == "modified:1" ) { metadata["terrain"]["state"] = "resolving:" + metadata["terrain"]["state"].asString();
		bool resolved = true;
		/* Process new regions */ {
			for ( uf::Entity* kv : this->m_children ) { if ( !kv ) continue;
				ext::Region& region = *((ext::Region*) kv);
				uf::Serializer& rMetadata = kv->getComponent<uf::Serializer>();
				if ( !rMetadata["region"]["initialized"].asBool() ) region.initialize();
				if ( rMetadata["region"]["modified"].asBool() ) resolved = false;
			}
		//	uf::thread::add( mainThread, [&]() -> int {
			metadata["terrain"]["state"] = resolved ? "open" : "modified:2";
			metadata["terrain"]["modified"] = !resolved;
		//	return 0;}, true );
		}
		/* Relight */ {
			{
				std::vector<uf::Entity*> regions;
				/* Retrieve changed regions */ for ( uf::Entity* kv : this->m_children ) { if ( !kv ) continue; if ( kv->getName() != "Region" ) continue;
					uf::Serializer& rMetadata = kv->getComponent<uf::Serializer>();
					if ( !rMetadata["region"]["initialized"].asBool() ) continue;
					regions.push_back(kv);
				}
				/* Sort by closest to farthest */ {
					const ext::World& world = this->getRootParent<ext::World>();
					const uf::Object& player = world.getController<uf::Object>();
					const pod::Vector3& position = player.getComponent<pod::Transform<>>().position;
					std::sort( regions.begin(), regions.end(), [&]( const uf::Entity* l, const uf::Entity* r ){
						if ( !l ) return false; if ( !r ) return true;
						if ( !l->hasComponent<pod::Transform<>>() ) return false; if ( !r->hasComponent<pod::Transform<>>() ) return true;
						return uf::vector::magnitude( uf::vector::subtract( l->getComponent<pod::Transform<>>().position, position ) ) < uf::vector::magnitude( uf::vector::subtract( r->getComponent<pod::Transform<>>().position, position ) );
					} );
				}
				/* Update meshes */ {
					for ( uf::Entity* pointer : regions ) { ext::Region& region = *((ext::Region*) pointer);
						ext::TerrainGenerator& generator = region.getComponent<ext::TerrainGenerator>();
						generator.updateLight();
					}
				}
			}
		}
	}
	// Handles generation of region meshes; deferred to separate state to ensure neighbors are accurate
	if ( metadata["terrain"]["state"] == "modified:2" ) { metadata["terrain"]["state"] = "resolving:" + metadata["terrain"]["state"].asString();
		metadata["_system"]["limiter"] = uf::thread::limiter;
		uf::thread::limiter = 1 / metadata["terrain"]["load limiter"].asFloat();
		std::cout << "Force limiter from " << metadata["_system"]["limiter"].asFloat() << " to " << uf::thread::limiter << std::endl;
	//	uf::thread::add( uf::thread::fetchWorker(), [&]() -> int {
			/* Process changed regions */ if ( metadata["terrain"]["modified"].asBool() ) {
				std::vector<uf::Entity*> regions;
				/* Retrieve changed regions */ for ( uf::Entity* kv : this->m_children ) { if ( !kv ) continue; if ( kv->getName() != "Region" ) continue;
					uf::Serializer& rMetadata = kv->getComponent<uf::Serializer>();
					if ( !rMetadata["region"]["initialized"].asBool() ) continue;
					if ( rMetadata["region"]["modified"].asBool() || !rMetadata["region"]["rasterized"].asBool() ) {
						regions.push_back(kv);
					}
				}
				/* Sort by closest to farthest */ {
					const ext::World& world = this->getRootParent<ext::World>();
					const uf::Object& player = world.getController<uf::Object>();
					const pod::Vector3& position = player.getComponent<pod::Transform<>>().position;
					std::sort( regions.begin(), regions.end(), [&]( const uf::Entity* l, const uf::Entity* r ){
						if ( !l ) return false; if ( !r ) return true;
						if ( !l->hasComponent<pod::Transform<>>() ) return false; if ( !r->hasComponent<pod::Transform<>>() ) return true;
						return uf::vector::magnitude( uf::vector::subtract( l->getComponent<pod::Transform<>>().position, position ) ) < uf::vector::magnitude( uf::vector::subtract( r->getComponent<pod::Transform<>>().position, position ) );
					} );
				}
				/* Update meshes */ {
					for ( uf::Entity* pointer : regions ) { ext::Region& region = *((ext::Region*) pointer);
						uf::Serializer& rMetadata = region.getComponent<uf::Serializer>();
						rMetadata["region"]["rasterized"] = false;
						ext::TerrainGenerator& generator = region.getComponent<ext::TerrainGenerator>();
						uf::thread::add( uf::thread::fetchWorker(), [&]() -> int {
							if ( this->hasComponent<ext::TerrainGenerator::mesh_t>() ) {
								ext::TerrainGenerator::mesh_t& mesh = region.getComponent<ext::TerrainGenerator::mesh_t>();
								mesh.destroy();
								generator.rasterize(mesh.vertices, region);
							}
							rMetadata["region"]["rasterized"] = true;
							region.queueHook("region:Reload.%UID%", "");
						return 0;}, true );
						rMetadata["region"]["modified"] = false;
						rMetadata["region"]["generated"] = false;
					}
					bool resolved = true;
					if ( !regions.empty() ) resolved = false;
				//	uf::thread::add( uf::thread::fetchWorker(), [&]() -> int {
						metadata["terrain"]["state"] = resolved ? "open" : "modified:3";
						metadata["terrain"]["modified"] = false;
						this->queueHook("system:TickRate.Restore", "", resolved ? 0.5 : 1.0);
				//	return 0;}, true );
				}
			}
	//	return 0;}, true );
	}

	// Handles generating unified mesh; deferred to ensure all active regions possesses geometry
	if ( metadata["terrain"]["state"] == "modified:3" ) { metadata["terrain"]["state"] = "resolving:" + metadata["terrain"]["state"].asString();
	//	this->queueHook("system:TickRate.Restore", "", 1.0);
		uf::thread::add( uf::thread::fetchWorker(), [&]() -> int {
			/* Update unified mesh */  if ( metadata["terrain"]["unified"].asBool() ) {
				if ( this->hasComponent<ext::TerrainGenerator::mesh_t>() ) {
					this->queueHook("terrain:Reload.%UID%", "");
				}
				metadata["terrain"]["state"] = "open";
			}
		return 0; }, true );
	}

	this->relocatePlayer();
}
#endif
#if UF_EASY_TICK
void ext::Terrain::tick() {
	uf::Object::tick();


	ext::World& parent = this->getRootParent<ext::World>();
	const pod::Transform<>& player = parent.getController()->getComponent<pod::Transform<>>();
	
	uf::Serializer& metadata = this->getComponent<uf::Serializer>();

	if ( metadata["terrain"]["state"] == "preinit" ) {
		std::function<uf::Entity*(uf::Entity*, int)> recurse = [&]( uf::Entity* parent, int indent ) {
			for ( uf::Entity* entity : parent->getChildren() ) {
				if ( entity->getName() == "Player" ) return entity;
				uf::Entity* p = recurse(entity, indent + 1);
				if ( p ) return p;
			}
			return (uf::Entity*) NULL;
		};
		uf::Entity* pointer = recurse(&parent, 0);
		if ( pointer ) {
			this->generate();
			this->relocatePlayer();

			metadata["terrain"]["state"] = "modified:1";
		}
	}


	if ( this->m_children.empty() ) return;

	static uf::Timer<long long> generationTimer(false);

	pod::Thread& mainThread = uf::thread::has("Main") ? uf::thread::get("Main") : uf::thread::create( "Main", false, true );
	if ( metadata["terrain"]["state"] == "open" && 
		( (generationTimer.running() && generationTimer.elapsed().asDouble() > 1) || !generationTimer.running() )
	) { metadata["terrain"]["state"] = "resolving:" + metadata["terrain"]["state"].asString();
		if ( !generationTimer.running() ) generationTimer.start();
		else generationTimer.reset();

		/* Create/Destroy Regions */ {
			/* Cleanup orphans */ uf::thread::add( mainThread, [&]() -> int {
				std::vector<uf::Entity*> orphans;
				for ( uf::Entity* e : uf::Entity::entities ) {
					if ( !e ) continue;
					if ( e->hasParent() ) continue;
					if ( e->getName() != "Region" ) continue;
					orphans.push_back(e);
				}
				for ( uf::Entity* e : orphans ) {
					e->destroy<ext::Region>();
				}
			return 0;}, true );

			std::vector<pod::Vector3i> locations;
			for ( uf::Entity* kv : this->m_children ) {
				if ( !kv ) continue;
				if ( kv->getName() != "Region" ) continue;
				const pod::Transform<>& transform = kv->getComponent<pod::Transform<>>();
				pod::Vector3i location = { (int) transform.position.x / metadata["region"]["size"][0].asInt(), (int) transform.position.y / metadata["region"]["size"][1].asInt(), (int) transform.position.z / metadata["region"]["size"][2].asInt() };
				bool degenerate = false;
				if ( !this->inBounds( location ) ) degenerate = true;

				for ( uf::Entity* _kv : kv->getChildren() ) {
					if ( !_kv ) continue;
					uf::Serializer& _metadata = _kv->getComponent<uf::Serializer>();
					if ( _metadata["region"].isObject() && _metadata["region"]["persist"].asBool() ) {
						degenerate = false;
						break;
					}
				}
				{
					uf::Serializer& rMetadata = kv->getComponent<uf::Serializer>();
					if ( !rMetadata["region"]["rasterized"].asBool() ) degenerate = false;
				}

				if ( degenerate ) locations.push_back(location);
			}

			bool resolved = !metadata["terrain"]["modified"].asBool();
			if ( !locations.empty() ) {
				std::cout << "GENERATING" << std::endl;
				this->generate();
				for ( pod::Vector3i& location : locations ) this->degenerate(location);
				resolved = false;
			}
			uf::thread::add( mainThread, [&]() -> int {
//				std::cout << metadata["terrain"]["state"].asString() << " -> ";
				metadata["terrain"]["state"] = resolved ? "open" : "modified:1";
				metadata["terrain"]["modified"] = !resolved;
//				std::cout << metadata["terrain"]["state"].asString() << std::endl;
			return 0;}, true );
		}
	}
	if ( true ) {
		if ( metadata["terrain"]["state"] == "modified:1" ) { metadata["terrain"]["state"] = "resolving:" + metadata["terrain"]["state"].asString();
			uf::thread::add( uf::thread::fetchWorker(), [&]() -> int {
				std::vector<uf::Entity*> regions;
				// Retrieve changed regions
				for ( uf::Entity* kv : this->m_children ) { if ( !kv ) continue;
//					std::cout << kv->getName() << ": " << kv << std::endl;
					// if ( kv->getName() != "Region" ) continue;
					uf::Serializer& rMetadata = kv->getComponent<uf::Serializer>();
					if ( rMetadata["region"]["initialized"].asBool() ) continue;
					regions.push_back(kv);
				}
				// Sort by closest to farthest
				{
					const ext::World& world = this->getRootParent<ext::World>();
					const pod::Vector3& position = world.getController()->getComponent<pod::Transform<>>().position;
					std::sort( regions.begin(), regions.end(), [&]( const uf::Entity* l, const uf::Entity* r ){
						if ( !l ) return false; if ( !r ) return true;
						if ( !l->hasComponent<pod::Transform<>>() ) return false; if ( !r->hasComponent<pod::Transform<>>() ) return true;
						return uf::vector::magnitude( uf::vector::subtract( l->getComponent<pod::Transform<>>().position, position ) ) < uf::vector::magnitude( uf::vector::subtract( r->getComponent<pod::Transform<>>().position, position ) );
					} );
				}
				// Update meshes
				for ( uf::Entity* kv : regions ) { ext::Region& region = *((ext::Region*) kv);
					uf::Serializer& rMetadata = kv->getComponent<uf::Serializer>();
					region.initialize();
					rMetadata["region"]["initialized"] = true;
				}
				for ( uf::Entity* kv : regions ) { ext::Region& region = *((ext::Region*) kv);
					uf::Serializer& rMetadata = kv->getComponent<uf::Serializer>();
					ext::TerrainGenerator& generator = region.getComponent<ext::TerrainGenerator>();
					generator.updateLight();
					rMetadata["region"]["generated"] = true;
				}
				for ( uf::Entity* kv : regions ) { ext::Region& region = *((ext::Region*) kv);
					uf::Serializer& rMetadata = kv->getComponent<uf::Serializer>();
					ext::TerrainGenerator& generator = region.getComponent<ext::TerrainGenerator>();
					if ( this->hasComponent<ext::TerrainGenerator::mesh_t>() ) {
						ext::TerrainGenerator::mesh_t& mesh = region.getComponent<ext::TerrainGenerator::mesh_t>();
						mesh.destroy();
						generator.rasterize(mesh.vertices, region);
					}
					rMetadata["region"]["rasterized"] = true;
				}
				for ( uf::Entity* kv : regions ) { ext::Region& region = *((ext::Region*) kv);
					region.queueHook("region:Reload.%UID%", "");
				}
				metadata["terrain"]["state"] = "open";
			return 0;}, true );
		}
	} else {
//		if ( metadata["terrain"]["state"] != "open" ) std::cout << metadata["terrain"]["state"].asString() << std::endl;

		if ( metadata["terrain"]["state"] == "modified:1" ) { metadata["terrain"]["state"] = "resolving:" + metadata["terrain"]["state"].asString();
			std::vector<uf::Entity*> regions;
			// Retrieve changed regions
			bool resolved = true;
			for ( uf::Entity* kv : this->m_children ) { if ( !kv ) continue;
				// if ( kv->getName() != "Region" ) continue;
				uf::Serializer& rMetadata = kv->getComponent<uf::Serializer>();
				if ( rMetadata["region"]["initialized"].asBool() ) continue;
				regions.push_back(kv);
				resolved = false;
			}
			// Update meshes
			for ( uf::Entity* kv : regions ) { ext::Region& region = *((ext::Region*) kv);
				uf::Serializer& rMetadata = kv->getComponent<uf::Serializer>();
				region.initialize();
				rMetadata["region"]["initialized"] = true;
				resolved = false;
			}
		//	uf::thread::add( mainThread, [&]() -> int {
//				std::cout << metadata["terrain"]["state"].asString() << " -> ";
				metadata["terrain"]["state"] = resolved ? "open" : "modified:2";
				metadata["terrain"]["modified"] = !resolved;
//				std::cout << metadata["terrain"]["state"].asString() << std::endl;
		//	return 0;}, true );
		}
		if ( metadata["terrain"]["state"] == "modified:2" ) { metadata["terrain"]["state"] = "resolving:" + metadata["terrain"]["state"].asString();
			uf::thread::add( uf::thread::fetchWorker(), [&]() -> int {
				std::vector<uf::Entity*> regions;
				for ( uf::Entity* kv : this->m_children ) { if ( !kv ) continue;
				//	if ( kv->getName() != "Region" ) continue;
					uf::Serializer& rMetadata = kv->getComponent<uf::Serializer>();
					if ( rMetadata["region"]["initialized"].asBool() && rMetadata["region"]["generated"].asBool() ) continue;
					regions.push_back(kv);
				}
				{
					const ext::World& world = this->getRootParent<ext::World>();
					const pod::Vector3& position = world.getController()->getComponent<pod::Transform<>>().position;
					std::sort( regions.begin(), regions.end(), [&]( const uf::Entity* l, const uf::Entity* r ){
						if ( !l ) return false; if ( !r ) return true;
						if ( !l->hasComponent<pod::Transform<>>() ) return false; if ( !r->hasComponent<pod::Transform<>>() ) return true;
						return uf::vector::magnitude( uf::vector::subtract( l->getComponent<pod::Transform<>>().position, position ) ) < uf::vector::magnitude( uf::vector::subtract( r->getComponent<pod::Transform<>>().position, position ) );
					} );
				}
				bool resolved = true;
				for ( uf::Entity* kv : regions ) { ext::Region& region = *((ext::Region*) kv);
					uf::Serializer& rMetadata = kv->getComponent<uf::Serializer>();
					ext::TerrainGenerator& generator = region.getComponent<ext::TerrainGenerator>();
					generator.updateLight();
					rMetadata["region"]["generated"] = true;
					resolved = false;

				}
			//	uf::thread::add( mainThread, [&]() -> int {
//					std::cout << metadata["terrain"]["state"].asString() << " -> ";
					metadata["terrain"]["state"] = resolved ? "open" : "modified:3";
					metadata["terrain"]["modified"] = !resolved;
//					std::cout << metadata["terrain"]["state"].asString() << std::endl;
			//	return 0;}, true );
			return 0;}, true );
		}
		if ( metadata["terrain"]["state"] == "modified:3" ) { metadata["terrain"]["state"] = "resolving:" + metadata["terrain"]["state"].asString();
			uf::thread::add( uf::thread::fetchWorker(), [&]() -> int {
				std::vector<uf::Entity*> regions;
				for ( uf::Entity* kv : this->m_children ) { if ( !kv ) continue;
					if ( kv->getName() != "Region" ) continue;
					uf::Serializer& rMetadata = kv->getComponent<uf::Serializer>();
					if ( rMetadata["region"]["generated"].asBool() && !rMetadata["region"]["rasterize"].asBool() ) continue;
					regions.push_back(kv);
				}
				bool resolved = true;
				for ( uf::Entity* kv : regions ) { ext::Region& region = *((ext::Region*) kv);
					uf::Serializer& rMetadata = kv->getComponent<uf::Serializer>();
					ext::TerrainGenerator& generator = region.getComponent<ext::TerrainGenerator>();
					if ( this->hasComponent<ext::TerrainGenerator::mesh_t>() ) {
						ext::TerrainGenerator::mesh_t& mesh = region.getComponent<ext::TerrainGenerator::mesh_t>();
						mesh.destroy();
						generator.rasterize(mesh.vertices, region);
					}
					rMetadata["region"]["rasterized"] = true;
				}
				for ( uf::Entity* kv : regions ) { ext::Region& region = *((ext::Region*) kv);
					region.queueHook("region:Reload.%UID%", "");
				}
			//	uf::thread::add( mainThread, [&]() -> int {
//					std::cout << metadata["terrain"]["state"].asString() << " -> ";
					metadata["terrain"]["state"] = resolved ? "open" : "modified:1";
					metadata["terrain"]["modified"] = !resolved;
//					std::cout << metadata["terrain"]["state"].asString() << std::endl;
			//	return 0;}, true );
			return 0;}, true );
		}
	}

	this->relocatePlayer();
}
#endif
void ext::Terrain::render() {
	if ( !this->m_parent ) return;
	if ( this->m_children.empty() ) return;

	uf::Object::render();

	/* Update uniforms */ if ( this->hasComponent<ext::vulkan::RTGraphic>() ) {
		auto& world = this->getRootParent<ext::World>();
		auto& rt = this->getComponent<ext::vulkan::RTGraphic>();
		auto& player = world.getController<uf::Object>();
		auto& camera = player.getComponent<uf::Camera>();
		auto& transform = player.getComponent<pod::Transform<>>();
		pod::Transform<> flatten = uf::transform::flatten(camera.getTransform(), true);
		rt.compute.uniforms.camera.view = uf::quaternion::matrix( flatten.orientation ); 
		rt.compute.uniforms.camera.aspectRatio = (float) camera.getSize().x / (float) camera.getSize().y;
		rt.compute.uniforms.camera.position = camera.getTransform().position + transform.position;
		rt.updateUniformBuffer();
	}
	/* Update uniforms */ if ( this->hasComponent<ext::TerrainGenerator::mesh_t>() ) {
		auto& mesh = this->getComponent<ext::TerrainGenerator::mesh_t>();
		auto& world = this->getRootParent<ext::World>();
		auto& player = world.getController<uf::Object>();
		auto& camera = player.getComponent<uf::Camera>();
		auto& transform = player.getComponent<pod::Transform<>>();
		if ( !mesh.generated ) return;
		camera.updateView();
		// ext::vulkan::MeshGraphic& graphic = *((ext::vulkan::MeshGraphic*) mesh.graphic);
		//auto& uniforms = mesh.graphic.uniforms<uf::StereoMeshDescriptor>();
		uf::StereoMeshDescriptor uniforms;
		uniforms.matrices.model = uf::matrix::identity();
		for ( std::size_t i = 0; i < 2; ++i ) {
			uniforms.matrices.view[i] = camera.getView( i );
			uniforms.matrices.projection[i] = camera.getProjection( i );
		}
		mesh.graphic.updateBuffer( uniforms, 0, false );
	}
}

void ext::Terrain::relocatePlayer() {
	std::vector<uf::Entity*> entities;
	ext::World& parent = this->getRootParent<ext::World>();
	std::function<void(uf::Entity*)> recurse = [&]( uf::Entity* parent ) {
		for ( uf::Entity* entity : parent->getChildren() ) {
			if ( !entity ) continue;
			if ( entity->getUid() == 0 ) continue;
			uf::Serializer& metadata = entity->getComponent<uf::Serializer>();
			if ( metadata["region"].isObject() && metadata["region"]["track"].asBool() ) {
				if ( std::find( entities.begin(), entities.end(), entity ) == entities.end() ) {
					entities.push_back(entity);
				}
			}
			recurse(entity);
		}
	}; recurse(&parent);
	
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
			uf::iostream << "Relocating " << &entity << ": " << entity.getName() << " to (" << point.x << ", " << point.y << ", " << point.z << ")" << "\n";
		}
	//	entity.getComponent<uf::Camera>().update(true);
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

void ext::Terrain::generate() {
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
	// uf::iostream << "Generating Regions @ ( " << location.x << ", " << location.y << ", " << location.z << ")" << "\n";

	for ( int y = 0; y < radius.y; ++y ) {
	for ( int z = 0; z < radius.z; ++z ) {
	for ( int x = 0; x < radius.x; ++x ) {
/*
	for ( int x = radius.x - 1; x >= 0; --x ) {
	for ( int y = radius.y - 1; y >= 0; --y ) {
	for ( int z = radius.z - 1; z >= 0; --z ) {
*/
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

	// uf::iostream << "Generating Region @ ( " << position.x << ", " << position.y << ", " << position.z << ")" << "\n";
	uf::Entity* base = new ext::Region; this->addChild(*base); {
		ext::Region& region = *((ext::Region*)base);
		uf::Serializer& m = region.getComponent<uf::Serializer>(); // = metadata;
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

	::region_table.erase( position );
}