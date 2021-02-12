#include <uf/ext/gltf/graph.h>
#include <uf/ext/bullet/bullet.h>
#include <uf/utils/math/physics.h>

#define UF_VK_TESTING 1

namespace {
	void initializeGraphics( pod::Graph& graph, uf::Object& entity ) {
		auto& graphic = entity.getComponent<uf::Graphic>();
		graphic.device = &uf::renderer::device;
		graphic.material.device = &uf::renderer::device;
		graphic.descriptor.frontFace = VK_FRONT_FACE_COUNTER_CLOCKWISE;
		graphic.descriptor.cullMode = !(graph.mode & ext::gltf::LoadMode::INVERT) ? VK_CULL_MODE_BACK_BIT : VK_CULL_MODE_FRONT_BIT; // VK_CULL_MODE_NONE
		if ( graph.metadata["cull mode"].is<std::string>() ) {
			if ( graph.metadata["cull mode"].as<std::string>() == "back" ) {
				graphic.descriptor.cullMode = VK_CULL_MODE_BACK_BIT;
			} else if ( graph.metadata["cull mode"].as<std::string>() == "front" ) {
				graphic.descriptor.cullMode = VK_CULL_MODE_FRONT_BIT;
			} else if ( graph.metadata["cull mode"].as<std::string>() == "none" ) {
				graphic.descriptor.cullMode = VK_CULL_MODE_NONE;
			} else if ( graph.metadata["cull mode"].as<std::string>() == "both" ) {
				graphic.descriptor.cullMode = VK_CULL_MODE_FRONT_AND_BACK;
			}
		}
		{
		#if UF_VK_TESTING
			for ( auto& texture : graph.textures ) {
				graphic.material.textures.emplace_back().aliasTexture(texture.texture);
			}
		#else
			if ( graph.mode & ext::gltf::LoadMode::ATLAS ) {
				auto& atlas = *graph.atlas;
				auto& texture = graphic.material.textures.emplace_back();
				texture.loadFromImage( atlas.getAtlas() );
			} else {
				graphic.material.textures.emplace_back().aliasTexture(uf::renderer::Texture2D::empty);
				for ( auto& image : graph.images ) {
					auto& texture = graphic.material.textures.emplace_back();

					if ( graph.metadata["filter"].is<std::string>() ) {
						std::string filter = graph.metadata["filter"].as<std::string>();
						if ( filter == "NEAREST" ) {
							texture.sampler.descriptor.filter.min = VK_FILTER_NEAREST;
							texture.sampler.descriptor.filter.mag = VK_FILTER_NEAREST;
						} else if ( filter == "LINEAR" ) {
							texture.sampler.descriptor.filter.min = VK_FILTER_LINEAR;
							texture.sampler.descriptor.filter.mag = VK_FILTER_LINEAR;
						}
					}
					texture.loadFromImage( image );
				}
			}
		#endif
			for ( auto& sampler : graph.samplers ) {
				graphic.material.samplers.emplace_back( sampler );
			}
		}
		if ( graph.mode & ext::gltf::LoadMode::LOAD ) {
			if ( graph.mode & ext::gltf::LoadMode::SEPARATE ) {
				if ( graph.mode & ext::gltf::LoadMode::SKINNED ) {
					graphic.material.attachShader("./data/shaders/gltf.skinned.vert.spv", VK_SHADER_STAGE_VERTEX_BIT);
				} else {
					graphic.material.attachShader("./data/shaders/gltf.vert.spv", VK_SHADER_STAGE_VERTEX_BIT);
				}
			} else {
				if ( graph.mode & ext::gltf::LoadMode::SKINNED ) {
					graphic.material.attachShader("./data/shaders/gltf.skinned.instanced.vert.spv", VK_SHADER_STAGE_VERTEX_BIT);
				} else {
					graphic.material.attachShader("./data/shaders/gltf.instanced.vert.spv", VK_SHADER_STAGE_VERTEX_BIT);
				}
			}
			graphic.material.attachShader("./data/shaders/gltf.frag.spv", VK_SHADER_STAGE_FRAGMENT_BIT);
			{
				auto& shader = graphic.material.getShader("vertex");
				struct SpecializationConstant {
					uint32_t passes = 6;
				};
				auto& specializationConstants = shader.specializationConstants.get<SpecializationConstant>();
				specializationConstants.passes = uf::renderer::settings::maxViews;
			}
			{
				auto& shader = graphic.material.getShader("fragment");
				struct SpecializationConstant {
					uint32_t textures = 1;
				};
				auto& specializationConstants = shader.specializationConstants.get<SpecializationConstant>();
				specializationConstants.textures = graphic.material.textures.size();
				for ( auto& binding : shader.descriptorSetLayoutBindings ) {
					if ( binding.descriptorCount > 1 )
						binding.descriptorCount = specializationConstants.textures;
				}
			}
		} else {
			graphic.process = false;
		}
	}
}

