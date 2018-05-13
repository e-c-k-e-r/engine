#include "world.h"
#include <uf/utils/time/time.h>
#include <uf/utils/io/iostream.h>
#include <uf/utils/math/vector.h>
#include <uf/utils/math/transform.h>
#include <uf/utils/window/window.h>
#include <uf/gl/gbuffer/gbuffer.h>
#include "../ext.h"
#include "./light/light.h"
#include "./gui/gui.h"

#include <uf/utils/audio/audio.h>

namespace {
	uf::Camera* camera;
	uf::GeometryBuffer light;
}

void ext::World::initialize() {
	this->m_name = "World";
	
	this->load();
}

void ext::World::tick() {
	static bool first = true; if ( first ) { first = false;
		uf::physics::tick();
	}

	uf::Entity::tick();

	{
		static float x = 1.07986, y = 24.7805;
		if ( uf::Window::isKeyPressed("L") ) x += 0.01;
		if ( uf::Window::isKeyPressed("J") ) x -= 0.01;
		if ( uf::Window::isKeyPressed("I") ) y += 0.01;
		if ( uf::Window::isKeyPressed("K") ) y -= 0.01;
		if ( uf::Window::isKeyPressed("O") ) std::cout << x << ", " << y << std::endl;
		glPolygonOffset(x, y);
	}
	if (uf::Window::isKeyPressed("U")) {
		std::function<void(const uf::Entity*, int)> recurse = [&]( const uf::Entity* parent, int indent ) {
			for ( const uf::Entity* entity : parent->getChildren() ) {
				for ( int i = 0; i < indent; ++i ) std::cout<<"\t";
				std::cout<<entity->getName()<<std::endl;
				recurse(entity, indent + 1);
			}
		}; recurse(this, 0);
	}
	{
		ext::Player& player = this->getPlayer();
		pod::Transform<>& transform = player.getComponent<pod::Transform<>>();
		
		ext::oal.listener( "POSITION", { transform.position.x, transform.position.y, transform.position.z } );
		ext::oal.listener( "VELOCITY", { 0, 0, 0 } );
		ext::oal.listener( "ORIENTATION", { 0, 0, 1, 1, 0, 0 } );

	}
}


uf::Camera& ext::World::getCamera() {
	return *::camera;
}

