#include "object.h"

#include "../../ext.h"
#include <uf/ext/assimp/assimp.h>
#include <uf/utils/window/window.h>

void ext::Object::initialize() {	
	uf::Entity::initialize();
}
void ext::Object::destroy() {

	uf::Serializer& metadata = this->getComponent<uf::Serializer>();
	for( Json::Value::iterator it = metadata["hooks"].begin() ; it != metadata["hooks"].end() ; ++it ) {
	 	std::string name = it.key().asString();
		for ( uint i = 0; i < metadata["hooks"][name].size(); ++i ) {
			uint id = metadata["hooks"][name][i].asUInt();
			uf::hooks.removeHook(name, id);
		}
	}

	uf::Entity::destroy();
}
void ext::Object::tick() {
	uf::Entity::tick();
	pod::Transform<>& transform = this->getComponent<pod::Transform<>>();
	pod::Physics& physics = this->getComponent<pod::Physics>();
	transform = uf::physics::update( transform, physics );
}
void ext::Object::render() {
	if ( !this->m_parent ) return;
	uf::Entity::render();
	ext::World& parent = this->getRootParent<ext::World>();

	if ( this->hasComponent<uf::SkeletalMesh>() ) {	
		pod::Transform<>& transform = this->getComponent<pod::Transform<>>();
		uf::Shader& shader = this->getComponent<uf::Shader>();
		uf::Camera& camera = parent.getCamera(); //parent.getPlayer().getComponent<uf::Camera>();
		uf::SkeletalMesh& mesh = this->getComponent<uf::SkeletalMesh>();

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
			shader.push("model", uf::transform::model(transform));
			shader.push("view", camera.getView());
			shader.push("projection", camera.getProjection());
			if ( this->hasComponent<uf::Texture>() ) shader.push("texture", slot);
			if ( this->hasComponent<uf::Material>() ) {
				if ( slots.diffuse >= 0 ) shader.push("diffuse", slots.diffuse);
				if ( slots.specular >= 0 ) shader.push("specular", slots.specular);
				if ( slots.normal >= 0 ) shader.push("normal", slots.normal);
			}

			auto& bones = mesh.getSkeleton().getBones();
			if ( !bones.empty() ) {
				for ( const auto& bone : bones ) {
					std::string name = "bones_" + std::to_string(bone.index);
					shader.push(name, bone.matrix);
				}
				shader.push("debug", uf::Window::isKeyPressed("RControl"));
			}
		}
		mesh.render();
	} else if ( this->hasComponent<uf::Mesh>() ) {	
		pod::Transform<>& transform = this->getComponent<pod::Transform<>>();
		uf::Shader& shader = this->getComponent<uf::Shader>();
		uf::Camera& camera = parent.getCamera(); //parent.getPlayer().getComponent<uf::Camera>();
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
			shader.push("model", uf::transform::model(transform));
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
}
bool ext::Object::load( const std::string& json ) {
	std::string root = "./entities/" + uf::string::directory(json);
	struct {
		uf::Serializer base;
		uf::Serializer mesh;
		uf::Serializer shader;
		uf::Serializer texture;
		uf::Serializer metadata;
	} config;
	/* Load entity file */ {
		struct {
			bool exists = false;
			std::string filename;
		} file;
		file.filename = root + uf::string::filename(json);
		/* Read from file */ if ( !(file.exists = config.base.readFromFile(file.filename)) ) {
			uf::iostream << "Error loading `" << file.filename << "!" << "\n";
			return false;
		}

		// Set name
		this->m_name = config.base["name"].asString();
		// Set transform
		pod::Transform<>& transform = this->getComponent<pod::Transform<>>();
		transform.position = uf::vector::create( config.base["transform"]["position"][0].asDouble(),config.base["transform"]["position"][1].asDouble(),config.base["transform"]["position"][2].asDouble() );
		transform.orientation = uf::quaternion::identity();
		transform.orientation = uf::quaternion::axisAngle( { config.base["transform"]["rotation"]["axis"][0].asDouble(),config.base["transform"]["rotation"]["axis"][1].asDouble(),config.base["transform"]["rotation"]["axis"][2].asDouble() }, config.base["transform"]["rotation"]["angle"].asDouble() );
		transform.scale = uf::vector::create( config.base["transform"]["scale"][0].asDouble(),config.base["transform"]["scale"][1].asDouble(),config.base["transform"]["scale"][2].asDouble() );
		transform = uf::transform::reorient( transform );
	}

	/* Texture */ if ( config.base["texture"] != Json::nullValue ) {
		struct {
			bool exists = false;
			std::string directory;
			std::string localFilename;
			std::string filename;
		} file;
		file.directory = root + uf::string::directory(config.base["texture"].asString()) ;
		file.localFilename = uf::string::filename(config.base["texture"].asString());
		file.filename = file.directory + file.localFilename;
		file.exists = config.texture.readFromFile(file.filename);
		/* Read from file */ if ( !file.exists ) {
			uf::iostream << "Error loading `" << file.filename << "!" << "\n";
			return false;
		}
		if ( config.texture["source"] != Json::nullValue ) {				
			uf::Texture& texture = this->getComponent<uf::Texture>();
			texture.open(file.directory + config.texture["source"].asString());
			texture.generate();
		} else {
			uf::Material& material = this->getComponent<uf::Material>();
			if ( config.texture["diffuse"] != Json::nullValue ) {
				uf::Texture& texture = material.getDiffuse();
				texture.open(file.directory + config.texture["diffuse"].asString());
			}
			if ( config.texture["specular"] != Json::nullValue ) {
				uf::Texture& texture = material.getSpecular();
				texture.open(file.directory + config.texture["specular"].asString());
			}
			if ( config.texture["normal"] != Json::nullValue ) {
				uf::Texture& texture = material.getNormal();
				texture.open(file.directory + config.texture["normal"].asString());
			}
			material.generate();
		}
	}

	/* Shader */ if ( config.base["shader"] != Json::nullValue ) {
		uf::Shader& shader = this->getComponent<uf::Shader>();
		struct {
			bool exists = false;
			std::string directory;
			std::string localFilename;
			std::string filename;
		} file;
		file.directory = root + uf::string::directory(config.base["shader"].asString());
		file.localFilename = uf::string::filename(config.base["shader"].asString());
		file.filename = file.directory + file.localFilename;
		 file.exists = config.shader.readFromFile(file.filename);
		/* Read from file */ if ( !file.exists ) {
			uf::iostream << "Error loading `" << file.filename << "!" << "\n";
			return false;
		}
		/* Load shaders */ {
			bool fromFile = config.shader["source"] == "file";

			// Vertex
			std::string vertex = file.directory + config.shader["shaders"]["vertex"].asString();
			if ( fromFile && !uf::string::exists(vertex) ) {
			//	vertex = config.fallbacks.embedded["vertex"].asString();
				std::string filename = vertex;
				std::ofstream output;
				output.open(filename, std::ios::binary);
				output.write( vertex.c_str(), vertex.size() );
				output.close();
			}
			
			shader.add( uf::Shader::Component( vertex, GL_VERTEX_SHADER, fromFile ) );
			
			// Fragment
			std::string fragment = file.directory + config.shader["shaders"]["fragment"].asString();
			if ( fromFile && !uf::string::exists(fragment) ) {
			//	fragment = config.fallbacks.embedded["fragment"].asString();
				std::string filename = fragment;
				std::ofstream output;
				output.open(filename, std::ios::binary);
				output.write( fragment.c_str(), fragment.size() );
				output.close();
			}

			shader.add( uf::Shader::Component( fragment, GL_FRAGMENT_SHADER, fromFile ) );
		}

		shader.compile();
		shader.link();
		for ( std::size_t i = 0; i < config.shader["attributes"].size(); ++i ) {
			shader.bindAttribute( i, config.shader["attributes"][(int) i].asString() );
		}
		for ( std::size_t i = 0; i < config.shader["fragmentData"].size(); ++i ) {
			shader.bindFragmentData( i, config.shader["fragmentData"][(int) i].asString() );
		}
	}	
	/* Mesh */ if ( config.base["mesh"] != Json::nullValue ) {
		struct {
			bool exists = false;
			std::string directory;
			std::string localFilename;
			std::string filename;
		} file;
		file.directory = root + uf::string::directory(config.base["mesh"].asString());
		file.localFilename = uf::string::filename(config.base["mesh"].asString());
		file.filename = file.directory + file.localFilename;
		file.exists = config.mesh.readFromFile(file.filename);
		/* Read from file */ if ( !file.exists ) {
			uf::iostream << "Error loading `" << file.filename << "!" << "\n";
			return false;
		}
		if ( !config.mesh["mesh"]["skeletal"].asBool() ) {
			uf::Mesh& mesh = this->getComponent<uf::Mesh>();
			/* Load from file (if from file) */ if ( config.mesh["source"] == "file" ) {
				std::string position = file.directory + config.mesh["mesh"]["position"].asString();
				/* Parse JSON file */ if ( uf::string::extension(position) == "json" ) {
					/* Positions */ {
						uf::Serializer json;
						if ( !json.readFromFile(position) ) {}
						std::vector<float>& positions = mesh.getPositions().getVertices();
						for ( auto& f : json ) positions.push_back(f.asFloat());
					}
				} /* Parse OBJ file */ else if ( uf::string::extension(position) == "obj" ) {}
				
				std::string uv = file.directory + config.mesh["mesh"]["uv"].asString();
				/* Parse JSON file */ if ( uf::string::extension(uv) == "json" ) {
					/* UVs */ {
						uf::Serializer json;
						if ( !json.readFromFile(uv) ) {	}
						std::vector<float>& uvs = mesh.getUvs().getVertices();
						for ( auto& f : json ) uvs.push_back(f.asFloat());
					}
				} /* Parse OBJ file */ else if ( uf::string::extension(uv) == "obj" ) {}
				
				std::string normal = file.directory + config.mesh["mesh"]["normal"].asString();
				/* Parse JSON file */ if ( uf::string::extension(normal) == "json" ) {
					/* Normals */ {
						uf::Serializer json;
						if ( !json.readFromFile(normal) ) {}
						std::vector<float>& normals = mesh.getNormals().getVertices();
						for ( auto& f : json ) normals.push_back(f.asFloat());
					}
				} /* Parse OBJ file */ else if ( uf::string::extension(normal) == "obj" ) {}
				
				std::string color = file.directory + config.mesh["mesh"]["color"].asString();
				/* Parse JSON file */ if ( uf::string::extension(color) == "json" ) {
					/* Colors */ {
						uf::Serializer json;
						if ( !json.readFromFile(color) ) {}
						std::vector<float>& colors = mesh.getColors().getVertices();
						for ( auto& f : json ) colors.push_back(f.asFloat());
					}
				} /* Parse OBJ file */ else if ( uf::string::extension(color) == "obj" ) {}
			/* Load from JSON (if embedded) */ } else if ( config.mesh["source"] == "embedded" ) {
				std::vector<float>& position 		= mesh.getPositions().getVertices();
				std::vector<float>& normal 			= mesh.getNormals().getVertices();
				std::vector<float>& color 			= mesh.getColors().getVertices();
				std::vector<float>& uv 				= mesh.getUvs().getVertices();
				
				position.reserve(config.mesh["mesh"]["position"].size());
				normal.reserve(config.mesh["mesh"]["normal"].size());
				color.reserve(config.mesh["mesh"]["color"].size());
				uv.reserve(config.mesh["mesh"]["uv"].size());

				for ( auto& f : config.mesh["mesh"]["position"] ) 		position.push_back(f.asFloat());
				for ( auto& f : config.mesh["mesh"]["normal"] ) 		normal.push_back(f.asFloat());
				for ( auto& f : config.mesh["mesh"]["color"] ) 			color.push_back(f.asFloat());
				for ( auto& f : config.mesh["mesh"]["uv"] ) 			uv.push_back(f.asFloat());
			}
			/* Load from Model (if specified) */ else if ( config.mesh["source"] == "model" ) {
				std::string filename = file.directory + config.mesh["mesh"]["model"].asString();
				ext::assimp::load( filename, mesh, config.mesh["mesh"]["flip"].asBool() );
			}
			if ( config.mesh["mesh"]["indexed"].asBool() ) mesh.index();
			mesh.generate();
		} else {
			uf::SkeletalMesh& mesh = this->getComponent<uf::SkeletalMesh>();
			/* Load from Model (if specified) */ if ( config.mesh["source"] == "model" ) {
				std::string filename = file.directory + config.mesh["mesh"]["model"].asString();
				ext::assimp::load( filename, mesh, config.mesh["mesh"]["flip"].asBool() );
			}
			if ( config.mesh["mesh"]["indexed"].asBool() ) mesh.index();
			mesh.generate();
		}
	}
	/* Metadata */ if ( config.base["metadata"] != Json::nullValue ) {
		uf::Serializer& metadata = this->getComponent<uf::Serializer>();
		struct {
			bool exists = false;
			std::string directory;
			std::string localFilename;
			std::string filename;
		} file;
		file.directory = root + uf::string::directory(config.base["metadata"].asString());
		file.localFilename = uf::string::filename(config.base["metadata"].asString());
		file.filename = file.directory + file.localFilename;
		file.exists = metadata.readFromFile(file.filename);
		/* Read from file */ if ( !file.exists ) {
			uf::iostream << "Error loading `" << file.filename << "!" << "\n";
			return false;
		}
	}

	return true;
}
void ext::Object::load( const uf::Serializer& m ) {
	uf::Serializer& metadata = this->getComponent<uf::Serializer>();
	metadata = m;
}