pod::Matrix4f uf::graph::local( pod::Graph& graph, int32_t index ) {
	auto& node = 0 < index && index <= graph.nodes.size() ? graph.nodes[index] : graph.root;
	return
		uf::matrix::translate( uf::matrix::identity(), node.transform.position ) *
		uf::quaternion::matrix(node.transform.orientation) *
		uf::matrix::scale( uf::matrix::identity(), node.transform.scale ) *
		node.transform.model;
}
pod::Matrix4f uf::graph::matrix( pod::Graph& graph, int32_t index ) {
	pod::Matrix4f matrix = local( graph, index );
	auto& node = *uf::graph::find( graph, index );
	int32_t parent = node.parent;
	while ( 0 < parent && parent <= graph.nodes.size() ) {
		matrix = local( graph, parent ) * matrix;
		parent = graph.nodes[parent].parent;
	}
	return matrix;
}
pod::Node* uf::graph::find( pod::Graph& graph, int32_t index ) {
	return 0 <= index && index < graph.nodes.size() ? &graph.nodes[index] : NULL;
}
pod::Node* uf::graph::find( pod::Graph& graph, const std::string& name ) {
	for ( auto& node : graph.nodes ) if ( node.name == name ) return &node;
	return NULL;
}

void uf::graph::process( uf::Object& entity ) {
	auto& graph = entity.getComponent<pod::Graph>();
	for ( auto index : graph.root.children ) process( graph, index, entity );
}
void uf::graph::process( pod::Graph& graph ) {
	if ( graph.atlas ) {
		auto& texture = *graph.textures.emplace(graph.textures.begin());
		texture.name = "atlas";
		texture.storage.index = 0;
		texture.storage.sampler = 0;
		texture.storage.remap = 0;
		texture.storage.blend = 0;
		auto& image = *graph.images.emplace(graph.images.begin(), graph.atlas->getAtlas());
	}
#if UF_VK_TESTING
	if ( graph.atlas ) {
		auto& image = graph.images[0];
		auto& texture = graph.textures[0].texture;

		if ( graph.metadata["filter"].is<std::string>() ) {
			std::string filter = graph.metadata["filter"].as<std::string>();
			if ( filter == "NEAREST" ) {
				texture.sampler.descriptor.filter.min = VK_FILTER_NEAREST;
				texture.sampler.descriptor.filter.mag = VK_FILTER_NEAREST;
			} else if ( filter == "LINEAR" ) {
				texture.sampler.descriptor.filter.min = VK_FILTER_LINEAR;
				texture.sampler.descriptor.filter.mag = VK_FILTER_LINEAR;
			}
		}
		texture.loadFromImage( image );
	} else {
		for ( size_t i = 0; i < graph.textures.size() && i < graph.images.size(); ++i ) {
			auto& image = graph.images[i];
			auto& texture = graph.textures[i].texture;

			if ( graph.metadata["filter"].is<std::string>() ) {
				std::string filter = graph.metadata["filter"].as<std::string>();
				if ( filter == "NEAREST" ) {
					texture.sampler.descriptor.filter.min = VK_FILTER_NEAREST;
					texture.sampler.descriptor.filter.mag = VK_FILTER_NEAREST;
				} else if ( filter == "LINEAR" ) {
					texture.sampler.descriptor.filter.min = VK_FILTER_LINEAR;
					texture.sampler.descriptor.filter.mag = VK_FILTER_LINEAR;
				}
			}
			texture.loadFromImage( image );
		}
	}
#endif

	if ( !graph.root.entity ) graph.root.entity = new uf::Object;
	for ( auto index : graph.root.children ) process( graph, index, *graph.root.entity );

	if ( !(graph.mode & ext::gltf::LoadMode::SEPARATE) ) {
		initializeGraphics( graph, *graph.root.entity );

		auto& graphic = graph.root.entity->getComponent<uf::Graphic>();
		
		std::vector<pod::Matrix4f> instances;
		instances.reserve( graph.nodes.size() );
		for ( auto& node : graph.nodes ) {
			instances.emplace_back( uf::transform::model( node.transform ) );
		}
		// Models storage buffer
		graph.instanceBufferIndex = graphic.initializeBuffer(
			(void*) instances.data(),
			instances.size() * sizeof(pod::Matrix4f),
			VK_BUFFER_USAGE_STORAGE_BUFFER_BIT
		);
		// Joints storage buffer
		if ( graph.mode & ext::gltf::LoadMode::SKINNED ) {
			for ( auto& node : graph.nodes ) {
				if ( node.skin < 0 ) continue;
				auto& skin = graph.skins[node.skin];
				node.jointBufferIndex = graphic.initializeBuffer(
					(void*) skin.inverseBindMatrices.data(),
					(1 + skin.inverseBindMatrices.size()) * sizeof(pod::Matrix4f),
					VK_BUFFER_USAGE_STORAGE_BUFFER_BIT
				);
				break;
			}
		}
		// Materials storage buffer
		std::vector<pod::Material::Storage> materials( graph.materials.size() );
		for ( size_t i = 0; i < graph.materials.size(); ++i ) {
			materials[i] = graph.materials[i].storage;
			if ( graph.metadata["alpha cutoff"].is<float>() )
				materials[i].factorAlphaCutoff = graph.metadata["alpha cutoff"].as<float>();
		}
		graph.root.materialBufferIndex = graphic.initializeBuffer(
			(void*) materials.data(),
			materials.size() * sizeof(pod::Material::Storage),
			VK_BUFFER_USAGE_STORAGE_BUFFER_BIT
		);
		// Texture storage buffer
		std::vector<pod::Texture::Storage> textures( graph.textures.size() );
		for ( size_t i = 0; i < graph.textures.size(); ++i ) {
			textures[i] = graph.textures[i].storage;
			textures[i].remap = i;
		}
		graph.root.textureBufferIndex = graphic.initializeBuffer(
			(void*) textures.data(),
			textures.size() * sizeof(pod::Texture::Storage),
			VK_BUFFER_USAGE_STORAGE_BUFFER_BIT
		);
	}

	graph.root.entity->process([&]( uf::Entity* entity ) {
		if ( !entity->hasComponent<ext::gltf::mesh_t>() ) return;
		
		auto& mesh = entity->getComponent<ext::gltf::mesh_t>();
		auto& metadata = entity->getComponent<uf::Serializer>();
		std::string nodeName = metadata["system"]["graph"]["name"].as<std::string>();
		if ( graph.mode & ext::gltf::LoadMode::NORMALS ) {
			// bool invert = false;
			bool INVERTED = graph.mode & ext::gltf::LoadMode::INVERT;
			if ( !mesh.indices.empty() ) {
				for ( size_t i = 0; i < mesh.vertices.size(); i+=3 ) {
					auto& a = mesh.vertices[i+(INVERTED ? 0 : 0)].position;
					auto& b = mesh.vertices[i+(INVERTED ? 1 : 2)].position;
					auto& c = mesh.vertices[i+(INVERTED ? 2 : 1)].position;

					pod::Vector3f normal = uf::vector::normalize( uf::vector::cross( b - a, c - a ) );
					mesh.vertices[i+0].normal = normal;
					mesh.vertices[i+1].normal = normal;
					mesh.vertices[i+2].normal = normal;
				}
			} else {
				for ( size_t i = 0; i < mesh.indices.size(); i+=3 ) {
					auto& A = mesh.vertices[mesh.indices[i+(INVERTED ? 0 : 0)]];
					auto& B = mesh.vertices[mesh.indices[i+(INVERTED ? 1 : 2)]];
					auto& C = mesh.vertices[mesh.indices[i+(INVERTED ? 2 : 1)]];

					auto& a = A.position;
					auto& b = B.position;
					auto& c = C.position;

					pod::Vector3f normal = uf::vector::normalize( uf::vector::cross( b - a, c - a ) );
					
					A.normal = normal;
					B.normal = normal;
					C.normal = normal;
				}
			}
		}
		if ( entity->hasComponent<uf::Graphic>() ) {
			size_t o = graph.metadata["mesh optimization"].is<size_t>() ? graph.metadata["mesh optimization"].as<size_t>() : SIZE_MAX;
			auto& graphic = entity->getComponent<uf::Graphic>();
			graphic.initialize();
			graphic.initializeGeometry( mesh, o );
		}
		
		if ( !ext::json::isNull( graph.metadata["tags"][nodeName] ) ) {
			auto& info = graph.metadata["tags"][nodeName];
			if ( info["collision"].is<std::string>() ) {
				std::string type = info["collision"].as<std::string>();
				if ( type == "static mesh" ) {
					bool applyTransform = false; //!(graph.mode & ext::gltf::LoadMode::TRANSFORM);
					auto& collider = ext::bullet::create( entity->as<uf::Object>(), mesh, applyTransform, 1 );
					if ( !applyTransform ) {
						btBvhTriangleMeshShape* triangleMeshShape = (btBvhTriangleMeshShape*) collider.shape;
						btTriangleInfoMap* triangleInfoMap = new btTriangleInfoMap();
						triangleInfoMap->m_edgeDistanceThreshold = 0.01f;
					    triangleInfoMap->m_maxEdgeAngleThreshold = SIMD_HALF_PI*0.25;
						if ( applyTransform ) {
							btGenerateInternalEdgeInfo(triangleMeshShape, triangleInfoMap);
						}
						collider.body->setCollisionFlags(collider.body->getCollisionFlags() | btCollisionObject::CF_CUSTOM_MATERIAL_CALLBACK);
					}
				} else if ( type == "bounding box" ) {

				}
			}
		}
	});
}
void uf::graph::process( pod::Graph& graph, int32_t index, uf::Object& parent ) {
	// create child if requested
	uf::Object* pointer = new uf::Object;
	parent.addChild(*pointer);

	uf::Object& entity = *pointer;
	auto& node = graph.nodes[index];
	node.entity = &entity;
	
	bool setName = entity.getName() == "Entity";
	auto& metadata = entity.getComponent<uf::Serializer>();
	metadata["system"]["graph"]["name"] = node.name;
	// tie to tag
	if ( !ext::json::isNull( graph.metadata["tags"][node.name] ) ) {
		auto& info = graph.metadata["tags"][node.name];
		if ( info["action"].as<std::string>() == "load" ) {
			if ( info["filename"].is<std::string>() ) {
				std::string filename = uf::io::resolveURI( info["filename"].as<std::string>(), graph.metadata["root"].as<std::string>() );
				entity.load(filename);
			} else if ( ext::json::isObject( info["payload"] ) ) {
				uf::Serializer json = info["payload"];
				json["root"] = graph.metadata["root"];
				entity.load(json);
			}
		} else if ( info["action"].as<std::string>() == "attach" ) {
			std::string filename = uf::io::resolveURI( info["filename"].as<std::string>(), graph.metadata["root"].as<std::string>() );
			auto& child = entity.loadChild( filename, false );
			auto& childTransform = child.getComponent<pod::Transform<>>();
			auto flatten = uf::transform::flatten( node.transform );
			if ( !info["preserve position"].as<bool>() ) childTransform.position = flatten.position;
			if ( !info["preserve orientation"].as<bool>() ) childTransform.orientation = flatten.orientation;
		}
	}
	// create as light
	for ( auto& l : graph.lights ) {
		if ( l.name != node.name ) continue;
		entity.load("/light.json");
		auto& metadata = entity.getComponent<uf::Serializer>();

		metadata["light"]["radius"][0] = 0.001;
		metadata["light"]["radius"][1] = l.range; // l.range <= 0.001f ? graph.metadata["lights"]["range cap"].as<float>() : l.range;
		metadata["light"]["power"] = l.intensity; //  * graph.metadata["lights"]["power scale"].as<float>();
		metadata["light"]["color"][0] = l.color.x;
		metadata["light"]["color"][1] = l.color.y;
		metadata["light"]["color"][2] = l.color.z;
		metadata["light"]["shadows"] = graph.metadata["lights"]["shadows"].as<bool>();
		if ( metadata["light"]["shadows"].as<bool>() ) {
			metadata["light"]["radius"][1] = 0;
		}
		if ( ext::json::isArray( graph.metadata["lights"]["radius"] ) ) {
			metadata["light"]["radius"] = graph.metadata["lights"]["radius"];
		}
		if ( graph.metadata["lights"]["bias"].is<float>() ) {
			metadata["light"]["bias"] = graph.metadata["lights"]["bias"].as<float>();
		}
		// copy from tag information
		ext::json::forEach( graph.metadata["tags"][node.name]["light"], [&]( const std::string& key, ext::json::Value& value ){
			if ( key == "transform" ) return;
			metadata["light"][key] = value;
		});

		if ( metadata["light"]["shadows"].as<bool>() ) {
			ext::json::Value m;
			if ( ext::json::isObject( graph.metadata["tags"][node.name]["renderMode"] ) ) {
				m = graph.metadata["tags"][node.name]["renderMode"];
			} else if ( ext::json::isObject( graph.metadata["lights"]["renderMode"] ) ) {
				m = graph.metadata["lights"]["renderMode"];
			}
			if ( m["target"].is<std::string>() ) {
				std::string targetNode = m["target"].as<std::string>();
				auto* node = uf::graph::find( graph, targetNode );
				if ( node ) {
					auto& drawCall = node->drawCall;
					size_t targetIndices = graph.drawCall.indices;
					metadata["renderMode"]["target"][std::to_string(targetIndices)]["size"] = drawCall.indices;
					metadata["renderMode"]["target"][std::to_string(targetIndices)]["offset"] = drawCall.indicesIndex;
				//	std::cout << "Found " << targetNode << ", Replacing " << targetNode << " with " << drawCall.indicesIndex << " + " << drawCall.indices << std::endl;
				}
			}
		}
	//	auto& transform = entity.getComponent<pod::Transform<>>();
	//	transform.orientation = uf::quaternion::inverse( transform.orientation );
		break;
	}

	// set name
	if ( setName ) entity.setName( node.name );

	// reference transform to parent
	{
		auto& transform = entity.getComponent<pod::Transform<>>();
		transform = node.transform;
		// is a child
		if ( node.index != -1 )
			transform.reference = &entity.getParent().getComponent<pod::Transform<>>();
		// override transform
		if ( !ext::json::isNull( graph.metadata["tags"][node.name] ) ) {
			auto& info = graph.metadata["tags"][node.name];
			if ( info["transform"]["offset"].as<bool>() ) {
				auto parsed = uf::transform::decode( info["transform"], pod::Transform<>{} );
				transform.position += parsed.position;
				transform.orientation = uf::quaternion::multiply( transform.orientation, parsed.orientation );
			} else {
				transform = uf::transform::decode( info["transform"], transform );
				if ( info["transform"]["parent"].is<std::string>() ) {
					auto* parentPointer = uf::graph::find( graph, info["transform"]["parent"].as<std::string>() );
					if ( parentPointer ) {
						auto& parentNode = *parentPointer;
						// entity already exists, bind to its transform
						if ( parentNode.entity && parentNode.entity->hasComponent<pod::Transform<>>() ) {
							auto& parentTransform = parentNode.entity->getComponent<pod::Transform<>>();
							transform = uf::transform::reference( transform, parentTransform, info["transform"]["reorient"].as<bool>() );
							transform.position = -transform.position;
						// doesnt exist, bind to the node transform
						} else {
							transform = uf::transform::reference( transform, parentNode.transform, info["transform"]["reorient"].as<bool>() );
						}
					}
				}
			}
		}
	}
	// copy mesh
	if ( !node.mesh.vertices.empty() ) {
		auto& mesh = entity.getComponent<ext::gltf::mesh_t>();
		struct {
			size_t vertices = 0;
			size_t indices = 0;
		} start = {
			mesh.vertices.size(),
			mesh.indices.size(),
		};
		mesh.vertices.reserve( node.mesh.vertices.size() + start.vertices );
		mesh.indices.reserve( node.mesh.indices.size() + start.indices );
		for ( auto& v : node.mesh.vertices ) mesh.vertices.emplace_back( v );
		for ( auto& i : node.mesh.indices ) mesh.indices.emplace_back( i + start.indices );

		if ( !(graph.mode & ext::gltf::LoadMode::SEPARATE) ) {
			auto& mesh = graph.root.entity->getComponent<ext::gltf::mesh_t>();
			ext::gltf::mesh_t expanded = node.mesh;
			expanded.expand();
			mesh.vertices.reserve( mesh.vertices.size() + expanded.vertices.size() );
			for ( auto& v : expanded.vertices ) mesh.vertices.emplace_back( v );
		} else if ( graph.mode & ext::gltf::LoadMode::RENDER ) {
			uf::instantiator::bind("RenderBehavior", entity);
			initializeGraphics( graph, entity );
			auto& graphic = entity.getComponent<uf::Graphic>();
			// Joints storage buffer
			if ( graph.mode & ext::gltf::LoadMode::SKINNED && node.skin >= 0 ) {
				auto& skin = graph.skins[node.skin];
				node.jointBufferIndex = graphic.initializeBuffer(
					(void*) skin.inverseBindMatrices.data(),
					skin.inverseBindMatrices.size() * sizeof(pod::Matrix4f),
					VK_BUFFER_USAGE_STORAGE_BUFFER_BIT
				);
			}
			// Materials storage buffer
			std::vector<pod::Material::Storage> materials( graph.materials.size() );
			for ( size_t i = 0; i < graph.materials.size(); ++i ) {
				materials[i] = graph.materials[i].storage;
			}
		/*
			if ( !ext::json::isNull( graph.metadata["tags"][node.name] ) ) {
				auto& info = graph.metadata["tags"][node.name];
				for ( size_t i = 0; i < graph.materials.size(); ++i ) {
					materials[i].factorAlphaCutoff = info["alpha cutoff"].as<float>();
				}
			}
		*/
			node.materialBufferIndex = graphic.initializeBuffer(
				(void*) materials.data(),
				materials.size() * sizeof(pod::Material::Storage),
				VK_BUFFER_USAGE_STORAGE_BUFFER_BIT
			);
			// Texture storage buffer
			std::vector<pod::Texture::Storage> textures( graph.textures.size() );
			for ( size_t i = 0; i < graph.textures.size(); ++i ) {
				textures[i] = graph.textures[i].storage;
				textures[i].remap = i;
			}
			graph.root.textureBufferIndex = graphic.initializeBuffer(
				(void*) textures.data(),
				textures.size() * sizeof(pod::Texture::Storage),
				VK_BUFFER_USAGE_STORAGE_BUFFER_BIT
			);
		}
	}
	
	for ( auto index : node.children ) uf::graph::process( graph, index, entity );
}

