#include "region.h"

#include "../../ext.h"
#include "generator.h"
#include "../light/light.h"

void ext::Region::initialize() {	
	this->addComponent<pod::Physics>();
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
	generator.initialize(size);

	float r = (rand() % 100) / 100.0;
	bool addLight = r > 0.95;
	if ( metadata["region"]["location"][0].asInt() == 0 && metadata["region"]["location"][1].asInt() == 0 && metadata["region"]["location"][2].asInt() == 0 ) addLight = true;
//	if ( metadata["region"]["location"][0].asInt() % 2 != 0 || metadata["region"]["location"][2].asInt() % 2 != 0 ) addLight = false;
	static bool first = false; if ( !first ) first = addLight = true; else addLight = false;
//	addLight = false;
	if ( addLight ) {
	//	std::cout << metadata["region"]["location"][0] << ", " << metadata["region"]["location"][1] << ", " << metadata["region"]["location"][2] << std::endl;
		pod::Vector3 color = { 1.0, 1.0, 1.0 }; {
			float r = (rand() % 256) / 256.0; color.x -= r;
			float g = (rand() % 256) / 256.0; color.y -= g;
			float b = (rand() % 256) / 256.0; color.z -= b;

			color = uf::vector::normalize( color  );
		}
		int radius = 3;
		for ( int i = 0; i < radius; ++i ) {
			uf::Entity* entity = new ext::Light;
			if ( !((ext::Object*) entity)->load("./light/config.json") ) { uf::iostream << "Error loading `" << "./light/config.json" << "!" << "\n"; delete entity; return; }
			this->addChild(*entity);
			entity->initialize();

			pod::Transform<>& parent = this->getComponent<pod::Transform<>>();
			pod::Transform<>& transform = entity->getComponent<uf::Camera>().getTransform();

			transform = uf::transform::initialize( transform );
			transform.position = parent.position;
			transform.position.y += size.y / 3 * 2;
			uf::transform::rotate( transform, transform.up, (360.0 / radius) * (3.1415926/180.0) * i );
			entity->getComponent<uf::Camera>().update(true);
			((ext::Light*)entity)->setColor( color );
		}
		/* Up */ if ( false ) {
			uf::Entity* entity = new ext::Light;
			if ( !((ext::Object*) entity)->load("./light/config.json") ) { uf::iostream << "Error loading `" << "./light/config.json" << "!" << "\n"; delete entity; return; }
			this->addChild(*entity);
			entity->initialize();

			pod::Transform<>& parent = this->getComponent<pod::Transform<>>();
			pod::Transform<>& transform = entity->getComponent<uf::Camera>().getTransform();

			transform = uf::transform::initialize( transform );
			transform.position = parent.position;
			transform.position.y += size.y / 3 * 2;
			uf::transform::rotate( transform, transform.right, 1.5708 * 1 );
			entity->getComponent<uf::Camera>().setFov(120);
			entity->getComponent<uf::Camera>().update(true);
			((ext::Light*)entity)->setColor( color );
		}
		/* Down */ if ( false ) {
			uf::Entity* entity = new ext::Light;
			if ( !((ext::Object*) entity)->load("./light/config.json") ) { uf::iostream << "Error loading `" << "./light/config.json" << "!" << "\n"; delete entity; return; }
			this->addChild(*entity);
			entity->initialize();

			pod::Transform<>& parent = this->getComponent<pod::Transform<>>();
			pod::Transform<>& transform = entity->getComponent<uf::Camera>().getTransform();

			transform = uf::transform::initialize( transform );
			transform.position = parent.position;
			transform.position.y += size.y / 3 * 2;
			uf::transform::rotate( transform, transform.right, 1.5708 * 3 );
			entity->getComponent<uf::Camera>().setFov(120);
			entity->getComponent<uf::Camera>().update(true);
			((ext::Light*)entity)->setColor( color );
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