#include "gui.h"

#include <uf/utils/hook/hook.h>
#include <uf/utils/time/time.h>
#include <uf/utils/serialize/serializer.h>
#include <uf/utils/userdata/userdata.h>
#include <uf/utils/window/window.h>
#include <uf/gl/camera/camera.h>
#include "../world.h"

void ext::Gui::initialize() {
	uf::Serializer& metadata = this->getComponent<uf::Serializer>();

	if ( this->m_name == "Gui Manager" )
		for ( uint i = 0; i < metadata["gui"]["list"].size(); ++i ) {
			std::string config = metadata["gui"]["list"][i].asString();
			uf::Entity* entity = new ext::Gui;
			ext::Gui& gui = *((ext::Gui*) entity);
			if ( !gui.load(config) ) { uf::iostream << "Error loading `" << config << "!" << "\n"; delete entity; continue; }
			this->addChild(*entity);
			entity->initialize();
		}
	else {
		if ( metadata["gui"]["position"] != Json::nullValue ) {
			pod::Transform<>& transform = this->getComponent<pod::Transform<>>();
			transform.position.x = metadata["gui"]["position"][0].asDouble();
			transform.position.y = metadata["gui"]["position"][1].asDouble();
			transform.position.z = metadata["gui"]["position"][2].asDouble();
		}

		uf::hooks.addHook( "window:Resized", [&](const std::string& event)->std::string{
			uf::Serializer json = event;

			// Update persistent window sized (size stored to JSON file)
			pod::Vector2ui size; {
				size.x = json["window"]["size"]["x"].asUInt64();
				size.y = json["window"]["size"]["y"].asUInt64();
			}
			uf::Texture& texture = this->getComponent<uf::Texture>();
			pod::Transform<>& transform = this->getComponent<pod::Transform<>>();

			if ( metadata["gui"]["scaling"].asString() == "relative" ) {
				transform.scale = pod::Vector3{ (double) texture.getImage().getDimensions().x / size.x, (double) texture.getImage().getDimensions().y / size.y, 1 };
			} else if ( metadata["gui"]["scaling"].asString() == "fixed" ) {
				transform.scale = pod::Vector3{ (double) texture.getImage().getDimensions().x / 1920, (double) texture.getImage().getDimensions().y / 1080, 1 };
			}

			return "true";
		} );
	}
}
void ext::Gui::tick() {

}
void ext::Gui::render() {
	ext::World& world = this->getRootParent<ext::World>();
	ext::Gui& parent = this->getParent<ext::Gui>();
	uf::Serializer& gamestate = world.getComponent<uf::Serializer>();
	uf::Serializer& metadata = this->getComponent<uf::Serializer>();
	if ( gamestate["state"].asInt() != 2 ) return;

	uf::Entity::render();

	if ( this->hasComponent<uf::Texture>() ) {
		uf::GeometryBuffer& buffer = world.getComponent<uf::GeometryBuffer>();
		buffer.unbind();
		
		uf::Shader& shader = this->hasComponent<uf::Shader>() ? this->getComponent<uf::Shader>() : parent.getComponent<uf::Shader>();
		uf::Texture& texture = this->getComponent<uf::Texture>();
		pod::Transform<>& transform = this->getComponent<pod::Transform<>>();
		ext::Player& player = world.getPlayer();
		uf::Camera& camera = player.getComponent<uf::Camera>();

		pod::Vector4 offset = { metadata["gui"]["uv"][0].asDouble(), metadata["gui"]["uv"][1].asDouble(), metadata["gui"]["uv"][2].asDouble(), metadata["gui"]["uv"][3].asDouble() };
		pod::Matrix4 matrix = uf::transform::model(transform);
		if ( metadata["gui"]["world"].asBool() ) {
			pod::Matrix4 rotation = uf::quaternion::matrix( uf::vector::multiply( { 1, 1, 1, -1 }, player.getComponent<pod::Transform<>>().orientation) );
			matrix = matrix * rotation;
		//	matrix = camera.getProjection() * camera.getView() * matrix;
		}

		shader.bind(); { int i = 0; for ( auto& texture : buffer.getBuffers() ) {
				texture.bind(i);
				shader.push("buffer_"+texture.getName(), i++);
			}
			texture.bind(i);
			shader.push("gui_element", i++);
			shader.push("offset",  offset);
			shader.push("model", matrix);
			if ( metadata["gui"]["world"].asBool() ) {
				shader.push("projection", camera.getProjection());
				shader.push("view", camera.getView());
				shader.push("model", matrix);
			}
		}
		
		uf::Mesh& mesh = buffer.getComponent<uf::Mesh>();
		mesh.render();
	}
}