void uf::graph::override( pod::Graph& graph ) {
	graph.settings.animations.override.a = 0;
	graph.settings.animations.override.map.clear();
	bool toNeutralPose = graph.sequence.empty();
	// store every node's current transform
	for ( auto& node : graph.nodes ) {
		graph.settings.animations.override.map[node.index].first = node.transform;
		graph.settings.animations.override.map[node.index].second = node.transform;
		if ( toNeutralPose ) {
			graph.settings.animations.override.map[node.index].second.position = { 0, 0, 0 };
			graph.settings.animations.override.map[node.index].second.orientation = { 0, 0, 0, 1 };
			graph.settings.animations.override.map[node.index].second.scale = { 1, 1, 1 };
		}
	}
	// set our destination transform per node
	if ( !toNeutralPose ) {
		std::string name = graph.sequence.front();
		pod::Animation& animation = graph.animations[name];
		for ( auto& channel : animation.channels ) {
			auto& override = graph.settings.animations.override.map[channel.node];
			auto& sampler = animation.samplers[channel.sampler];
			if ( sampler.interpolator != "LINEAR" ) continue;
			for ( size_t i = 0; i < sampler.inputs.size() - 1; ++i ) {
				if ( !(animation.start >= sampler.inputs[i] && animation.start <= sampler.inputs[i+1]) ) continue;
				if ( channel.path == "translation" ) {
					override.second.position = sampler.outputs[i];
				} else if ( channel.path == "rotation" ) {
					override.second.orientation = uf::quaternion::normalize( sampler.outputs[i] );
				} else if ( channel.path == "scale" ) {
					override.second.scale = sampler.outputs[i];
				}
			}
		}
	}
}

