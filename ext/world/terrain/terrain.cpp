#include "terrain.h"

#include "../../ext.h"
#include "generator.h"
#include "region.h"

#include <uf/utils/window/window.h>


void ext::Terrain::initialize() {
	ext::Object::initialize();

	this->m_name = "Terrain";
	
	this->generate();
	this->relocatePlayer();

	uf::Serializer& metadata = this->getComponent<uf::Serializer>();
	metadata["terrain"]["state"] = "modified:1";
}
#include <uf/utils/thread/thread.h>
void ext::Terrain::tick() {
	uf::Entity::tick();
	
	if ( this->m_children.empty() ) return;

	ext::World& parent = this->getRootParent<ext::World>();
	const pod::Transform<>& player = parent.getPlayer().getComponent<pod::Transform<>>();
	
	uf::Serializer& metadata = this->getComponent<uf::Serializer>();

	/* Debug Information */ {
		/* Terrain Metadata */ {
			static uf::Timer<long long> timer(false);
			if ( !timer.running() ) timer.start();
			if ( uf::Window::isKeyPressed("L") && timer.elapsed().asDouble() >= 1 ) { timer.reset();
				std::cout << metadata << std::endl;
			}
		}
		/* Terrain Reset */ {
			static uf::Timer<long long> timer(false);
			if ( !timer.running() ) timer.start();
			if ( uf::Window::isKeyPressed("G") && timer.elapsed().asDouble() >= 1 ) { timer.reset();
				std::cout << "Regenerating ";

				uint i = 0;
				/* Retrieve changed regions */ for ( uf::Entity* kv : this->m_children ) { if ( !kv ) continue; if ( kv->getName() != "Region" ) continue;
					uf::Serializer& rMetadata = kv->getComponent<uf::Serializer>();
					if ( !rMetadata["region"]["initialized"].asBool() ) continue;
					rMetadata["region"]["modified"] = true;
					rMetadata["region"]["rasterized"] = false;
//					kv->getComponent<uf::Mesh>().clear();
//					kv->getComponent<uf::Mesh>().destroy();
//					kv->getComponent<uf::Mesh>() = uf::Mesh();
					++i;
				}
//				this->getComponent<uf::Mesh>().clear();
//				this->getComponent<uf::Mesh>().destroy();
				metadata["terrain"]["modified"] = true;
				std::cout << i << " regions" << std::endl;
			}
		}
	}

	// Open Gamestate: Handles creating and destroying region entities
	if ( metadata["terrain"]["state"] == "open" ) { metadata["terrain"]["state"] = "resolving:" + metadata["terrain"]["state"].asString();
		/* Create/Destroy Regions */ {
			/* Cleanup orphans */ uf::thread::add( uf::thread::has("Main") ? uf::thread::get("Main") : uf::thread::create( "Main", false, true ), [&]() -> int {
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
				if ( !this->inBounds( location ) ) locations.push_back(location);
			}

			bool resolved = !metadata["terrain"]["modified"].asBool();
			if ( !locations.empty() ) {
				this->generate();
				for ( pod::Vector3i& location : locations ) this->degenerate(location);
				resolved = false;
			}
			uf::thread::add( uf::thread::has("Main") ? uf::thread::get("Main") : uf::thread::create( "Main", false, true ), [&]() -> int {
				metadata["terrain"]["state"] = resolved ? "open" : "modified:1";
				metadata["terrain"]["modified"] = !resolved;
			return 0;}, true );
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
			uf::thread::add( uf::thread::has("Main") ? uf::thread::get("Main") : uf::thread::create( "Main", false, true ), [&]() -> int {
				metadata["terrain"]["state"] = resolved ? "open" : "modified:2";
				metadata["terrain"]["modified"] = !resolved;
			return 0;}, true );
		}
	}
	// Handles generation of region meshes; deferred to separate state to ensure neighbors are accurate
	if ( metadata["terrain"]["state"] == "modified:2" ) { metadata["terrain"]["state"] = "resolving:" + metadata["terrain"]["state"].asString();
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
				const ext::Player& player = world.getPlayer();
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
					uf::Mesh& mesh = region.getComponent<uf::Mesh>() = uf::Mesh();
					ext::TerrainGenerator& generator = region.getComponent<ext::TerrainGenerator>();
					uf::thread::add( uf::thread::has("Aux") ? uf::thread::get("Aux") : uf::thread::create( "Aux", true, false ), [&]() -> int {
						uf::Mesh nesh;
						generator.rasterize(nesh, region, false, false );
						mesh.copy(nesh);
						uf::thread::add( uf::thread::has("Main") ? uf::thread::get("Main") : uf::thread::create( "Main", false, true ), [&]() -> int {
						//	mesh.index();
							mesh.generate();
							rMetadata["region"]["rasterized"] = true;
							rMetadata["region"]["unified"] = false;
						return 0;}, true );
					return 0;}, true );
					rMetadata["region"]["modified"] = false;
				}
				bool resolved = true;
				if ( !regions.empty() ) {
					resolved = false;
//					this->getComponent<uf::Mesh>() = uf::Mesh();
				}
				uf::thread::add( uf::thread::has("Aux") ? uf::thread::get("Aux") : uf::thread::create( "Aux", true, false ), [&]() -> int {
				uf::thread::add( uf::thread::has("Main") ? uf::thread::get("Main") : uf::thread::create( "Main", false, true ), [&]() -> int {
					metadata["terrain"]["state"] = resolved ? "open" : "modified:3";
					metadata["terrain"]["modified"] = false;
				return 0;}, true );
				return 0;}, true );
			}
		}
	}
	// Handles generating unified mesh; deferred to ensure all active regions possesses geometry
	if ( metadata["terrain"]["state"] == "modified:3" ) { metadata["terrain"]["state"] = "resolving:" + metadata["terrain"]["state"].asString();
		/* Update unified mesh */ if ( metadata["terrain"]["unified"].asBool() ) {
			uf::thread::add( uf::thread::has("Aux") ? uf::thread::get("Aux") : uf::thread::create( "Aux", true, false ), [&]() -> int {
			uf::thread::add( uf::thread::has("Main") ? uf::thread::get("Main") : uf::thread::create( "Main", false, true ), [&]() -> int {
				uf::Mesh& mesh = this->getComponent<uf::Mesh>() = uf::Mesh();
				uf::Mesh nesh;
				for ( uf::Entity* kv : this->m_children ) { if ( !kv ) continue; if ( kv->getName() != "Region" ) continue;
					uf::Serializer& rMetadata = kv->getComponent<uf::Serializer>();
					uf::Mesh& rMesh = kv->getComponent<uf::Mesh>();
					nesh.insert(rMesh);
					rMetadata["region"]["unified"] = true;
				}
				mesh.copy(nesh);
				mesh.generate();
				metadata["terrain"]["state"] = "open";
				std::cout << "Finished!" << std::endl;
			return 0;}, true );
			return 0;}, true );
		}  else {
			metadata["terrain"]["state"] = "open";
		}
	}
	
	this->relocatePlayer();
}
void ext::Terrain::render() {
	if ( !this->m_parent ) return; if ( this->m_children.empty() ) return;

	uf::Entity::render();

	if ( !this->hasComponent<uf::Mesh>() ) return;
	if ( !this->getComponent<uf::Mesh>().generated() ) return;

	ext::World& root = this->getRootParent<ext::World>();
		
	pod::Transform<>& transform = this->getComponent<pod::Transform<>>();
	uf::Shader& shader = this->getComponent<uf::Shader>();
	uf::Camera& camera = root.getCamera(); //root.getPlayer().getComponent<uf::Camera>();
	uf::Mesh& mesh = this->getComponent<uf::Mesh>();

	int slot = 0; 
	if ( this->hasComponent<uf::Texture>() ) {
		uf::Texture& texture = this->getComponent<uf::Texture>();
		texture.bind(slot);
	}
	struct {
		int diffuse = 0; int specular = 1; int normal = 2;
	} slots;
	if ( this->hasComponent<uf::Material>() ) {
		uf::Material& material = this->getComponent<uf::Material>();
		material.bind(slots.diffuse, slots.specular, slots.normal);
	}
	shader.bind(); {
		camera.update();
		shader.push("model", uf::matrix::identity());
		shader.push("view", camera.getView());
		shader.push("projection", camera.getProjection());
		if ( this->hasComponent<uf::Texture>() ) shader.push("texture", slot);
		if ( this->hasComponent<uf::Material>() ) {
			if ( slots.diffuse >= 0 ) shader.push("diffuse", slots.diffuse);
			if ( slots.specular >= 0 ) shader.push("specular", slots.specular);
			if ( slots.normal >= 0 ) shader.push("normal", slots.normal);
		}
	}
	mesh.render();
}

