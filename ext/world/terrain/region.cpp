#include "region.h"

#include "../../ext.h"
#include "generator.h"
#include "../light/light.h"

void ext::Region::initialize() {	
	this->addComponent<pod::Transform<>>();
	this->addComponent<ext::TerrainGenerator>();

	this->m_name = "Region";

	this->load();
}
void ext::Region::tick() {
	uf::Entity::tick();
}
void ext::Region::load() {
	uf::Mesh& mesh = this->getComponent<uf::Mesh>();
	ext::TerrainGenerator& generator = this->getComponent<ext::TerrainGenerator>();
	uf::Serializer& metadata = this->getComponent<uf::Serializer>();
	
	pod::Vector3ui size; {
		size.x = metadata["region"]["size"][0].asUInt();
		size.y = metadata["region"]["size"][1].asUInt();
		size.z = metadata["region"]["size"][2].asUInt();
	}

	float r = (rand() % 100) / 100.0;
	bool addLight = r < metadata["region"]["light"]["random"].asFloat();
	// Guarantee at least one light source (per XZ plane)
	if ( !addLight ) { addLight = true;
		ext::Terrain& parent = this->getParent<ext::Terrain>();
		for ( const uf::Entity* pRegion : parent.getChildren() ) if ( pRegion->getName() == "Region" ) {
			const pod::Transform<>& tRegion = pRegion->getComponent<pod::Transform<>>();
			const pod::Transform<>& transform = this->getComponent<pod::Transform<>>();
			if ( tRegion.position.y != transform.position.y ) continue;
			for ( const uf::Entity* pLight : pRegion->getChildren() ) if ( pLight->getName() == "Light" ) {
				addLight = false;
				break;
			}
			if ( !addLight ) break;
		}
	}

	if ( addLight ) {
	//	std::cout << metadata["region"]["location"][0] << ", " << metadata["region"]["location"][1] << ", " << metadata["region"]["location"][2] << std::endl;
		pod::Vector3 color = { 1.0, 1.0, 1.0 }; {
		//	float r = (rand() % 256) / 256.0; color.x -= r;
		//	float g = (rand() % 256) / 256.0; color.y -= g;
		//	float b = (rand() % 256) / 256.0; color.z -= b;

			color = uf::vector::normalize( color );
		}
		int radius = metadata["region"]["light"]["radius"].asInt();
		/* Up */ if ( metadata["region"]["light"]["up"].asBool() ) {
			uf::Entity* entity = new ext::Light;
			if ( !((ext::Object*) entity)->load("./light/config.json") ) { uf::iostream << "Error loading `" << "./light/config.json" << "!" << "\n"; delete entity; return; }
			this->addChild(*entity);
			entity->initialize();

			pod::Transform<>& parent = this->getComponent<pod::Transform<>>();
			pod::Transform<>& transform = entity->getComponent<pod::Transform<>>();
			entity->getComponent<uf::Camera>().getTransform().reference = &transform;

			transform = uf::transform::initialize( transform );
			transform.position = parent.position;
			uf::transform::rotate( transform, transform.right, 1.5708 * 1 );
			entity->getComponent<uf::Camera>().setFov(120);
			entity->getComponent<uf::Camera>().update(true);
			((ext::Light*)entity)->setColor( color );
		}
		/* Down */ if ( metadata["region"]["light"]["down"].asBool() ) {
			uf::Entity* entity = new ext::Light;
			if ( !((ext::Object*) entity)->load("./light/config.json") ) { uf::iostream << "Error loading `" << "./light/config.json" << "!" << "\n"; delete entity; return; }
			this->addChild(*entity);
			entity->initialize();

			pod::Transform<>& parent = this->getComponent<pod::Transform<>>();
			pod::Transform<>& transform = entity->getComponent<pod::Transform<>>();
			entity->getComponent<uf::Camera>().getTransform().reference = &transform;

			transform = uf::transform::initialize( transform );
			transform.position = parent.position;
			uf::transform::rotate( transform, transform.right, 1.5708 * 3 );
			entity->getComponent<uf::Camera>().setFov(120);
			entity->getComponent<uf::Camera>().update(true);
			((ext::Light*)entity)->setColor( color );
		}
		for ( int i = 0; i < radius; ++i ) {
			uf::Entity* entity = new ext::Light;
			if ( !((ext::Object*) entity)->load("./light/config.json") ) { uf::iostream << "Error loading `" << "./light/config.json" << "!" << "\n"; delete entity; return; }
			this->addChild(*entity);
			entity->initialize();

			pod::Transform<>& parent = this->getComponent<pod::Transform<>>();
			pod::Transform<>& transform = entity->getComponent<pod::Transform<>>();//entity->getComponent<uf::Camera>().getTransform();
			entity->getComponent<uf::Camera>().getTransform().reference = &transform;

			transform = uf::transform::initialize( transform );
			transform.position = parent.position;
			uf::transform::rotate( transform, transform.up, (360.0 / radius) * (3.1415926/180.0) * i );
			entity->getComponent<uf::Camera>().update(true);
			((ext::Light*)entity)->setColor( color );
		}
	}

	generator.initialize(size);
	generator.generate();
	/* Collider */ {
		pod::Transform<>& transform = this->getComponent<pod::Transform<>>();
		uf::CollisionBody& collider = this->getComponent<uf::CollisionBody>();

		auto*** voxels = generator.getVoxels();
		for ( uint x = 0; x < size.x; ++x ) {
		for ( uint y = 0; y < size.y; ++y ) {
		for ( uint z = 0; z < size.z; ++z ) {
				pod::Vector3 offset = transform.position;
				offset.x += x - (size.x / 2.0f);
				offset.y += y - (size.y / 2.0f);
				offset.z += z - (size.z / 2.0f);

				ext::TerrainVoxel voxel = ext::TerrainVoxel::atlas(voxels[x][y][z]);
				if ( !voxel.opaque() ) continue;

				uf::Collider* box = new uf::AABBox( offset, {0.5, 0.5, 0.5} );
			//	uf::Collider* box = new uf::SphereCollider( 0.5f, offset );
				collider.add(box);
			}
		}
		}
	}

//	generator.rasterize(mesh, *this);
}
void ext::Region::render() {
	if ( !this->m_parent ) return;
	ext::World& parent = this->getRootParent<ext::World>();

	pod::Transform<>& transform = this->getComponent<pod::Transform<>>();
	uf::Shader& shader = this->getParent<ext::Terrain>().getComponent<uf::Shader>();
	uf::Camera& camera = parent.getCamera(); //parent.getPlayer().getComponent<uf::Camera>();
	uf::Mesh& mesh = this->getComponent<uf::Mesh>();

	uf::Serializer& metadata = this->getComponent<uf::Serializer>();
	const uf::Serializer& pMetadata = parent.getComponent<uf::Serializer>();
/*
	if ( pMetadata["state"].asInt() == 1 && metadata["light"]["rendered"] != Json::nullValue ) {
		const ext::Player& player = parent.getPlayer();
		const pod::Transform<>& pTransform = player.getComponent<pod::Transform<>>();
		pod::Vector3ui size = {
			metadata["region"]["size"][0].asUInt(),
			metadata["region"]["size"][1].asUInt(),
			metadata["region"]["size"][2].asUInt(),
		};
		pod::Vector3 pointf = uf::vector::divide(pTransform.position, {size.x, size.y, size.z});
		pod::Vector3i point = {
			(int) (pointf.x + (pointf.x > 0 ? 0.5 : -0.5)),
			(int) (pointf.y + (pointf.y > 0 ? 0.5 : -0.5)),
			(int) (pointf.z + (pointf.z > 0 ? 0.5 : -0.5)),
		};
		if ( point.y != transform.position.y / size.y ) return;
	}
	metadata["light"]["rendered"] = true;
*/

	uf::Entity::render();

	int slot = 0; 
	if ( this->getParent<ext::Terrain>().hasComponent<uf::Texture>() ) {
		uf::Texture& texture = this->getParent<ext::Terrain>().getComponent<uf::Texture>();
		texture.bind(slot);
	}
	struct {
		int diffuse = 0; int specular = 1; int normal = 2;
	} slots;
	if ( this->getParent<ext::Terrain>().hasComponent<uf::Material>() ) {
		uf::Material& material = this->getParent<ext::Terrain>().getComponent<uf::Material>();
		material.bind(slots.diffuse, slots.specular, slots.normal);
	}
	shader.bind(); {
		camera.update();
		shader.push("model", uf::transform::model(transform));
		shader.push("view", camera.getView());
		shader.push("projection", camera.getProjection());
		if ( this->getParent<ext::Terrain>().hasComponent<uf::Texture>() ) shader.push("texture", slot);
		if ( this->getParent<ext::Terrain>().hasComponent<uf::Material>() ) {
			if ( slots.diffuse >= 0 ) shader.push("diffuse", slots.diffuse);
			if ( slots.specular >= 0 ) shader.push("specular", slots.specular);
			if ( slots.normal >= 0 ) shader.push("normal", slots.normal);
		}
	}
	mesh.render();
}