void uf::graph::animate( pod::Graph& graph, const std::string& name, float speed, bool immediate ) {
	if ( !(graph.mode & ext::gltf::LoadMode::SKINNED) ) return;
	if ( graph.animations.count( name ) > 0 ) {
		// if already playing, ignore it
		if ( !graph.sequence.empty() && graph.sequence.front() == name ) return;
		if ( immediate ) {
			while ( !graph.sequence.empty() ) graph.sequence.pop();
		}
		bool empty = graph.sequence.empty();
		graph.sequence.emplace(name);
		if ( empty ) uf::graph::override( graph );
		graph.settings.animations.speed = speed;
	}
	update( graph, 0 );
}
void uf::graph::update( pod::Graph& graph ) {
	return update( graph, uf::physics::time::delta );
}
void uf::graph::update( pod::Graph& graph, float delta ) {
	// no override
	if ( !(graph.mode & ext::gltf::LoadMode::SKINNED) ) return;
	if ( graph.sequence.empty() ) goto UPDATE;
	if ( graph.settings.animations.override.a >= 0 ) goto OVERRIDE;
	{
		std::string name = graph.sequence.front();
		pod::Animation* animation = &graph.animations[name];
	//	std::cout << "ANIMATION: " << name << "\t" << animation->cur << std::endl;
		animation->cur += delta * graph.settings.animations.speed; // * graph.settings.animations.override.speed;
		if ( animation->end < animation->cur ) {
			animation->cur = graph.settings.animations.loop ? animation->cur - animation->end : 0;
			// go-to next animation
			if ( !graph.settings.animations.loop ) {
				graph.sequence.pop();
				// out of animations, set to neutral pose
				if ( graph.sequence.empty() ) {
					uf::graph::override( graph );
					goto OVERRIDE;
				}
				name = graph.sequence.front();
				animation = &graph.animations[name];
			}
		}
		for ( auto& channel : animation->channels ) {
			auto& sampler = animation->samplers[channel.sampler];
			if ( sampler.interpolator != "LINEAR" ) continue;
			for ( size_t i = 0; i < sampler.inputs.size() - 1; ++i ) {
				if ( !(animation->cur >= sampler.inputs[i] && animation->cur <= sampler.inputs[i+1]) ) continue;
				float a = (animation->cur - sampler.inputs[i]) / (sampler.inputs[i+1] - sampler.inputs[i]);
				auto& transform = graph.nodes[channel.node].transform;
				if ( channel.path == "translation" ) {
					transform.position = uf::vector::mix( sampler.outputs[i], sampler.outputs[i+1], a );
				} else if ( channel.path == "rotation" ) {
					transform.orientation = uf::quaternion::normalize( uf::quaternion::slerp(sampler.outputs[i], sampler.outputs[i+1], a) );
				} else if ( channel.path == "scale" ) {
					transform.scale = uf::vector::mix( sampler.outputs[i], sampler.outputs[i+1], a );
				}
			}
		}
		goto UPDATE;
	}
OVERRIDE:
	// std::cout << "OVERRIDED: " << graph.settings.animations.override.a << "\t" << -std::numeric_limits<float>::max() << std::endl;
	for ( auto pair : graph.settings.animations.override.map ) {
		graph.nodes[pair.first].transform.position = uf::vector::mix( pair.second.first.position, pair.second.second.position, graph.settings.animations.override.a );
		graph.nodes[pair.first].transform.orientation = uf::quaternion::normalize( uf::quaternion::slerp(pair.second.first.orientation, pair.second.second.orientation, graph.settings.animations.override.a) );
		graph.nodes[pair.first].transform.scale = uf::vector::mix( pair.second.first.scale, pair.second.second.scale, graph.settings.animations.override.a );
	}
	// finished our overrided interpolation, clear it
	if ( (graph.settings.animations.override.a += delta * graph.settings.animations.override.speed) >= 1 ) {
		graph.settings.animations.override.a = -std::numeric_limits<float>::max();
		graph.settings.animations.override.map.clear();
	}
UPDATE:
	for ( auto& node : graph.nodes ) uf::graph::update( graph, node );
}
void uf::graph::update( pod::Graph& graph, pod::Node& node ) {
	if ( node.skin >= 0 ) {
		pod::Matrix4f nodeMatrix = uf::graph::matrix( graph, node.index );
		pod::Matrix4f inverseTransform = uf::matrix::inverse( nodeMatrix );
		auto& skin = graph.skins[node.skin];
		std::vector<pod::Matrix4f> joints;
		joints.reserve( skin.joints.size() );
		for ( size_t i = 0; i < skin.joints.size(); ++i ) {
			joints.emplace_back( uf::matrix::identity() );
		}
		if ( graph.settings.animations.override.a >= 0 || !graph.sequence.empty() ) {
			for ( size_t i = 0; i < skin.joints.size(); ++i ) {
				joints[i] = inverseTransform * (uf::graph::matrix(graph, skin.joints[i]) * skin.inverseBindMatrices[i]);
			}
		}
		if ( node.entity && node.entity->hasComponent<uf::Graphic>() ) {
			auto& graphic = node.entity->getComponent<uf::Graphic>();
			auto& buffer = graphic.buffers.at(node.jointBufferIndex);
			graphic.updateBuffer( (void*) joints.data(), joints.size() * sizeof(pod::Matrix4f), buffer, false );
		}
	}
}
void uf::graph::destroy( pod::Graph& graph ) {
	for ( auto& t : graph.textures ) {
		t.texture.destroy();
	}
	delete graph.atlas;
}