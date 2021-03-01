#include "behavior.h"
#include "terrain.h"
#include "generator.h"

#include "../../../ext.h"
#include <uf/engine/asset/asset.h>
#include "generator.h"
#include "region.h"

#include <uf/utils/window/window.h>
#include <uf/utils/graphic/mesh.h>
#include <uf/utils/graphic/graphic.h>
#include <uf/utils/camera/camera.h>
#include <uf/utils/math/collision.h>
#include <uf/utils/thread/thread.h>
#include <uf/utils/renderer/renderer.h>

#include <uf/utils/string/hash.h>

#include <sys/stat.h>

namespace {
	std::unordered_map<pod::Vector3i, uf::Object*> region_table;
}

UF_OBJECT_REGISTER_BEGIN(ext::Terrain)
	UF_OBJECT_REGISTER_BEHAVIOR(uf::EntityBehavior)
	UF_OBJECT_REGISTER_BEHAVIOR(uf::ObjectBehavior)
	UF_OBJECT_REGISTER_BEHAVIOR(ext::TerrainBehavior)
UF_OBJECT_REGISTER_END()
void ext::Terrain::relocateChildren() {
	uf::Scene& scene = uf::scene::getCurrentScene();
	std::vector<uf::Entity*> entities;
	scene.process( [&]( uf::Entity* entity ) {
		if ( !entity || entity->getUid() == 0 ) return;
		uf::Serializer& metadata = entity->getComponent<uf::Serializer>();
		if ( ext::json::isObject( metadata["region"] ) && metadata["region"]["track"].as<bool>() ) {
			if ( std::find( entities.begin(), entities.end(), entity ) == entities.end() ) {
				entities.push_back(entity);
			}
		}
	} );
	
	uf::Serializer& metadata = this->getComponent<uf::Serializer>();
	pod::Vector3ui size = {
		metadata["region"]["size"][0].as<size_t>(),
		metadata["region"]["size"][1].as<size_t>(),
		metadata["region"]["size"][2].as<size_t>(),
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
		uf::Object& region = *this->at(point);
		bool should = false;
		if ( entity.getParent().getName() != "Region" ) should = true;
		else {
			auto& current = entity.getParent();
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
		metadata["region"]["size"][0].as<int>(),
		metadata["region"]["size"][1].as<int>(),
		metadata["region"]["size"][2].as<int>(),
	};
	if ( this->getChildren().empty() ) return false;
	for ( auto it = this->getChildren().begin(); it != this->getChildren().end(); ++it ) { uf::Entity* kv = *it;
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
	const uf::Scene& scene = uf::scene::getCurrentScene();
	const pod::Transform<>& player = scene.getController().getComponent<pod::Transform<>>();
	const uf::Serializer& metadata = this->getComponent<uf::Serializer>();
	pod::Vector3ui size = {
		metadata["region"]["size"][0].as<size_t>(),
		metadata["region"]["size"][1].as<size_t>(),
		metadata["region"]["size"][2].as<size_t>(),
	};
	pod::Vector3ui threshold = {
		metadata["terrain"]["radius"][0].as<size_t>(),
		metadata["terrain"]["radius"][1].as<size_t>(),
		metadata["terrain"]["radius"][2].as<size_t>(),
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
		metadata["terrain"]["radius"][0].as<int>(),
		metadata["terrain"]["radius"][1].as<int>(),
		metadata["terrain"]["radius"][2].as<int>(),
	};
	pod::Vector3i size = {
		metadata["region"]["size"][0].as<int>(),
		metadata["region"]["size"][1].as<int>(),
		metadata["region"]["size"][2].as<int>(),
	};

	uf::Scene& scene = uf::scene::getCurrentScene();
	const pod::Transform<>& player = scene.getController().getComponent<pod::Transform<>>();
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
uf::Object* ext::Terrain::at( const pod::Vector3i& position ) const {
	if ( ::region_table.count(position) > 0 ) return ::region_table.at(position);

	const uf::Serializer& metadata = this->getComponent<uf::Serializer>();
	pod::Vector3i size = {
		metadata["region"]["size"][0].as<int>(),
		metadata["region"]["size"][1].as<int>(),
		metadata["region"]["size"][2].as<int>(),
	};
	if ( this->getChildren().empty() ) return NULL;
	for ( auto it = this->getChildren().begin(); it != this->getChildren().end(); ++it ) { uf::Entity* kv = *it;
		const pod::Transform<>& transform = kv->getComponent<pod::Transform<>>();
		pod::Vector3i location = {
			(int) (transform.position.x / size.x),
			(int) (transform.position.y / size.y),
			(int) (transform.position.z / size.z),
		};
		if ( uf::vector::equals( location, position ) ) return (uf::Object*) kv;
	}
	return NULL;
}
void ext::Terrain::generate( const pod::Vector3i& position ) {
	uf::Serializer& metadata = this->getComponent<uf::Serializer>();
	pod::Vector3i size = {
		metadata["region"]["size"][0].as<int>(),
		metadata["region"]["size"][1].as<int>(),
		metadata["region"]["size"][2].as<int>(),
	};

	if ( this->exists(position) ) return;
	if ( !this->inBounds(position) ) return;
/*
	auto& region = uf::instantiator::instantiate<uf::Object>();
	uf::instantiator::bind("RegionBehavior", region);
	this->addChild(region);
*/
	auto& region = this->loadChild("./region.json", false);
	{
		uf::Serializer& m = region.getComponent<uf::Serializer>(); // = metadata;
		m["system"]["type"] = "Region";
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
	uf::Object* region = this->at( position );
	if ( !region ) return;
	this->removeChild( *region );
	region->destroy();
	delete region;
	::region_table.erase( position );
}