void ext::Terrain::relocatePlayer() {
	ext::World& parent = this->getRootParent<ext::World>();
	ext::Player& player = parent.getPlayer();
	const pod::Transform<>& transform = player.getComponent<pod::Transform<>>();
	uf::Serializer& metadata = this->getComponent<uf::Serializer>();
	pod::Vector3ui size = {
		metadata["region"]["size"][0].asUInt(),
		metadata["region"]["size"][1].asUInt(),
		metadata["region"]["size"][2].asUInt(),
	};
	pod::Vector3 pointf = uf::vector::divide(transform.position, {size.x, size.y, size.z});
	pod::Vector3i point = {
		(int) (pointf.x + (pointf.x > 0 ? 0.5 : -0.5)),
		(int) (pointf.y + (pointf.y > 0 ? 0.5 : -0.5)),
		(int) (pointf.z + (pointf.z > 0 ? 0.5 : -0.5)),
	};


	if ( !this->exists(point) ) return; // oops
	ext::Region& region = *this->at(point);
	bool should = false;
	if ( player.getParent().getName() != "Region" ) should = true;
	else {
		ext::Region& current = player.getParent<ext::Region>();
		const pod::Transform<>& t = current.getComponent<pod::Transform<>>();
		pod::Vector3i location = {
			(int) (t.position.x / size.x),
			(int) (t.position.y / size.y),
			(int) (t.position.z / size.z),
		};
		if ( !uf::vector::equals( point, location ) ) should = true;
	}
	if ( should ) {
		region.moveChild(player);
		std::cout << "Relocating to (" << point.x << ", " << point.y << ", " << point.z << ")" << std::endl;
	}
	player.getComponent<uf::Camera>().update(true);
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
	const pod::Transform<>& player = parent.getPlayer().getComponent<pod::Transform<>>();
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
	const pod::Transform<>& player = parent.getPlayer().getComponent<pod::Transform<>>();
	pod::Vector3i location = {
		(int) player.position.x / size.x,
		(int) player.position.y / size.y,
		(int) player.position.z / size.z,
	};
	// uf::iostream << "Generating Regions @ ( " << location.x << ", " << location.y << ", " << location.z << ")" << "\n";

	for ( int x = 0; x < radius.x; ++x ) {
	for ( int y = 0; y < radius.y; ++y ) {
	for ( int z = 0; z < radius.z; ++z ) {
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
		uf::Serializer& m = region.getComponent<uf::Serializer>() = metadata;

		pod::Transform<>& transform = region.getComponent<pod::Transform<>>();
		transform.position.x = position.x * size.x;
		transform.position.y = position.y * size.y;
		transform.position.z = position.z * size.z;

		m["region"]["location"][0] = position.x;
		m["region"]["location"][1] = position.y;
		m["region"]["location"][2] = position.z;
	}

}
void ext::Terrain::degenerate( const pod::Vector3i& position ) {
	if ( !this->exists( position ) ) return;
	ext::Region* region = this->at( position );
	if ( !region ) return;
	this->removeChild( *region );
/*
	uf::Serializer& metadata = this->getComponent<uf::Serializer>();
	pod::Vector3i size = {
		metadata["region"]["size"][0].asInt(),
		metadata["region"]["size"][1].asInt(),
		metadata["region"]["size"][2].asInt(),
	};
	if ( this->m_children.empty() ) return;
	for ( auto it = this->m_children.begin(); it != this->m_children.end(); ++it ) { uf::Entity* kv = *it;
		const pod::Transform<>& transform = kv->getComponent<pod::Transform<>>();
		pod::Vector3i location = {
			(int) (transform.position.x / size.x),
			(int) (transform.position.y / size.y),
			(int) (transform.position.z / size.z),
		};
		if ( uf::vector::equals( location, position ) ) {
			for ( uf::Entity* e : kv->getChildren() ) if ( e->getName() == "Player" ) return;

			this->removeChild(*it);
		//	*it = NULL;
		//	this->m_children.erase(it);
		//	delete kv;
			break;
		}
	}
*/
}