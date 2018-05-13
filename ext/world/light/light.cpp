#include "light.h"

#include <uf/utils/hook/hook.h>
#include <uf/utils/time/time.h>
#include <uf/utils/serialize/serializer.h>
#include <uf/utils/userdata/userdata.h>
#include <uf/utils/window/window.h>
#include <uf/gl/camera/camera.h>
#include "../world.h"

const pod::Vector3& ext::Light::getColor() const {
	return this->m_color;
}
void ext::Light::setColor( const pod::Vector3& color ) {
	this->m_color = color;
}
const pod::Vector3& ext::Light::getSpecular() const {
	return this->m_specular;
}
void ext::Light::setSpecular( const pod::Vector3& specular ) {
	this->m_specular = specular;
}
float ext::Light::getAttenuation() const {
	return this->m_attenuation;
}
void ext::Light::setAttenuation( float attenuation ) {
	this->m_attenuation = attenuation;
}
float ext::Light::getPower() const {
	return this->m_power;
}
void ext::Light::setPower( float power ) {
	this->m_power = power;
}

void ext::Light::initialize() {
	this->addComponent<uf::Camera>(); {
		pod::Transform<>& transform = this->getComponent<pod::Transform<>>();
		transform = uf::transform::initialize(transform);
		this->getComponent<uf::Camera>().getTransform().reference = this->getComponentPointer<pod::Transform<>>();
	}

	/* Load Config */ {
		struct {
			int mode = 0;
			struct {
				pod::Vector2 lr, bt, nf;
			} ortho;
			struct {
				pod::Math::num_t fov = 120;
				pod::Vector2 size = {640, 480};
				pod::Vector2 bounds = {0.5, 128.0};
			} perspective;
			pod::Vector3 offset = {0, 0, 0};
		} settings;
		uf::Serializer& metadata = this->getComponent<uf::Serializer>();

		uf::Camera& camera = this->getComponent<uf::Camera>();
		settings.mode = metadata["camera"]["ortho"].asBool() ? -1 : 1;
		settings.perspective.size.x = metadata["camera"]["settings"]["size"]["x"].asDouble();
		settings.perspective.size.y = metadata["camera"]["settings"]["size"]["y"].asDouble();
		camera.setSize(settings.perspective.size);
		if ( settings.mode < 0 ) {
			settings.ortho.lr.x 	= metadata["camera"]["settings"]["left"].asDouble();
			settings.ortho.lr.y 	= metadata["camera"]["settings"]["right"].asDouble();
			settings.ortho.bt.x 	= metadata["camera"]["settings"]["bottom"].asDouble();
			settings.ortho.bt.y 	= metadata["camera"]["settings"]["top"].asDouble();
			settings.ortho.nf.x 	= metadata["camera"]["settings"]["near"].asDouble();
			settings.ortho.nf.y 	= metadata["camera"]["settings"]["far"].asDouble();

			camera.ortho( settings.ortho.lr, settings.ortho.bt, settings.ortho.nf );
		} else {
			settings.perspective.fov 		= metadata["camera"]["settings"]["fov"].asDouble();
			settings.perspective.bounds.x 	= metadata["camera"]["settings"]["clip"]["near"].asDouble();
			settings.perspective.bounds.y 	= metadata["camera"]["settings"]["clip"]["far"].asDouble();

			camera.setFov(settings.perspective.fov);
			camera.setBounds(settings.perspective.bounds);
		}

		settings.offset.x = metadata["camera"]["offset"][0].asDouble();
		settings.offset.y = metadata["camera"]["offset"][1].asDouble();
		settings.offset.z = metadata["camera"]["offset"][2].asDouble();

		pod::Transform<>& transform = camera.getTransform();
		/* Transform initialization */ {
			transform.position.x = metadata["camera"]["position"][0].asDouble();
			transform.position.y = metadata["camera"]["position"][1].asDouble();
			transform.position.z = metadata["camera"]["position"][2].asDouble();
		}

		camera.setOffset(settings.offset);
		camera.update(true);

		metadata["light"]["state"] = 0;

		/* Attenuation */ {
			if ( metadata["light"]["attenuation"] != Json::nullValue ) this->setAttenuation( metadata["light"]["attenuation"].asFloat() );
		}
		/* Power */ {
			if (  metadata["light"]["power"] != Json::nullValue ) this->setPower(metadata["light"]["power"].asFloat() );
		}
		/* Specular */ {
			if (  metadata["light"]["specular"] != Json::nullValue ) {
				pod::Vector3 specular;
				specular.x = metadata["light"]["specular"][0].asDouble();
				specular.y = metadata["light"]["specular"][1].asDouble();
				specular.z = metadata["light"]["specular"][2].asDouble();
				this->setSpecular(specular);
			}
		}
		
		if ( metadata["light"]["dedicated"].asBool() ) {
			uf::GeometryBuffer& buffer = this->getComponent<uf::GeometryBuffer>(); {
				if ( metadata["camera"]["settings"]["size"]["auto"].asBool() ) {
					uf::hooks.addHook( "window:Resized", [&](const std::string& event)->std::string{
						uf::Serializer json = event;

						// Update persistent window sized (size stored to JSON file)
						pod::Vector2ui size; {
							size.x = json["window"]["size"]["x"].asUInt64();
							size.y = json["window"]["size"]["y"].asUInt64();
						}
						size *= metadata["camera"]["settings"]["size"]["scale"].asDouble();
						
						metadata["light"]["state"] = 0;

						if ( size.x == buffer.getSize().x && size.y == buffer.getSize().y ) return "false";

						auto& buffers = buffer.getBuffers();
						buffers.clear();
						uint i = 0;
						buffers.push_back( spec::ogl::GeometryTexture() );
						spec::ogl::GeometryTexture& depth = buffers[i++];
						depth.setSize(size); depth.setName("depth"); depth.setType({ GL_DEPTH_ATTACHMENT, GL_FLOAT, GL_DEPTH_COMPONENT, GL_DEPTH_COMPONENT });
						buffer.setSize(size);
						buffer.generate();
						return "true";
					} );
				}
				pod::Vector2ui size = { settings.perspective.size.x, settings.perspective.size.y };
				auto& buffers = buffer.getBuffers();
				buffers.clear();
				uint i = 0;
				buffers.push_back( spec::ogl::GeometryTexture() );
				spec::ogl::GeometryTexture& depth = buffers[i++];
				depth.setSize(size); depth.setName("depth"); depth.setType({ GL_DEPTH_ATTACHMENT, GL_FLOAT, GL_DEPTH_COMPONENT, GL_DEPTH_COMPONENT });
				buffer.setSize(size);
				buffer.generate();
			}
		}
		if ( metadata["camera"]["settings"]["size"]["auto"].asBool() ) {
			uf::hooks.addHook( "window:Resized", [&](const std::string& event)->std::string{
				uf::Serializer json = event;

				// Update persistent window sized (size stored to JSON file)
				pod::Vector2ui size; {
					size.x = json["window"]["size"]["x"].asUInt64();
					size.y = json["window"]["size"]["y"].asUInt64();
				}
				size *= metadata["camera"]["settings"]["size"]["scale"].asDouble();
				camera.setSize({ size.x, size.y });
				camera.update(true);
				return "true";
			} );
		}
	}

	this->m_name = "Light";
}
void ext::Light::tick() {
	uf::Camera& camera = this->getComponent<uf::Camera>();
	camera.update(true);
}
void ext::Light::render() {
}