void ext::World::render() {
	uf::GeometryBuffer& buffer = this->getComponent<uf::GeometryBuffer>();
	uf::Serializer& metadata = this->getComponent<uf::Serializer>();

	{
		::camera = this->getPlayer().getComponentPointer<uf::Camera>();
	}

	/* Prepare Geometry Buffer */ {
		buffer.bind();
		glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
	}
	/* Render entities (normal pass) */ {
		metadata["state"] = 0;
		uf::Entity::render();
	}
	
	/* Prepare deferred pass */ {
		metadata["state"] = 1;

		/* */ {
			::light.bind();
			glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
		}

		buffer.unbind();
		glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
	}

	std::vector<ext::Light*> lights;
	static int renderedState = 0;
	/* Light pass */ {
		std::function<void(uf::Entity*, int)> recurse = [&]( uf::Entity* parent, int indent ) {
			for ( uf::Entity* entity : parent->getChildren() ) {
				if ( entity->getName() == "Light" ) lights.push_back( (ext::Light*) entity );
				recurse(entity, indent + 1);
			}
		};
		recurse(this, 0);

		for ( ext::Light* entity : lights ) {
			ext::Light& light = *entity;
			uf::Serializer& metadata = light.getComponent<uf::Serializer>();
			uf::GeometryBuffer& lightBuffer = metadata["light"]["dedicated"].asBool() ? light.getComponent<uf::GeometryBuffer>() : ::light;
			uf::Camera& lightCam = light.getComponent<uf::Camera>();

			lightCam.updateView();

			if ( !light.hasComponent<uf::GeometryBuffer>() || metadata["light"]["render_state"].asInt() == 0 ){
				lightBuffer.bind();
				glClear(GL_DEPTH_BUFFER_BIT);
				::camera = light.getComponentPointer<uf::Camera>();
				glViewport( 0, 0, ::camera->getSize().x, ::camera->getSize().y );
				glEnable(GL_POLYGON_OFFSET_FILL);
				glDisable(GL_CULL_FACE);
				uf::Entity::render();
				glDisable(GL_POLYGON_OFFSET_FILL);
				glEnable(GL_CULL_FACE);
				::camera = this->getPlayer().getComponentPointer<uf::Camera>();
				glViewport( 0, 0, ::camera->getSize().x, ::camera->getSize().y );
			}
			if ( (metadata["light"]["render_state"]=metadata["light"]["render_state"].asInt()+1).asInt()-1 >= metadata["light"]["rate"].asInt() ) metadata["light"]["render_state"]= 0;
			{
				::light.bind();

				if ( metadata["light"]["blend"] != Json::nullValue ) {
					glEnable(GL_BLEND);
					GLenum parameters[4] = {
						GL_ONE,
						GL_ONE,
						GL_ONE,
						GL_ONE,
					};
					for ( int i = 0; i < metadata["light"]["blend"].size(); ++i ) {
						if ( metadata["light"]["blend"][i] == "ZERO" || metadata["light"]["blend"][i] == "GL_ZERO" ) parameters[i] = GL_ZERO;
						if ( metadata["light"]["blend"][i] == "ONE" || metadata["light"]["blend"][i] == "GL_ONE" ) parameters[i] = GL_ONE;
						if ( metadata["light"]["blend"][i] == "SRC_COLOR" || metadata["light"]["blend"][i] == "GL_SRC_COLOR" ) parameters[i] = GL_SRC_COLOR;
						if ( metadata["light"]["blend"][i] == "ONE_MINUS_SRC_COLOR" || metadata["light"]["blend"][i] == "GL_ONE_MINUS_SRC_COLOR" ) parameters[i] = GL_ONE_MINUS_SRC_COLOR;
						if ( metadata["light"]["blend"][i] == "DST_COLOR" || metadata["light"]["blend"][i] == "GL_DST_COLOR" ) parameters[i] = GL_DST_COLOR;
						if ( metadata["light"]["blend"][i] == "ONE_MINUS_DST_COLOR" || metadata["light"]["blend"][i] == "GL_ONE_MINUS_DST_COLOR" ) parameters[i] = GL_ONE_MINUS_DST_COLOR;
						if ( metadata["light"]["blend"][i] == "SRC_ALPHA" || metadata["light"]["blend"][i] == "GL_SRC_ALPHA" ) parameters[i] = GL_SRC_ALPHA;
						if ( metadata["light"]["blend"][i] == "ONE_MINUS_SRC_ALPHA" || metadata["light"]["blend"][i] == "GL_ONE_MINUS_SRC_ALPHA" ) parameters[i] = GL_ONE_MINUS_SRC_ALPHA;
						if ( metadata["light"]["blend"][i] == "DST_ALPHA" || metadata["light"]["blend"][i] == "GL_DST_ALPHA" ) parameters[i] = GL_DST_ALPHA;
						if ( metadata["light"]["blend"][i] == "ONE_MINUS_DST_ALPHA" || metadata["light"]["blend"][i] == "GL_ONE_MINUS_DST_ALPHA" ) parameters[i] = GL_ONE_MINUS_DST_ALPHA;
						if ( metadata["light"]["blend"][i] == "CONSTANT_COLOR" || metadata["light"]["blend"][i] == "GL_CONSTANT_COLOR" ) parameters[i] = GL_CONSTANT_COLOR;
						if ( metadata["light"]["blend"][i] == "ONE_MINUS_CONSTANT_COLOR" || metadata["light"]["blend"][i] == "GL_ONE_MINUS_CONSTANT_COLOR" ) parameters[i] = GL_ONE_MINUS_CONSTANT_COLOR;
						if ( metadata["light"]["blend"][i] == "CONSTANT_ALPHA" || metadata["light"]["blend"][i] == "GL_CONSTANT_ALPHA" ) parameters[i] = GL_CONSTANT_ALPHA;
						if ( metadata["light"]["blend"][i] == "ONE_MINUS_CONSTANT_ALPHA" || metadata["light"]["blend"][i] == "GL_ONE_MINUS_CONSTANT_ALPHA" ) parameters[i] = GL_ONE_MINUS_CONSTANT_ALPHA;
						if ( metadata["light"]["blend"][i] == "SRC_ALPHA_SATURATE" || metadata["light"]["blend"][i] == "GL_SRC_ALPHA_SATURATE" ) parameters[i] = GL_SRC_ALPHA_SATURATE;
						if ( metadata["light"]["blend"][i] == "SRC1_COLOR" || metadata["light"]["blend"][i] == "GL_SRC1_COLOR" ) parameters[i] = GL_SRC1_COLOR;
						if ( metadata["light"]["blend"][i] == "ONE_MINUS_SRC_COLOR" || metadata["light"]["blend"][i] == "GL_ONE_MINUS_SRC_COLOR" ) parameters[i] = GL_ONE_MINUS_SRC_COLOR;
						if ( metadata["light"]["blend"][i] == "SRC1_ALPHA" || metadata["light"]["blend"][i] == "GL_SRC1_ALPHA" ) parameters[i] = GL_SRC1_ALPHA;
						if ( metadata["light"]["blend"][i] == "ONE_MINUS_SRC_ALPHA" || metadata["light"]["blend"][i] == "GL_ONE_MINUS_SRC_ALPHA" ) parameters[i] = GL_ONE_MINUS_SRC_ALPHA;
					}
				
					if ( metadata["light"]["blend"].size() == 2 ) {
						glBlendEquation(GL_FUNC_ADD);
						glBlendFunc(parameters[0], parameters[1]);
					} else if ( metadata["light"]["blend"].size() == 4 ) {
						glBlendEquationSeparate(GL_FUNC_ADD, GL_FUNC_ADD);
						glBlendFuncSeparate(parameters[0], parameters[1], parameters[2], parameters[3]);
					}
				}

				uf::Shader& shader = light.getComponent<uf::Shader>();
				uf::Mesh& mesh = buffer.getComponent<uf::Mesh>();
				pod::Transform<>& lightTransform = light.getComponent<pod::Transform<>>();

				shader.bind(); { int i = 0; for ( auto& texture : buffer.getBuffers() ) {
						texture.bind(i);
						shader.push("buffers.geom_"+texture.getName(), i++);
					} for ( auto& texture : lightBuffer.getBuffers() ) {
						texture.bind(i);
						shader.push("buffers.light_"+texture.getName(), i++);
					}

					shader.push("matrices.view", this->getPlayer().getComponent<uf::Camera>().getView());
					shader.push("matrices.projection", this->getPlayer().getComponent<uf::Camera>().getProjection());
					shader.push("matrices.projectionInverse", uf::matrix::inverse(this->getPlayer().getComponent<uf::Camera>().getProjection()));

					shader.push("parameters.color", light.getColor());
					shader.push("parameters.attenuation", light.getAttenuation());
					shader.push("parameters.power", light.getPower());
					shader.push("parameters.specular", light.getSpecular());
					shader.push("parameters.position", lightTransform.position);

					shader.push("parameters.view", lightCam.getView());
					shader.push("parameters.projection", lightCam.getProjection());
				}
				mesh.render();
				glDisable(GL_BLEND);
			}
		}
		{
			buffer.unbind();
			uf::Shader& shader = buffer.getComponent<uf::Shader>();
			uf::Camera& camera = this->getCamera();
			uf::Mesh& mesh = buffer.getComponent<uf::Mesh>();
			shader.bind(); { int i = 0; for ( auto& texture : buffer.getBuffers() ) {
					texture.bind(i);
					shader.push("buffers.geom_"+texture.getName(), i++);
				} for ( auto& texture : ::light.getBuffers() ) {
					texture.bind(i);
					shader.push("buffers.light_"+texture.getName(), i++);
				}

				shader.push("matrices.view", camera.getView());
				shader.push("matrices.projection", camera.getProjection());
				shader.push("matrices.projectionInverse", uf::matrix::inverse(this->getPlayer().getComponent<uf::Camera>().getProjection()));

				shader.push("parameters.light", !lights.empty());
			}
			mesh.render();
		}
	}
	/* GUI pass */  {
		metadata["state"] = 2;
		glEnable(GL_BLEND);
		glDisable(GL_DEPTH_TEST);
		glBlendEquation(GL_FUNC_ADD);
		glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
		for ( uf::Entity* entity : this->getChildren() ) if ( entity->getName() == "Gui Manager" ) entity->render();
		glDisable(GL_BLEND);
		glEnable(GL_DEPTH_TEST);
	}
	buffer.unbind();
}

