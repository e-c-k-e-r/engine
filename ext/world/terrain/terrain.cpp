#include "terrain.h"

#include "../../ext.h"
#include "generator.h"
#include "region.h"

void ext::Terrain::initialize() {
	this->m_name = "Terrain";
	
	this->generate();
}
void ext::Terrain::tick() {
	uf::Entity::tick();
	ext::World& parent = this->getRootParent<ext::World>();
	const pod::Transform<>& player = parent.getPlayer().getComponent<pod::Transform<>>();
	const uf::Serializer& metadata = this->getComponent<uf::Serializer>();

	pod::Vector3ui size = {
		metadata["region"]["size"][0].asUInt(),
		metadata["region"]["size"][1].asUInt(),
		metadata["region"]["size"][2].asUInt(),
	};
	if ( this->m_children.empty() ) return;
	
	for ( uf::Entity* kv : this->m_children ) {
		const pod::Transform<>& transform = kv->getComponent<pod::Transform<>>();
		pod::Vector3i location = {
			(int) (transform.position.x / size.x),
			(int) (transform.position.y / size.y),
			(int) (transform.position.z / size.z),
		};

		if ( !this->inBounds(location) ) {
			this->degenerate(location);
			this->generate();
			continue;
		}
	}
	for ( uf::Entity* kv : this->m_children ) {
		// Defer generation to make sure all neighbors have their voxels generated
		std::vector<ext::Region*> queue;
		for ( uf::Entity* kv : this->m_children ) { if ( !kv ) continue;
			ext::Region& region = *((ext::Region*) kv);
			if ( !region.hasComponent<ext::TerrainGenerator>() ) {
				region.initialize();
				queue.push_back(&region);
			}
		}

		// Defer rasterization to make sure all neighbors have their voxels generated
		for ( ext::Region* kv : queue ) { if ( !kv ) continue;
			uf::Mesh& mesh = kv->getComponent<uf::Mesh>();
			ext::TerrainGenerator& generator = kv->getComponent<ext::TerrainGenerator>();
			generator.rasterize(mesh, *kv);
		}
	}
	this->relocatePlayer();
}
void ext::Terrain::render() {
	if ( !this->m_parent ) return; if ( this->m_children.empty() ) return;
	uf::Entity::render();
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
	
	//	region.initialize();
	}

}
void ext::Terrain::degenerate( const pod::Vector3i& position ) {
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
			for ( uf::Entity* e : kv->getChildren() ) if ( e->getName() == "Player" ) {
				return;
			}
		/*
			for ( uf::Entity* e : kv->getChildren() ) if ( e->getName() == "Player" ) {
				this->getRootParent<ext::World>().moveChild(*e);
				std::cout << "Emergency Provisions" << std::endl;
				std::function<void(const uf::Entity*, int)> recurse = [&]( const uf::Entity* parent, int indent ) {
					for ( const uf::Entity* entity : parent->getChildren() ) {
						for ( int i = 0; i < indent; ++i ) std::cout<<"\t";
						std::cout<<entity->getName()<<std::endl;
						recurse(entity, indent + 1);
					}
				}; recurse(&this->getRootParent<ext::World>(), 0);
				std::cout << "Emergency Provisions" << std::endl;
			}
		*/
			delete kv; *it = NULL;
			this->m_children.erase(it);
			// this->generate();
			break;
		}
	}
}