ext::Player& ext::World::getPlayer() {
	std::function<uf::Entity*(uf::Entity*, int)> recurse = [&]( uf::Entity* parent, int indent ) {
		for ( uf::Entity* entity : parent->getChildren() ) {
			if ( entity->getName() == "Player" ) return entity;
			uf::Entity* p = recurse(entity, indent + 1);
			if ( p ) return p;
		}
		return (uf::Entity*) NULL;
	};
	uf::Entity* pointer = recurse(this, 0);
	return *((ext::Player*) pointer);
}
const ext::Player& ext::World::getPlayer() const {
	std::function<const uf::Entity*(const uf::Entity*, int)> recurse = [&]( const uf::Entity* parent, int indent ) {
		for ( const uf::Entity* entity : parent->getChildren() ) {
			if ( entity->getName() == "Player" ) return entity;
			const uf::Entity* p = recurse(entity, indent + 1);
			if ( p ) return p;
		}
		return (const uf::Entity*) NULL;
	};
	const uf::Entity* pointer = recurse(this, 0); return
	*((const ext::Player*) pointer);
}

bool ext::World::load() {
	uf::Serializer& metadata = this->getComponent<uf::Serializer>();
	struct { uf::Serializer base; uf::Serializer shader; } config;
	/* Load entity file */ {
		struct { bool exists = false; std::string filename = "./entities/world/config.json"; } file;
		/* Read from file */ if ( !(file.exists = config.base.readFromFile(file.filename)) ) { uf::iostream << "Error loading `" << file.filename << "!" << "\n"; return false; }
		metadata = config.base;
		for ( uint i = 0; i < config.base["entities"].size(); ++i ) {
			std::string json = config.base["entities"][i].asString();
			std::string type = ""; {
				std::string root = "./entities/" + uf::string::directory(json);
				struct { uf::Serializer base; } config;
				/* Load entity file */ {
					struct { bool exists = false; std::string filename; } file;
					file.filename = root + uf::string::filename(json);
					/* Read from file */ if ( !(file.exists = config.base.readFromFile(file.filename)) ) { uf::iostream << "Error loading `" << file.filename << "!" << "\n"; return false; }
					type = config.base["type"].asString();
				}
				if ( config.base["ignore"].asBool() ) continue;
			}
			uf::Entity* entity;
			if ( type == "Terrain" ) entity = new ext::Terrain;
			else if ( type == "Player" ) entity = new ext::Player;
			else if ( type == "Craeture" ) entity = new ext::Craeture;
			else if ( type == "Light" ) entity = new ext::Light;
			else if ( type == "Gui" ) entity = new ext::Gui;
			else entity = new ext::Object;

			if ( !((ext::Object*) entity)->load(json) ) { uf::iostream << "Error loading `" << json << "!" << "\n"; delete entity; return false; }
			this->addChild(*entity); entity->initialize();
		}
		/* Geometry Buffer */ {
			/* Generate default light shadowmap */ {
				uf::GeometryBuffer& buffer = ::light;
				uf::hooks.addHook( "window:Resized", [&](const std::string& event)->std::string{
					uf::Serializer json = event;

					// Update persistent window sized (size stored to JSON file)
					pod::Vector2ui size; {
						size.x = json["window"]["size"]["x"].asUInt64();
						size.y = json["window"]["size"]["y"].asUInt64();
					}
					size *= metadata["buffer"]["scale"].asDouble();
					if ( size.x == buffer.getSize().x && size.y == buffer.getSize().y ) return "false";

					auto& buffers = buffer.getBuffers();
					buffers.clear();
					uint i = 0;
					buffers.push_back( spec::ogl::GeometryTexture() );
					buffers.push_back( spec::ogl::GeometryTexture() );
					spec::ogl::GeometryTexture& light = buffers[i++];
					spec::ogl::GeometryTexture& depth = buffers[i++];
					light.setSize(size); light.setName("light"); light.setType({ GL_COLOR_ATTACHMENT0, GL_FLOAT, GL_RGBA, GL_RGBA16F });
					depth.setSize(size); depth.setName("depth"); depth.setType({ GL_DEPTH_ATTACHMENT, GL_FLOAT, GL_DEPTH_COMPONENT, GL_DEPTH_COMPONENT });
					buffer.setSize(size);
					buffer.generate();
					return "true";
				} );
			}
			uf::GeometryBuffer& buffer = this->getComponent<uf::GeometryBuffer>(); {
				uf::hooks.addHook( "window:Resized", [&](const std::string& event)->std::string{
					uf::Serializer json = event;

					// Update persistent window sized (size stored to JSON file)
					pod::Vector2ui size; {
						size.x = json["window"]["size"]["x"].asUInt64();
						size.y = json["window"]["size"]["y"].asUInt64();
					}
					size *= metadata["buffer"]["scale"].asDouble();
					if ( size.x == buffer.getSize().x && size.y == buffer.getSize().y ) return "false";

					auto& buffers = buffer.getBuffers();
					buffers.clear();
					uint i = 0;
					buffers.push_back( spec::ogl::GeometryTexture() );
					buffers.push_back( spec::ogl::GeometryTexture() );
					buffers.push_back( spec::ogl::GeometryTexture() );
			//		buffers.push_back( spec::ogl::GeometryTexture() );

					spec::ogl::GeometryTexture& color = buffers[i++];
					spec::ogl::GeometryTexture& normal = buffers[i++];
			//		spec::ogl::GeometryTexture& position = buffers[i++];
					spec::ogl::GeometryTexture& depth = buffers[i++];

					color.setSize(size); color.setName("color"); color.setType({ GL_COLOR_ATTACHMENT0, GL_FLOAT, GL_RGBA, GL_RGBA16F });
					normal.setSize(size); normal.setName("normal"); normal.setType({ GL_COLOR_ATTACHMENT1, GL_FLOAT, GL_RGB, GL_RGB16F });
			//		position.setSize(size); position.setName("position"); position.setType({ GL_COLOR_ATTACHMENT2, GL_FLOAT, GL_RGB, GL_RGB16F });
					depth.setSize(size); depth.setName("depth"); depth.setType({ GL_DEPTH_ATTACHMENT, GL_FLOAT, GL_DEPTH_COMPONENT, GL_DEPTH_COMPONENT });

					buffer.setSize(size);
					buffer.generate();
					return "true";
				} );
			}

			uf::Shader& shader = buffer.getComponent<uf::Shader>();
			/* Shader */ if ( config.base["shader"] != Json::nullValue ) {
				struct {
					bool exists = false;
					std::string directory;
					std::string localFilename;
					std::string filename;
				} file;
				std::string root = "./entities/world/";
				file.directory = root + uf::string::directory(config.base["shader"].asString());
				file.localFilename = uf::string::filename(config.base["shader"].asString());
				file.filename = file.directory + file.localFilename;
				 file.exists = config.shader.readFromFile(file.filename);
				/* Read from file */ if ( !file.exists ) { uf::iostream << "Error loading `" << file.filename << "!" << "\n"; return false; }
				/* Load shaders */ {
					bool fromFile = config.shader["source"] == "file";
					// Vertex
					std::string vertex = file.directory + config.shader["shaders"]["vertex"].asString();
					shader.add( uf::Shader::Component( vertex, GL_VERTEX_SHADER, fromFile ) );
					// Fragment
					std::string fragment = file.directory + config.shader["shaders"]["fragment"].asString();
					shader.add( uf::Shader::Component( fragment, GL_FRAGMENT_SHADER, fromFile ) );
				}

				shader.compile();
				shader.link();
				for ( std::size_t i = 0; i < config.shader["attributes"].size(); ++i ) shader.bindAttribute( i, config.shader["attributes"][(int) i].asString() );
				for ( std::size_t i = 0; i < config.shader["fragmentData"].size(); ++i ) shader.bindFragmentData( i, config.shader["fragmentData"][(int) i].asString() );
			}
		}
	}


	return true;
}