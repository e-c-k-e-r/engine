#include <uf/ext/gltf/graph.h>
#include <uf/ext/bullet/bullet.h>
#include <uf/utils/math/physics.h>
#include <uf/utils/mesh/grid.h>
#include <uf/utils/thread/thread.h>

#define UF_VK_TESTING 1
#if UF_ENV_DREAMCAST
	#define UF_GRAPH_LOAD_MULTITHREAD 0
#else
	#define UF_GRAPH_LOAD_MULTITHREAD 1
#endif

namespace {
	void initializeShaders( pod::Graph& graph, uf::Object& entity ) {
		auto& graphic = entity.getComponent<uf::Graphic>();
		std::string root = uf::io::directory( graph.name );
		size_t texture2Ds = 0;
		size_t texture3Ds = 0;
		for ( auto& texture : graphic.material.textures ) {
			if ( texture.width > 1 && texture.height > 1 && texture.depth == 1 && texture.layers == 1 ) ++texture2Ds;
			else if ( texture.width > 1 && texture.height > 1 && texture.depth > 1 && texture.layers == 1 ) ++texture3Ds;
		}

		// standard pipeline
		{
			std::string vertexShaderFilename = graph.metadata["shaders"]["vertex"].as<std::string>("/gltf/base.vert.spv");
			std::string geometryShaderFilename = graph.metadata["shaders"]["geometry"].as<std::string>("");
			std::string fragmentShaderFilename = graph.metadata["shaders"]["fragment"].as<std::string>("/gltf/base.frag.spv");
			{
				if ( !graph.metadata["flags"]["SEPARATE"].as<bool>() ) {
					vertexShaderFilename = graph.metadata["flags"]["SKINNED"].as<bool>() ? "/gltf/skinned.instanced.vert.spv" : "/gltf/instanced.vert.spv";
				} else if ( graph.metadata["flags"]["SKINNED"].as<bool>() ) vertexShaderFilename = "/gltf/skinned.vert.spv";
				vertexShaderFilename = entity.grabURI( vertexShaderFilename, root );
				graphic.material.attachShader(vertexShaderFilename, uf::renderer::enums::Shader::VERTEX);
			}
			if ( geometryShaderFilename != "" && uf::renderer::device.enabledFeatures.geometryShader ) {
				geometryShaderFilename = entity.grabURI( geometryShaderFilename, root );
				graphic.material.attachShader(geometryShaderFilename, uf::renderer::enums::Shader::GEOMETRY);
			}
			{
				fragmentShaderFilename = entity.grabURI( fragmentShaderFilename, root );
				graphic.material.attachShader(fragmentShaderFilename, uf::renderer::enums::Shader::FRAGMENT);
			}
		#if UF_USE_VULKAN
			{
				auto& shader = graphic.material.getShader("vertex");
				struct SpecializationConstant {
					uint32_t passes = 6;
				};
				auto& specializationConstants = shader.specializationConstants.get<SpecializationConstant>();
				specializationConstants.passes = uf::renderer::settings::maxViews;
				ext::json::forEach( shader.metadata["specializationConstants"], [&]( ext::json::Value& sc ){
					if ( sc["name"].as<std::string>() == "PASSES" ) sc["value"] = specializationConstants.passes;
				});
			}
			{
				auto& shader = graphic.material.getShader("fragment");
				struct SpecializationConstant {
					uint32_t textures = 1;
				};
				auto& specializationConstants = shader.specializationConstants.get<SpecializationConstant>();
				specializationConstants.textures = texture2Ds;
				ext::json::forEach( shader.metadata["specializationConstants"], [&]( ext::json::Value& sc ){
					if ( sc["name"].as<std::string>() == "TEXTURES" ) sc["value"] = specializationConstants.textures;
				});

				ext::json::forEach( shader.metadata["definitions"]["textures"], [&]( ext::json::Value& t ){
					if ( t["name"].as<std::string>() != "samplerTextures" ) return;
					size_t binding = t["binding"].as<size_t>();
					for ( auto& layout : shader.descriptorSetLayoutBindings ) {
						if ( layout.binding == binding ) layout.descriptorCount = specializationConstants.textures;
					}
				});
			}
		#endif
		}
		// vxgi pipeline
		if ( uf::renderer::settings::experimental::deferredMode == "vxgi" ) {
			std::string vertexShaderFilename = graph.metadata["shaders"]["vertex"].as<std::string>("/gltf/base.vert.spv");
			std::string geometryShaderFilename = graph.metadata["shaders"]["geometry"].as<std::string>("/gltf/voxelize.geom.spv");
			std::string fragmentShaderFilename = graph.metadata["shaders"]["fragment"].as<std::string>("/gltf/voxelize.frag.spv");

			if ( geometryShaderFilename != "" && uf::renderer::device.enabledFeatures.geometryShader ) {
				geometryShaderFilename = entity.grabURI( geometryShaderFilename, root );
				graphic.material.attachShader(geometryShaderFilename, uf::renderer::enums::Shader::GEOMETRY, "vxgi");
			}
			{
				fragmentShaderFilename = entity.grabURI( fragmentShaderFilename, root );
				graphic.material.attachShader(fragmentShaderFilename, uf::renderer::enums::Shader::FRAGMENT, "vxgi");
			}
		#if UF_USE_VULKAN
			{
				auto& shader = graphic.material.getShader("fragment", "vxgi");
				struct SpecializationConstant {
					uint32_t cascades = 4;
					uint32_t textures = 256;
				};
				auto& specializationConstants = shader.specializationConstants.get<SpecializationConstant>();
				specializationConstants.textures = texture2Ds;
				specializationConstants.cascades = texture3Ds / 4;
				ext::json::forEach( shader.metadata["specializationConstants"], [&]( ext::json::Value& sc ){
					std::string name = sc["name"].as<std::string>();
					if ( name == "TEXTURES" ) sc["value"] = specializationConstants.textures;
					else if ( name == "CASCADES" ) sc["value"] = specializationConstants.cascades;
				});
				ext::json::forEach( shader.metadata["definitions"]["textures"], [&]( ext::json::Value& t ){
					size_t binding = t["binding"].as<size_t>();
					std::string name = t["name"].as<std::string>();
					for ( auto& layout : shader.descriptorSetLayoutBindings ) {
						if ( layout.binding != binding ) continue;
						if ( name == "samplerTextures" ) layout.descriptorCount = specializationConstants.textures;
						else if ( name == "voxelId" ) layout.descriptorCount = specializationConstants.cascades;
						else if ( name == "voxelUv" ) layout.descriptorCount = specializationConstants.cascades;
						else if ( name == "voxelNormal" ) layout.descriptorCount = specializationConstants.cascades;
						else if ( name == "voxelRadiance" ) layout.descriptorCount = specializationConstants.cascades;
					}
				});
			}
		#endif
		}
		graphic.process = true;
	}
	void initializeGraphics( pod::Graph& graph, uf::Object& entity ) {
		auto& graphic = entity.getComponent<uf::Graphic>();
		graphic.device = &uf::renderer::device;
		graphic.material.device = &uf::renderer::device;
		graphic.descriptor.frontFace = uf::renderer::enums::Face::CCW;
		graphic.descriptor.cullMode = !(graph.metadata["flags"]["INVERT"].as<bool>()) ? uf::renderer::enums::CullMode::BACK : uf::renderer::enums::CullMode::FRONT;
		if ( graph.metadata["cull mode"].is<std::string>() ) {
			if ( graph.metadata["cull mode"].as<std::string>() == "back" ) {
				graphic.descriptor.cullMode = uf::renderer::enums::CullMode::BACK;
			} else if ( graph.metadata["cull mode"].as<std::string>() == "front" ) {
				graphic.descriptor.cullMode = uf::renderer::enums::CullMode::FRONT;
			} else if ( graph.metadata["cull mode"].as<std::string>() == "none" ) {
				graphic.descriptor.cullMode = uf::renderer::enums::CullMode::NONE;
			} else if ( graph.metadata["cull mode"].as<std::string>() == "both" ) {
				graphic.descriptor.cullMode = uf::renderer::enums::CullMode::BOTH;
			}
		}
		{
			for ( auto& texture : graph.textures ) {
				if ( !texture.bind ) continue;
				graphic.material.textures.emplace_back().aliasTexture(texture.texture);
			}
			for ( auto& sampler : graph.samplers ) {
				graphic.material.samplers.emplace_back( sampler );
			}
			// bind scene's voxel texture
			if ( uf::renderer::settings::experimental::deferredMode == "vxgi" ) {
				auto& scene = uf::scene::getCurrentScene();
				auto& sceneTextures = scene.getComponent<pod::SceneTextures>();
				for ( auto& t : sceneTextures.voxels.id ) graphic.material.textures.emplace_back().aliasTexture(t);
				for ( auto& t : sceneTextures.voxels.uv ) graphic.material.textures.emplace_back().aliasTexture(t);
				for ( auto& t : sceneTextures.voxels.normal ) graphic.material.textures.emplace_back().aliasTexture(t);
				for ( auto& t : sceneTextures.voxels.radiance ) graphic.material.textures.emplace_back().aliasTexture(t);
			}
		}
		if ( graph.metadata["flags"]["LOAD"].as<bool>() ) {
			initializeShaders( graph, entity );
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
	// using an atlas texture will not bind other textures
	// load lightmap, if requested
	if ( graph.metadata["lightmap"].is<std::string>() ) {
		// check if valid filename, if not it's a texture name
		std::string f = uf::io::sanitize( graph.metadata["lightmap"].as<std::string>(), uf::io::directory( graph.name ) );
		if ( uf::io::exists( f ) ) {
			for ( auto& material : graph.materials ) {
				material.storage.indexLightmap = graph.textures.size();
			}
			
			auto& texture = graph.textures.emplace_back();
			texture.name = "lightmap";
			texture.bind = true;
			texture.storage.index = graph.images.size();
			texture.storage.sampler = -1;
			texture.storage.remap = graph.images.size();
			texture.storage.blend = 0;
			auto& image = graph.images.emplace_back();
		#if UF_USE_VULKAN
			bool flip = false;
		#elif UF_USE_OPENGL
			bool flip = false;
		#endif
			image.open( f, flip );

			graph.metadata["lightmap"] = texture.name;
			graph.metadata["lightmapped"] = true;

			texture.texture.loadFromImage( image );

			texture.texture.sampler.descriptor.filter.min = uf::renderer::enums::Filter::NEAREST;
			texture.texture.sampler.descriptor.filter.mag = uf::renderer::enums::Filter::NEAREST;
		}
	// invalidate our ST's if we're in OpenGL
	} else {
	#if UF_USE_OPENGL
		for ( auto& mesh : graph.meshes ) for ( auto& v : mesh.vertices ) v.st = pod::Vector2f{0,0};
	#endif
	}
	if ( graph.metadata["flags"]["ATLAS"].as<bool>() && graph.atlas.generated() ) {
		auto& image = *graph.images.emplace(graph.images.begin(), graph.atlas.getAtlas());
		auto& texture = *graph.textures.emplace(graph.textures.begin());
		texture.name = "atlas";
		texture.bind = true;
		texture.storage.index = 0;
		texture.storage.sampler = 0;
		texture.storage.remap = -1;
		texture.storage.blend = 0;

		if ( graph.metadata["filter"].is<std::string>() ) {
			std::string filter = graph.metadata["filter"].as<std::string>();
			if ( filter == "NEAREST" ) {
				texture.texture.sampler.descriptor.filter.min = uf::renderer::enums::Filter::NEAREST;
				texture.texture.sampler.descriptor.filter.mag = uf::renderer::enums::Filter::NEAREST;
			} else if ( filter == "LINEAR" ) {
				texture.texture.sampler.descriptor.filter.min = uf::renderer::enums::Filter::LINEAR;
				texture.texture.sampler.descriptor.filter.mag = uf::renderer::enums::Filter::LINEAR;
			}
		}
		for ( auto& material : graph.materials ) {
			if ( 0 <= material.storage.indexAlbedo ) ++material.storage.indexAlbedo;
			if ( 0 <= material.storage.indexNormal ) ++material.storage.indexNormal;
			if ( 0 <= material.storage.indexEmissive ) ++material.storage.indexEmissive;
			if ( 0 <= material.storage.indexOcclusion ) ++material.storage.indexOcclusion;
			if ( 0 <= material.storage.indexMetallicRoughness ) ++material.storage.indexMetallicRoughness;
			material.storage.indexAtlas = texture.storage.index;
			if ( 0 <= material.storage.indexLightmap ) ++material.storage.indexLightmap;
		}
		texture.texture.loadFromImage( image );

		// remap texture slots
		size_t textureSlot = 0;
		for ( auto& texture : graph.textures ) {
			texture.storage.index = -1;
			if ( !texture.bind ) continue;
			texture.storage.index = textureSlot++;
		}
	} else {
		for ( size_t i = 0; i < graph.textures.size() && i < graph.images.size(); ++i ) {
			auto& image = graph.images[i];
			auto& texture = graph.textures[i];
			texture.bind = true;
			if ( graph.metadata["filter"].is<std::string>() ) {
				std::string filter = graph.metadata["filter"].as<std::string>();
				if ( filter == "NEAREST" ) {
					texture.texture.sampler.descriptor.filter.min = uf::renderer::enums::Filter::NEAREST;
					texture.texture.sampler.descriptor.filter.mag = uf::renderer::enums::Filter::NEAREST;
				} else if ( filter == "LINEAR" ) {
					texture.texture.sampler.descriptor.filter.min = uf::renderer::enums::Filter::LINEAR;
					texture.texture.sampler.descriptor.filter.mag = uf::renderer::enums::Filter::LINEAR;
				}
			}
			texture.texture.loadFromImage( image );
		}
	}

	if ( !graph.root.entity ) graph.root.entity = new uf::Object;
	for ( auto index : graph.root.children ) process( graph, index, *graph.root.entity );

	if ( !(graph.metadata["flags"]["SEPARATE"].as<bool>()) ) {
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
			uf::renderer::enums::Buffer::STORAGE
		);
		// Joints storage buffer
		if ( graph.metadata["flags"]["SKINNED"].as<bool>() ) {
			for ( auto& node : graph.nodes ) {
				if ( node.skin < 0 ) continue;
				auto& skin = graph.skins[node.skin];
				node.jointBufferIndex = graphic.initializeBuffer(
					(void*) skin.inverseBindMatrices.data(),
					(1 + skin.inverseBindMatrices.size()) * sizeof(pod::Matrix4f),
					uf::renderer::enums::Buffer::STORAGE
				);
				break;
			}
		}
		// Failsafe
		if ( graph.materials.empty() ) graph.materials.emplace_back();
		if ( graph.textures.empty() ) graph.textures.emplace_back();
		
		// Materials storage buffer
		std::vector<pod::Material::Storage> materials( graph.materials.size() );
		for ( size_t i = 0; i < graph.materials.size(); ++i ) {
			materials[i] = graph.materials[i].storage;
		}
		graph.root.materialBufferIndex = graphic.initializeBuffer(
			(void*) materials.data(),
			materials.size() * sizeof(pod::Material::Storage),
			uf::renderer::enums::Buffer::STORAGE
		);
		// Texture storage buffer
		std::vector<pod::Texture::Storage> textures( graph.textures.size() );
		for ( size_t i = 0; i < graph.textures.size(); ++i ) {
			textures[i] = graph.textures[i].storage;
		}
		graph.root.textureBufferIndex = graphic.initializeBuffer(
			(void*) textures.data(),
			textures.size() * sizeof(pod::Texture::Storage),
			uf::renderer::enums::Buffer::STORAGE
		);
	}

	// calculate extents
	pod::Vector3f extentMin = { std::numeric_limits<float>::max(), std::numeric_limits<float>::max(), std::numeric_limits<float>::max() };
	pod::Vector3f extentMax = { -std::numeric_limits<float>::max(), -std::numeric_limits<float>::max(), -std::numeric_limits<float>::max() };

	graph.root.entity->process([&]( uf::Entity* entity ) {
		if ( !entity->hasComponent<ext::gltf::mesh_t>() ) return;
		
		auto& transform = entity->getComponent<pod::Transform<>>();
		auto& mesh = entity->getComponent<ext::gltf::mesh_t>();
		auto& metadata = entity->getComponent<uf::Serializer>();
		std::string nodeName = metadata["system"]["graph"]["name"].as<std::string>();

		pod::Vector3f min = { std::numeric_limits<float>::max(), std::numeric_limits<float>::max(), std::numeric_limits<float>::max() };
		pod::Vector3f max = { -std::numeric_limits<float>::max(), -std::numeric_limits<float>::max(), -std::numeric_limits<float>::max() };

		auto model = uf::transform::model( transform );
		for ( auto& vertex : mesh.vertices ) {
		//	auto position = uf::matrix::multiply<float>( model, vertex.position, 1.0f );
			min.x = std::min( min.x, vertex.position.x );
			min.y = std::min( min.y, vertex.position.y );
			min.z = std::min( min.z, vertex.position.z );

			max.x = std::max( max.x, vertex.position.x );
			max.y = std::max( max.y, vertex.position.y );
			max.z = std::max( max.z, vertex.position.z );
		}

		min = uf::matrix::multiply<float>( model, min, 1.0f );
		max = uf::matrix::multiply<float>( model, max, 1.0f );

		extentMin.x = std::min( min.x, extentMin.x );
		extentMin.y = std::min( min.y, extentMin.y );
		extentMin.z = std::min( min.z, extentMin.z );

		extentMax.x = std::max( max.x, extentMax.x );
		extentMax.y = std::max( max.y, extentMax.y );
		extentMax.z = std::max( max.z, extentMax.z );
	#if 1
		if ( graph.metadata["flags"]["NORMALS"].as<bool>() ) {
			// bool invert = false;
			bool INVERTED = graph.metadata["flags"]["INVERT"].as<bool>();
			if ( mesh.indices.empty() ) {
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
	#endif
		if ( entity->hasComponent<uf::Graphic>() ) {
			auto& graphic = entity->getComponent<uf::Graphic>();
			graphic.initialize();
		#if 1
			if ( entity->getName() == "worldspawn_20" && ext::json::isObject( graph.metadata["grid"] ) ) {
				auto sliced = mesh;
				auto& grid = entity->getComponent<uf::MeshGrid>();
				grid.initialize( sliced, uf::vector::decode( graph.metadata["grid"]["size"], pod::Vector3ui{1,1,1} ) );
				grid.analyze();

				auto center = uf::vector::decode( graph.metadata["grid"]["center"], grid.center() );
				sliced.indices = grid.get<>( center );
				graphic.initializeMesh( sliced );
			} else {
				graphic.initializeMesh( mesh );
			}
		#else
			graphic.initialize();
			graphic.initializeMesh( mesh, 0 );
			if ( ext::json::isObject( graph.metadata["grid"] ) ) {
				auto& grid = entity->getComponent<uf::MeshGrid>();
				grid.initialize( mesh, uf::vector::decode( graph.metadata["grid"]["size"], pod::Vector3ui{1,1,1} ) );
				grid.analyze();

				auto center = uf::vector::decode( graph.metadata["grid"]["center"], grid.center() );
				auto indices = grid.get<>( center );
				graphic.descriptor.indices = indices.size();
				UF_DEBUG_MSG("Center: " << uf::vector::toString(center) << " | Indices: " << indices.size());
				int32_t indexBuffer = -1;
				for ( size_t i = 0; i < graphic.buffers.size(); ++i ) {
					if ( graphic.buffers[i].usage & uf::renderer::enums::Buffer::INDEX ) indexBuffer = i;
				}
				if ( indexBuffer >= 0 && !indices.empty() && indices.size() % 3 == 0 ) {
					graphic.updateBuffer(
						(void*) indices.data(),
						indices.size() * mesh.attributes.index.size,
						indexBuffer
					);
				}
			}
		#endif
		}
		
		if ( !ext::json::isNull( graph.metadata["tags"][nodeName] ) ) {
			auto& info = graph.metadata["tags"][nodeName];
			if ( info["collision"].is<std::string>() ) {
				std::string type = info["collision"].as<std::string>();
			#if UF_USE_BULLET
				if ( type == "static mesh" ) {
					bool applyTransform = false; //!(graph.metadata["flags"]["TRANSFORM"].as<bool>());
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
			#endif
			}
		}
	});

//	if ( ext::json::isNull( graph.metadata["extents"]["min"] ) )
		graph.metadata["extents"]["min"] = uf::vector::encode( extentMin * graph.metadata["extents"]["scale"].as<float>(1.0f) );
//	if ( ext::json::isNull( graph.metadata["extents"]["max"] ) )
		graph.metadata["extents"]["max"] = uf::vector::encode( extentMax * graph.metadata["extents"]["scale"].as<float>(1.0f) );
}
void uf::graph::process( pod::Graph& graph, int32_t index, uf::Object& parent ) {
	auto& node = graph.nodes[index];
	// ignore pesky light_Orientation nodes
	if ( uf::string::split( node.name, "_" ).back() == "Orientation" ) return;
	// create child if requested
	uf::Object* pointer = new uf::Object;
	parent.addChild(*pointer);

	uf::Object& entity = *pointer;
	node.entity = &entity;
	
	bool setName = entity.getName() == "Entity";
	auto& metadata = entity.getComponent<uf::ObjectBehavior::Metadata>();
	auto& metadataJson = entity.getComponent<uf::Serializer>();
	metadataJson["system"]["graph"]["name"] = node.name;
	// on systems where frametime is very, very important, we can set all static nodes to not tick
	// tie to tag
	if ( !ext::json::isNull( graph.metadata["tags"][node.name] ) ) {
		auto& info = graph.metadata["tags"][node.name];
		if ( info["ignore"].as<bool>() ) {
			return;
		}
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
		if ( info["static"].is<bool>() ) {
			metadata.system.ignoreGraph = info["static"].as<bool>();
		}
	}
	// create as light
	{
		for ( auto& l : graph.lights ) {
			if ( l.name != node.name ) continue;
		#if UF_ENV_DREAMCAST
			metadata.system.ignoreGraph = graph.metadata["debug"]["static"].as<bool>();
		#endif
			uf::Serializer metadataLight;
			metadataLight["radius"][0] = 0.001;
			metadataLight["radius"][1] = l.range; // l.range <= 0.001f ? graph.metadata["lights"]["range cap"].as<float>() : l.range;
			metadataLight["power"] = l.intensity; //  * graph.metadata["lights"]["power scale"].as<float>();
			metadataLight["color"][0] = l.color.x;
			metadataLight["color"][1] = l.color.y;
			metadataLight["color"][2] = l.color.z;
			metadataLight["shadows"] = graph.metadata["lights"]["shadows"].as<bool>();
			if ( metadataLight["shadows"].as<bool>() ) {
				metadataLight["radius"][1] = 0;
			}
			if ( ext::json::isArray( graph.metadata["lights"]["radius"] ) ) {
				metadataLight["radius"] = graph.metadata["lights"]["radius"];
			}
			if ( graph.metadata["lights"]["bias"].is<float>() ) {
				metadataLight["bias"] = graph.metadata["lights"]["bias"].as<float>();
			}
			// copy from tag information
			ext::json::forEach( graph.metadata["tags"][node.name]["light"], [&]( const std::string& key, ext::json::Value& value ){
				if ( key == "transform" ) return;
				metadataLight[key] = value;
			});
			if ( graph.metadata["lightmapped"].as<bool>() && !(metadataLight["shadows"].as<bool>() || metadataLight["dynamic"].as<bool>()) ) continue;
			auto& metadataJson = entity.getComponent<uf::Serializer>();
			entity.load("/light.json");
			// copy
			ext::json::forEach( metadataLight, [&]( const std::string& key, ext::json::Value& value ) {
				metadataJson["light"][key] = value;
			});
			break;
		}
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
	if ( 0 <= node.mesh && node.mesh < graph.meshes.size() ) {
		auto& nodeMesh = graph.meshes[node.mesh];
		// preprocess mesh
		for ( auto& vertex : nodeMesh.vertices ) {
			vertex.id.x = node.index;
		}

		auto& mesh = entity.getComponent<ext::gltf::mesh_t>();
		mesh.insert( nodeMesh );

		if ( !(graph.metadata["flags"]["SEPARATE"].as<bool>()) ) {
			auto& mesh = graph.root.entity->getComponent<ext::gltf::mesh_t>();
			mesh.insert( nodeMesh );
		} else if ( graph.metadata["flags"]["RENDER"].as<bool>() ) {
			uf::instantiator::bind("RenderBehavior", entity);
			initializeGraphics( graph, entity );
			auto& graphic = entity.getComponent<uf::Graphic>();
			// Joints storage buffer
			if ( graph.metadata["flags"]["SKINNED"].as<bool>() && node.skin >= 0 ) {
				auto& skin = graph.skins[node.skin];
				node.jointBufferIndex = graphic.initializeBuffer(
					(void*) skin.inverseBindMatrices.data(),
					skin.inverseBindMatrices.size() * sizeof(pod::Matrix4f),
					uf::renderer::enums::Buffer::STORAGE
				);
			}
			// Materials storage buffer
			std::vector<pod::Material::Storage> materials( graph.materials.size() );
			for ( size_t i = 0; i < graph.materials.size(); ++i ) {
				materials[i] = graph.materials[i].storage;
			}
			node.materialBufferIndex = graphic.initializeBuffer(
				(void*) materials.data(),
				materials.size() * sizeof(pod::Material::Storage),
				uf::renderer::enums::Buffer::STORAGE
			);
			// Texture storage buffer
			std::vector<pod::Texture::Storage> textures( graph.textures.size() );
			for ( size_t i = 0; i < graph.textures.size(); ++i ) {
				textures[i] = graph.textures[i].storage;
			//	textures[i].remap = -1;
			}
			graph.root.textureBufferIndex = graphic.initializeBuffer(
				(void*) textures.data(),
				textures.size() * sizeof(pod::Texture::Storage),
				uf::renderer::enums::Buffer::STORAGE
			);
		}
	}
	
	for ( auto index : node.children ) uf::graph::process( graph, index, entity );
}
void uf::graph::cleanup( pod::Graph& graph ) {
	graph.images.clear();
	graph.meshes.clear();
	graph.atlas.clear();
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

void uf::graph::initialize( pod::Graph& graph ) {
	graph.root.entity->initialize();
	graph.root.entity->process([&]( uf::Entity* entity ) {
		if ( !entity->hasComponent<uf::Graphic>() ) {
			if ( entity->getUid() == 0 ) entity->initialize();
			return;
		}
		if ( !graph.metadata["flags"]["LOAD"].as<bool>() ) initializeShaders( graph, entity->as<uf::Object>() );
		uf::instantiator::bind( "GltfBehavior", *entity );
		uf::instantiator::unbind( "RenderBehavior", *entity );
		if ( entity->getUid() == 0 ) entity->initialize();
	});
}

void uf::graph::animate( pod::Graph& graph, const std::string& name, float speed, bool immediate ) {
	if ( !(graph.metadata["flags"]["SKINNED"].as<bool>()) ) return;
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
	if ( !(graph.metadata["flags"]["SKINNED"].as<bool>()) ) return;
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
			if ( node.jointBufferIndex < graphic.buffers.size() ) {
				auto& buffer = graphic.buffers.at(node.jointBufferIndex);
				graphic.updateBuffer( (void*) joints.data(), joints.size() * sizeof(pod::Matrix4f), buffer );
			}
		}
	}
}
void uf::graph::destroy( pod::Graph& graph ) {
	for ( auto& t : graph.textures ) {
		t.texture.destroy();
	}
}

#include <uf/utils/string/base64.h>

namespace {
	uf::Image decodeImage( ext::json::Value& json ) {
		uf::Image image;
		auto size = uf::vector::decode( json["size"], pod::Vector2ui{} );
		size_t bpp = json["bpp"].as<size_t>();
		size_t channels = json["channels"].as<size_t>();
		auto pixels = uf::base64::decode( json["data"].as<std::string>() );
		image.loadFromBuffer( &pixels[0], size, bpp, channels, true );
		return image;
	}
	uf::Image& decode( ext::json::Value& json, uf::Image& image ) {
		return image = decodeImage( json );
	}

	pod::Texture decodeTexture( ext::json::Value& json ) {
		pod::Texture texture;
		texture.name = json["name"].as<std::string>();
		texture.storage.index = json["index"].is<size_t>() ? json["index"].as<size_t>() : -1;
		texture.storage.sampler = json["sampler"].is<size_t>() ? json["sampler"].as<size_t>() : -1;
		texture.storage.remap = json["remap"].is<size_t>() ? json["remap"].as<size_t>() : -1;
		texture.storage.blend = json["blend"].is<size_t>() ? json["blend"].as<size_t>() : -1;
		texture.storage.lerp = uf::vector::decode( json["lerp"], pod::Vector4f{} );
		return texture;
	}
	pod::Texture& decode( ext::json::Value& json, pod::Texture& texture ) {
		return texture = decodeTexture( json );
	}

	uf::renderer::Sampler decodeSampler( ext::json::Value& json ) {
		uf::renderer::Sampler sampler;
		sampler.descriptor.filter.min = (uf::renderer::enums::Filter::type_t) json["min"].as<size_t>();
		sampler.descriptor.filter.mag = (uf::renderer::enums::Filter::type_t) json["mag"].as<size_t>();
		sampler.descriptor.addressMode.u = (uf::renderer::enums::AddressMode::type_t) json["u"].as<size_t>();
		sampler.descriptor.addressMode.v = (uf::renderer::enums::AddressMode::type_t) json["v"].as<size_t>();
		sampler.descriptor.addressMode.w = sampler.descriptor.addressMode.v;
		return sampler;
	}
	uf::renderer::Sampler& decode( ext::json::Value& json, uf::renderer::Sampler& sampler ) {
		return sampler = decodeSampler( json );
	}

	pod::Material decodeMaterial( ext::json::Value& json ) {
		pod::Material material;
		material.name = json["name"].as<std::string>();
		material.alphaMode = json["alphaMode"].as<std::string>();
		material.storage.colorBase = uf::vector::decode( json["base"], pod::Vector4f{} );
		material.storage.colorEmissive = uf::vector::decode( json["emissive"], pod::Vector4f{} );
		material.storage.factorMetallic = json["fMetallic"].as<float>();
		material.storage.factorRoughness = json["fRoughness"].as<float>();
		material.storage.factorOcclusion = json["fOcclusion"].as<float>();
		material.storage.factorAlphaCutoff = json["fAlphaCutoff"].as<float>();
		material.storage.indexAlbedo = json["iAlbedo"].is<size_t>() ? json["iAlbedo"].as<size_t>() : -1;
		material.storage.indexNormal = json["iNormal"].is<size_t>() ? json["iNormal"].as<size_t>() : -1;
		material.storage.indexEmissive = json["iEmissive"].is<size_t>() ? json["iEmissive"].as<size_t>() : -1;
		material.storage.indexOcclusion = json["iOcclusion"].is<size_t>() ? json["iOcclusion"].as<size_t>() : -1;
		material.storage.indexMetallicRoughness = json["iMetallicRoughness"].is<size_t>() ? json["iMetallicRoughness"].as<size_t>() : -1;
		material.storage.modeAlpha = json["modeAlpha"].as<size_t>();
		return material;
	}
	pod::Material& decode( ext::json::Value& json, pod::Material& material ) {
		return material = decodeMaterial( json );
	}

	pod::Light decodeLight( ext::json::Value& json ) {
		pod::Light light;
		light.name = json["name"].as<std::string>();
		light.color = uf::vector::decode( json["color"], pod::Vector3f{} );
		light.intensity = json["intensity"].as<float>();
		light.range = json["range"].as<float>();
		return light;
	}
	pod::Light& decode( ext::json::Value& json, pod::Light& light ) {
		return light = decodeLight( json );
	}

	pod::Animation decodeAnimation( ext::json::Value& json ) {
		pod::Animation animation;
		animation.name = json["name"].as<std::string>();
		animation.start = json["start"].as<float>();
		animation.end = json["end"].as<float>();

		ext::json::forEach( json["samplers"], [&]( ext::json::Value& value ){
			auto& sampler = animation.samplers.emplace_back();
			sampler.interpolator = value["interpolator"].as<std::string>();

			sampler.inputs.reserve( value["inputs"].size() );
			ext::json::forEach( value["inputs"], [&]( ext::json::Value& input ){
				sampler.inputs.emplace_back( input.as<float>() );
			});
			
			sampler.outputs.reserve( value["outputs"].size() );
			ext::json::forEach( value["outputs"], [&]( ext::json::Value& output ){
				sampler.outputs.emplace_back( uf::vector::decode( output, pod::Vector4f{} ) );
			});
		});

		ext::json::forEach( json["channels"], [&]( ext::json::Value& value ){
			auto& channel = animation.channels.emplace_back();
			channel.path = value["path"].as<std::string>();
			channel.node = value["node"].as<int32_t>();
			channel.sampler = value["sampler"].as<uint32_t>();
		});
		return animation;
	}
	pod::Animation& decode( ext::json::Value& json, pod::Animation& animation ) {
		return animation = decodeAnimation( json );
	}

	pod::Skin decodeSkin( ext::json::Value& json ) {
		pod::Skin skin;
		skin.name = json["name"].as<std::string>();
		
		skin.joints.reserve( json["joints"].size() );
		ext::json::forEach( json["joints"], [&]( ext::json::Value& value ){
			skin.joints.emplace_back( value.as<int32_t>() );
		});
		
		skin.inverseBindMatrices.reserve( json["inverseBindMatrices"].size() );
		ext::json::forEach( json["inverseBindMatrices"], [&]( ext::json::Value& value ){
			skin.inverseBindMatrices.emplace_back( uf::matrix::decode( value, pod::Matrix4f{} ) );
		});
		
		return skin;
	}
	pod::Skin& decode( ext::json::Value& json, pod::Skin& skin ) {
		return skin = decodeSkin( json );
	}
	pod::Graph::Mesh decodeMesh( ext::json::Value& json ) {
		pod::Graph::Mesh mesh;
		mesh.vertices.reserve( json["vertices"].size() );
		ext::json::forEach( json["vertices"], [&]( ext::json::Value& value ){
			auto& vertex = mesh.vertices.emplace_back();
			size_t attr = 0;
			vertex.position = uf::vector::decode( value[attr++], pod::Vector3f{} );
			vertex.uv = uf::vector::decode( value[attr++], pod::Vector2f{} );
			vertex.st = uf::vector::decode( value[attr++], pod::Vector2f{} );
			vertex.normal = uf::vector::decode( value[attr++], pod::Vector3f{} );
			vertex.tangent = uf::vector::decode( value[attr++], pod::Vector4f{} );
			vertex.id = uf::vector::decode( value[attr++], pod::Vector2ui{} );
			vertex.joints = uf::vector::decode( value[attr++], pod::Vector4ui{} );
			vertex.weights = uf::vector::decode( value[attr++], pod::Vector4f{} );
		});
		mesh.indices.reserve( json["indices"].size() );
		ext::json::forEach( json["indices"], [&]( ext::json::Value& value ){
			mesh.indices.emplace_back( value.as<size_t>() );
		});
		return mesh;
	}
	pod::Graph::Mesh& decode( ext::json::Value& json, pod::Graph::Mesh& mesh ) {
		return mesh = decodeMesh( json );
	}

	pod::Node decodeNode( ext::json::Value& json ) {
		pod::Node node;
		node.name = json["name"].as<std::string>();
		node.index = json["index"].as<int32_t>();
		node.skin = json["skin"].is<int32_t>() ? json["skin"].as<int32_t>() : -1;
		node.mesh = json["mesh"].is<int32_t>() ? json["mesh"].as<int32_t>() : -1;
		node.parent = json["parent"].is<int32_t>() ? json["parent"].as<int32_t>() : -1;
		node.children.reserve( json["children"].size() );
		ext::json::forEach( json["children"], [&]( ext::json::Value& value ){
			node.children.emplace_back( value.as<int32_t>() );
		});
		node.transform = uf::transform::decode( json["transform"], pod::Transform<>{} );
		return node;
	}
	pod::Node& decode( ext::json::Value& json, pod::Node& node ) {
		return node = decodeNode( json );
	}
}

pod::Graph uf::graph::load( const std::string& filename, ext::gltf::load_mode_t mode, const uf::Serializer& metadata ) {
#if UF_ENV_DREAMCAST
	#define UF_DEBUG_TRACE_MSG(X) UF_DEBUG_MSG(X); //malloc_stats();
	uf::Timer<long long> timer(false);
	if ( !timer.running() ) timer.start();
#else
	#define UF_DEBUG_TRACE_MSG(X)
#endif
	std::string directory = uf::io::directory( filename ) + "/";
	pod::Graph graph;
	uf::Serializer serializer;
	UF_DEBUG_TRACE_MSG("Reading " << filename);
	serializer.readFromFile( filename );
	// load metadata
	graph.name = filename; //serializer["name"].as<std::string>();
	graph.mode = mode; // serializer["mode"].as<size_t>();
	graph.metadata = metadata; // serializer["metadata"];
#if UF_GRAPH_LOAD_MULTITHREAD
	std::vector<std::function<int()>> jobs;
	jobs.emplace_back([&]{
		// load mesh information
		UF_DEBUG_TRACE_MSG("Reading meshes...");
		graph.meshes.reserve( serializer["meshes"].size() );
		ext::json::forEach( serializer["meshes"], [&]( ext::json::Value& value ){
			auto& mesh = graph.meshes.emplace_back();
			if ( value.is<std::string>() ) {
				UF_DEBUG_TRACE_MSG(directory + "/" + value.as<std::string>());
				uf::Serializer json;
				json.readFromFile( directory + "/" + value.as<std::string>() );
				decode( json, mesh );
			
			} else {
				decode( value, mesh );
			}
		});
		return 0;
	});
	jobs.emplace_back([&]{
		if ( !ext::json::isNull( serializer["atlas"] ) ) {
			UF_DEBUG_TRACE_MSG("Reading atlas...");
			auto& image = graph.atlas.getAtlas();
			auto& value = serializer["atlas"];
			if ( value.is<std::string>() ) {
				std::string filename = directory + "/" + value.as<std::string>();
				UF_DEBUG_TRACE_MSG("Reading atlas " << filename);
				image.open(filename, false);
			} else {
				decode( value, image );
			}
		}
		// load images
		UF_DEBUG_TRACE_MSG("Reading images...");
		graph.images.reserve( serializer["images"].size() );
		ext::json::forEach( serializer["images"], [&]( ext::json::Value& value ){
			auto& image = graph.images.emplace_back();
			if ( value.is<std::string>() ) {
				std::string filename = directory + "/" + value.as<std::string>();
				UF_DEBUG_TRACE_MSG("Reading image " << filename);
				image.open(filename, false);
			} else {
				decode( value, image );
			}
		});
		return 0;
	});
	jobs.emplace_back([&]{
		// load texture information
		UF_DEBUG_TRACE_MSG("Reading texture information...");
		graph.textures.reserve( serializer["textures"].size() );
		ext::json::forEach( serializer["textures"], [&]( ext::json::Value& value ){
			decode( value, graph.textures.emplace_back() );
		});
		return 0;
	});
	jobs.emplace_back([&]{
		// load sampler information
		UF_DEBUG_TRACE_MSG("Reading sampler information...");
		graph.samplers.reserve( serializer["samplers"].size() );
		ext::json::forEach( serializer["samplers"], [&]( ext::json::Value& value ){
			decode( value, graph.samplers.emplace_back() );
		});
		return 0;
	});
	jobs.emplace_back([&]{
		// load material information
		UF_DEBUG_TRACE_MSG("Reading material information...");
		graph.materials.reserve( serializer["materials"].size() );
		ext::json::forEach( serializer["materials"], [&]( ext::json::Value& value ){
			decode( value, graph.materials.emplace_back() );
		});
		return 0;
	});
	jobs.emplace_back([&]{
		// load light information
		UF_DEBUG_TRACE_MSG("Reading lighting information...");
		graph.lights.reserve( serializer["lighting"].size() );
		ext::json::forEach( serializer["lighting"], [&]( ext::json::Value& value ){
			decode( value, graph.lights.emplace_back() );
		});
		return 0;
	});
	jobs.emplace_back([&]{
		// load animation information
		UF_DEBUG_TRACE_MSG("Reading animation information...");
		graph.animations.reserve( serializer["animations"].size() );
		ext::json::forEach( serializer["animations"], [&]( ext::json::Value& value ){
			if ( value.is<std::string>() ) {
				uf::Serializer json;
				json.readFromFile( directory + "/" + value.as<std::string>() );
				std::string key = json["name"].as<std::string>();
				decode( json, graph.animations[key] );
			} else {
				std::string key = value["name"].as<std::string>();
				decode( value, graph.animations[key] );
			}
		});
		return 0;
	});
	jobs.emplace_back([&]{
		// load skin information
		UF_DEBUG_TRACE_MSG("Reading skinning information...");
		graph.skins.reserve( serializer["skins"].size() );
		ext::json::forEach( serializer["skins"], [&]( ext::json::Value& value ){
			decode( value, graph.skins.emplace_back() );
		});
		return 0;
	});
	jobs.emplace_back([&]{
		// load node information
		UF_DEBUG_TRACE_MSG("Reading nodes...");
		graph.nodes.reserve( serializer["nodes"].size() );
		ext::json::forEach( serializer["nodes"], [&]( ext::json::Value& value ){
			decode( value, graph.nodes.emplace_back() );
		});
		decode(serializer["root"], graph.root);
		return 0;
	});
	if ( !jobs.empty() ) uf::thread::batchWorkers( jobs );
#else
	// load mesh information
	UF_DEBUG_TRACE_MSG("Reading meshes...");
	graph.meshes.reserve( serializer["meshes"].size() );
	ext::json::forEach( serializer["meshes"], [&]( ext::json::Value& value ){
		auto& mesh = graph.meshes.emplace_back();
		if ( value.is<std::string>() ) {
			UF_DEBUG_TRACE_MSG(directory + "/" + value.as<std::string>());
			uf::Serializer json;
			json.readFromFile( directory + "/" + value.as<std::string>() );
			decode( json, mesh );
		
		} else {
			decode( value, mesh );
		}
	});
	// load images
	if ( !ext::json::isNull( serializer["atlas"] ) ) {
		UF_DEBUG_TRACE_MSG("Reading atlas...");
		auto& image = graph.atlas.getAtlas();
		auto& value = serializer["atlas"];
		if ( value.is<std::string>() ) {
			std::string filename = directory + "/" + value.as<std::string>();
			UF_DEBUG_TRACE_MSG("Reading atlas " << filename);
			image.open(filename, false);
		} else {
			decode( value, image );
		}
	}
	UF_DEBUG_TRACE_MSG("Reading images...");
	graph.images.reserve( serializer["images"].size() );
	ext::json::forEach( serializer["images"], [&]( ext::json::Value& value ){
		auto& image = graph.images.emplace_back();
		if ( value.is<std::string>() ) {
			std::string filename = directory + "/" + value.as<std::string>();
			UF_DEBUG_TRACE_MSG("Reading image " << filename);
			image.open(filename, false);
		} else {
			decode( value, image );
		}
	});
	// load texture information
	UF_DEBUG_TRACE_MSG("Reading texture information...");
	graph.textures.reserve( serializer["textures"].size() );
	ext::json::forEach( serializer["textures"], [&]( ext::json::Value& value ){
		decode( value, graph.textures.emplace_back() );
	});
	// load sampler information
	UF_DEBUG_TRACE_MSG("Reading sampler information...");
	graph.samplers.reserve( serializer["samplers"].size() );
	ext::json::forEach( serializer["samplers"], [&]( ext::json::Value& value ){
		decode( value, graph.samplers.emplace_back() );
	});
	// load material information
	UF_DEBUG_TRACE_MSG("Reading material information...");
	graph.materials.reserve( serializer["materials"].size() );
	ext::json::forEach( serializer["materials"], [&]( ext::json::Value& value ){
		decode( value, graph.materials.emplace_back() );
	});
	// load light information
	UF_DEBUG_TRACE_MSG("Reading lighting information...");
	graph.lights.reserve( serializer["lighting"].size() );
	ext::json::forEach( serializer["lighting"], [&]( ext::json::Value& value ){
		decode( value, graph.lights.emplace_back() );
	});
	// load animation information
	UF_DEBUG_TRACE_MSG("Reading animation information...");
	graph.animations.reserve( serializer["animations"].size() );
	ext::json::forEach( serializer["animations"], [&]( ext::json::Value& value ){
		if ( value.is<std::string>() ) {
			uf::Serializer json;
			json.readFromFile( directory + "/" + value.as<std::string>() );
			std::string key = json["name"].as<std::string>();
			decode( json, graph.animations[key] );
		} else {
			std::string key = value["name"].as<std::string>();
			decode( value, graph.animations[key] );
		}
	});
	// load skin information
	UF_DEBUG_TRACE_MSG("Reading skinning information...");
	graph.skins.reserve( serializer["skins"].size() );
	ext::json::forEach( serializer["skins"], [&]( ext::json::Value& value ){
		decode( value, graph.skins.emplace_back() );
	});
	// load node information
	UF_DEBUG_TRACE_MSG("Reading nodes...");
	graph.nodes.reserve( serializer["nodes"].size() );
	ext::json::forEach( serializer["nodes"], [&]( ext::json::Value& value ){
		decode( value, graph.nodes.emplace_back() );
	});
	decode(serializer["root"], graph.root);
#endif
#if 0
	// generate atlas
	if ( graph.metadata["flags"]["ATLAS"].as<bool>() ) {
		UF_DEBUG_TRACE_MSG("Generating atlas...");
		graph.atlas.generate( graph.images );
		graph.atlas.clear(false);
	}
#endif
	// re-reference all transform parents
	for ( auto& node : graph.nodes ) {
		if ( 0 <= node.parent && node.parent < graph.nodes.size() && node.index != node.parent ) {
			node.transform.reference = &graph.nodes[node.parent].transform;
		}
	}
	UF_DEBUG_TRACE_MSG("Processing graph...");
	return graph;
}

namespace {
	uf::Serializer encode( const uf::Image& image, bool compress = false ) {
		uf::Serializer json;
		json["size"] = uf::vector::encode( image.getDimensions() );
		json["bpp"] = image.getBpp() / image.getChannels();
		json["channels"] = image.getChannels();
		json["data"] = uf::base64::encode( image.getPixels() );
		return json;
	}
	uf::Serializer encode( const pod::Texture& texture, bool compress = false ) {
		uf::Serializer json;
		json["name"] = texture.name;
		json["index"] = texture.storage.index;
		json["sampler"] = texture.storage.sampler;
		json["remap"] = texture.storage.remap;
		json["blend"] = texture.storage.blend;
		json["lerp"] = uf::vector::encode(texture.storage.lerp);
		return json;
	}
	uf::Serializer encode( const uf::renderer::Sampler& sampler, bool compress = false ) {
		uf::Serializer json;
		json["min"] = sampler.descriptor.filter.min;
		json["mag"] = sampler.descriptor.filter.mag;
		json["u"] = sampler.descriptor.addressMode.u;
		json["v"] = sampler.descriptor.addressMode.v;
		return json;
	}
	uf::Serializer encode( const pod::Material& material, bool compress = false ) {
		uf::Serializer json;
		json["name"] = material.name;
		json["alphaMode"] = material.alphaMode;
		json["base"] = uf::vector::encode( material.storage.colorBase );
		json["emissive"] = uf::vector::encode( material.storage.colorEmissive );
		json["fMetallic"] = material.storage.factorMetallic;
		json["fRoughness"] = material.storage.factorRoughness;
		json["fOcclusion"] = material.storage.factorOcclusion;
		json["fAlphaCutoff"] = material.storage.factorAlphaCutoff;
		if ( material.storage.indexAlbedo >= 0 ) json["iAlbedo"] = material.storage.indexAlbedo;
		if ( material.storage.indexNormal >= 0 ) json["iNormal"] = material.storage.indexNormal;
		if ( material.storage.indexEmissive >= 0 ) json["iEmissive"] = material.storage.indexEmissive;
		if ( material.storage.indexOcclusion >= 0 ) json["iOcclusion"] = material.storage.indexOcclusion;
		if ( material.storage.indexMetallicRoughness >= 0 ) json["iMetallicRoughness"] = material.storage.indexMetallicRoughness;
		json["modeAlpha"] = material.storage.modeAlpha;
		return json;
	}
	uf::Serializer encode( const pod::Light& light, bool compress = false ) {
		uf::Serializer json;
		json["name"] = light.name;
		json["color"] = uf::vector::encode( light.color );
		json["intensity"] = light.intensity;
		json["range"] = light.range;
		return json;
	}
	uf::Serializer encode( const pod::Animation& animation, bool compress = false ) {
		uf::Serializer json;
		json["name"] = animation.name;
		json["start"] = animation.start;
		json["end"] = animation.end;

		ext::json::reserve( json["samplers"], animation.samplers.size() );
		auto& samplers = json["samplers"];
		for ( auto& sampler : animation.samplers ) {
			auto& json = samplers.emplace_back();
			json["interpolator"] = sampler.interpolator;
			for ( auto& input : sampler.inputs ) json["inputs"].emplace_back(input);
			for ( auto& output : sampler.outputs ) {
				json["outputs"].emplace_back(uf::vector::encode( output, compress ));
			}
		}
		ext::json::reserve( json["channels"], animation.channels.size() );
		auto& channels = json["channels"];
		for ( auto& channel : animation.channels ) {
			auto& json = channels.emplace_back();
			json["path"] = channel.path;
			json["node"] = channel.node;
			json["sampler"] = channel.sampler;
		}
		return json;
	}	
	uf::Serializer encode( const pod::Skin& skin, bool compress = false ) {
		uf::Serializer json;
		json["name"] = skin.name;

		ext::json::reserve( json["joints"], skin.joints.size() );
		for ( auto& joint : skin.joints ) json["joints"].emplace_back( joint );

		ext::json::reserve( json["inverseBindMatrices"], skin.inverseBindMatrices.size() );
		for ( auto& inverseBindMatrix : skin.inverseBindMatrices )
			json["inverseBindMatrices"].emplace_back( uf::matrix::encode(inverseBindMatrix, compress) );
		return json;
	}	
	uf::Serializer encode( const pod::Graph::Mesh& mesh, bool compress = false ) {
		uf::Serializer json;
		ext::json::reserve( json["attributes"], mesh.attributes.descriptor.size() );
		for ( auto& attribute : mesh.attributes.descriptor ) {
			auto& a = json["attributes"].emplace_back();
			a["name"] = attribute.name;
			a["size"] = attribute.size;
			a["type"] = attribute.type;
			a["format"] = attribute.format;
			a["offset"] = attribute.offset;
			a["components"] = attribute.components;
		}
		ext::json::reserve( json["vertices"], mesh.vertices.size() );
		for ( auto& vertex : mesh.vertices ) {
			auto& v = json["vertices"].emplace_back();
			v.emplace_back(!vertex.position ? ext::json::array() : uf::vector::encode(vertex.position, compress));
			v.emplace_back(!vertex.uv ? ext::json::array() : uf::vector::encode(vertex.uv, compress));
			v.emplace_back(!vertex.st ? ext::json::array() : uf::vector::encode(vertex.st, compress));
			v.emplace_back(!vertex.normal ? ext::json::array() : uf::vector::encode(vertex.normal, compress));
			v.emplace_back(!vertex.tangent ? ext::json::array() : uf::vector::encode(vertex.tangent, compress));
			v.emplace_back(!vertex.id ? ext::json::array() : uf::vector::encode(vertex.id, compress));
			v.emplace_back(!vertex.joints ? ext::json::array() : uf::vector::encode(vertex.joints, compress));
			v.emplace_back(!vertex.weights ? ext::json::array() : uf::vector::encode(vertex.weights, compress));
		}
		ext::json::reserve( json["indices"], mesh.indices.size() );
		for ( auto& index : mesh.indices ) json["indices"].emplace_back(index);
		return json;
	}
	uf::Serializer encode( const pod::Node& node, bool compress = false ) {
		uf::Serializer json;
		json["name"] = node.name;
		json["index"] = node.index;
		if ( node.skin >= 0 ) json["skin"] = node.skin;
		if ( node.mesh >= 0 ) json["mesh"] = node.mesh;
		if ( node.parent >= 0 ) json["parent"] = node.parent;
		ext::json::reserve( json["children"], node.children.size() );
		for ( auto& child : node.children ) json["children"].emplace_back(child);

		json["transform"] = uf::transform::encode( node.transform, false, compress );
		return json;
	}
}
void uf::graph::save( const pod::Graph& graph, const std::string& filename ) {
	std::string directory = uf::io::directory( filename ) + "/" + uf::io::basename(filename) + "/";
	std::string target = uf::io::directory( filename ) + "/" + uf::io::basename(filename) + ".graph";

	uf::Serializer serializer;
	uf::Serializer metadata;
	// store metadata
/*
	serializer["name"] = graph.name;
	serializer["mode"] = graph.mode;
	serializer["metadata"] = graph.metadata;
	serializer["metadata"]["mesh optimization"] = 0;
*/
	bool saveSeparately = !metadata["debug"]["export"]["combined"].as<bool>();
	bool compression = metadata["debug"]["export"]["compression"].as<bool>();
	// store images
	if ( saveSeparately ) uf::io::mkdir(directory);

#if UF_GRAPH_LOAD_MULTITHREAD
	std::vector<std::function<int()>> jobs;
	jobs.emplace_back([&]{
		if ( graph.atlas.generated() ) {
			auto& image = graph.atlas.getAtlas();
			if ( saveSeparately ) {
				std::string f = "atlas";
				f += compression?".jpg":".png";
				image.save(directory + "/" + f);
				serializer["atlas"] = f;
			} else {
				serializer["atlas"] = encode(image, compression);
			}
		}
		ext::json::reserve( serializer["images"], graph.images.size() );
		if ( saveSeparately ) {
			for ( size_t i = 0; i < graph.images.size(); ++i ) {
				std::string f = "image."+std::to_string(i)+(compression?".jpg":".png");
				graph.images[i].save(directory + "/" + f);
				serializer["images"].emplace_back(f);
			}
		} else {
			for ( auto& image : graph.images ) serializer["images"].emplace_back( encode(image, compression) );
		}
		return 0;
	});
	jobs.emplace_back([&]{
		// store texture information
		ext::json::reserve( serializer["textures"], graph.textures.size() );
		for ( auto& texture : graph.textures ) serializer["textures"].emplace_back( encode(texture, compression) );
		return 0;
	});
	jobs.emplace_back([&]{
		// store sampler information
		ext::json::reserve( serializer["samplers"], graph.samplers.size() );
		for ( auto& sampler : graph.samplers ) serializer["samplers"].emplace_back( encode(sampler, compression) );
		return 0;
	});
	jobs.emplace_back([&]{
		// store material information
		ext::json::reserve( serializer["materials"], graph.materials.size() );
		for ( auto& material : graph.materials ) serializer["materials"].emplace_back( encode(material, compression) );
		return 0;
	});
	jobs.emplace_back([&]{
		// store light information
		ext::json::reserve( serializer["lighting"], graph.lights.size() );
		for ( auto& light : graph.lights ) serializer["lighting"].emplace_back( encode(light, compression) );
		return 0;
	});
	jobs.emplace_back([&]{
		// store animation information
		ext::json::reserve( serializer["animations"], graph.animations.size() );
		if ( saveSeparately ) {
			for ( auto pair : graph.animations ) {
				std::string f = "animation."+pair.first+".json";
				encode(pair.second, compression).writeToFile(directory+"/" + f);
				serializer["animations"].emplace_back(f);
			}
		} else {
			for ( auto pair : graph.animations ) serializer["animations"][pair.first] = encode(pair.second, compression);
		}
		return 0;
	});
	jobs.emplace_back([&]{
		// store skin information
		ext::json::reserve( serializer["skins"], graph.skins.size() );
		for ( auto& skin : graph.skins ) serializer["skins"].emplace_back( encode(skin, compression) );
		return 0;
	});
	jobs.emplace_back([&]{
		// store mesh information
		ext::json::reserve( serializer["meshes"], graph.meshes.size() );
		if ( saveSeparately ) {
			for ( size_t i = 0; i < graph.meshes.size(); ++i ) {
				std::string f = "mesh."+std::to_string(i)+".json";
				encode(graph.meshes[i], compression).writeToFile(directory+"/" + f);
				serializer["meshes"].emplace_back(f);
			}
		} else {
			for ( auto& mesh : graph.meshes ) serializer["meshes"].emplace_back( encode(mesh, compression) );
		}
		return 0;
	});
	jobs.emplace_back([&]{
		// store node information
		ext::json::reserve( serializer["nodes"], graph.nodes.size() );
		for ( auto& node : graph.nodes ) serializer["nodes"].emplace_back( encode(node, compression) );
		serializer["root"] = encode(graph.root, compression);
		return 0;
	});
	if ( !jobs.empty() ) uf::thread::batchWorkers( jobs );
#else
	if ( graph.atlas.generated() ) {
		auto& image = graph.atlas.getAtlas();
		if ( saveSeparately ) {
			std::string f = "atlas"+(compression?".jpg":".png");
			image.save(directory + "/" + f);
			serializer["atlas"] = f;
		} else {
			serializer["atlas"] = encode(image, compression);
		}
	}
	ext::json::reserve( serializer["images"], graph.images.size() );
	if ( saveSeparately ) {
		for ( size_t i = 0; i < graph.images.size(); ++i ) {
			std::string f = "image."+std::to_string(i)+(compression?".jpg":".png");
			graph.images[i].save(directory + "/" + f);
			serializer["images"].emplace_back(f);
		}
	} else {
		for ( auto& image : graph.images ) serializer["images"].emplace_back( encode(image, compression) );
	}
	// store texture information
	ext::json::reserve( serializer["textures"], graph.textures.size() );
	for ( auto& texture : graph.textures ) serializer["textures"].emplace_back( encode(texture, compression) );
	// store sampler information
	ext::json::reserve( serializer["samplers"], graph.samplers.size() );
	for ( auto& sampler : graph.samplers ) serializer["samplers"].emplace_back( encode(sampler, compression) );
	// store material information
	ext::json::reserve( serializer["materials"], graph.materials.size() );
	for ( auto& material : graph.materials ) serializer["materials"].emplace_back( encode(material, compression) );
	// store light information
	ext::json::reserve( serializer["lighting"], graph.lights.size() );
	for ( auto& light : graph.lights ) serializer["lighting"].emplace_back( encode(light, compression) );
	// store animation information
	ext::json::reserve( serializer["animations"], graph.animations.size() );
	if ( saveSeparately ) {
		for ( auto pair : graph.animations ) {
			std::string f = "animation."+pair.first+".json";
			encode(pair.second, compression).writeToFile(directory+"/" + f);
			serializer["animations"].emplace_back(f);
		}
	} else {
		for ( auto pair : graph.animations ) serializer["animations"][pair.first] = encode(pair.second, compression);
	}
	// store skin information
	ext::json::reserve( serializer["skins"], graph.skins.size() );
	for ( auto& skin : graph.skins ) serializer["skins"].emplace_back( encode(skin, compression) );
	// store mesh information
	ext::json::reserve( serializer["meshes"], graph.meshes.size() );
	if ( saveSeparately ) {
		for ( size_t i = 0; i < graph.meshes.size(); ++i ) {
			std::string f = "mesh."+std::to_string(i)+".json";
			encode(graph.meshes[i], compression).writeToFile(directory+"/" + f);
			serializer["meshes"].emplace_back(f);
		}
	} else {
		for ( auto& mesh : graph.meshes ) serializer["meshes"].emplace_back( encode(mesh, compression) );
	}
	// store node information
	ext::json::reserve( serializer["nodes"], graph.nodes.size() );
	for ( auto& node : graph.nodes ) serializer["nodes"].emplace_back( encode(node, compression) );
	serializer["root"] = encode(graph.root, compression);
#endif

	if ( saveSeparately ) target = directory + "/graph.json";
	serializer.writeToFile( target );
}

std::string uf::graph::print( const pod::Graph& graph ) {
	std::stringstream ss;
	ss << "Graph Data:"
		"\n\tImages: " << graph.images.size() << ""
		"\n\tTextures: " << graph.textures.size() << ""
		"\n\tMaterials: " << graph.materials.size() << ""
		"\n\tLights: " << graph.lights.size() << ""
		"\n\tMeshes: " << graph.meshes.size() << ""
		"\n\tAnimations: " << graph.animations.size() << ""
		"\n\tNodes: " << graph.nodes.size() << ""
		"\n";
	ss << "Graph Tree: \n";
	std::function<void(const pod::Node&, size_t)> print = [&]( const pod::Node& node, size_t indent ) {
		for ( size_t i = 0; i < indent; ++i ) ss << "\t";
		ss << "Node[" << node.index << "] " << node.name << ":\n";
		for ( auto index : node.children ) print( graph.nodes[index], indent + 1 );
	};
	print( graph.root, 1 );
	return ss.str();
}
uf::Serializer uf::graph::stats( const pod::Graph& graph ) {
	ext::json::Value json;

	size_t memoryTextures = sizeof(pod::Texture::Storage) * graph.textures.size();
	size_t memoryMaterials = sizeof(pod::Material::Storage) * graph.materials.size();
	size_t memoryLights = sizeof(pod::Light) * graph.lights.size();
	size_t memoryImages = 0;
	size_t memoryMeshes = 0;
	size_t memoryAnimations = 0;
	size_t memoryNodes = 0;
	size_t memoryStrings = 0;
	size_t stringsCount = 0;

	for ( auto& texture : graph.textures ) {
		memoryStrings += sizeof(char) * texture.name.length();
		++memoryStrings;
	}
	for ( auto& material : graph.materials ) {
		memoryStrings += sizeof(char) * material.name.length();
		++memoryStrings;
	}
	for ( auto& light : graph.lights ) {
		memoryStrings += sizeof(char) * light.name.length();
		++memoryStrings;
	}

	for ( auto& image : graph.images ) memoryImages += image.getPixels().size();
	for ( auto& mesh : graph.meshes ) {
		memoryMeshes += sizeof(ext::gltf::mesh_t::vertex_t) * mesh.vertices.size();
		memoryMeshes += sizeof(ext::gltf::mesh_t::indices_t) * mesh.indices.size();
	}
	for ( auto pair : graph.animations ) {
		memoryAnimations += sizeof(float) * 3;
		for ( auto& sampler : pair.second.samplers ) {
			memoryAnimations += sizeof(float) * 1 * sampler.inputs.size();
			memoryAnimations += sizeof(float) * 4 * sampler.outputs.size();
			memoryStrings += sizeof(char) * sampler.interpolator.length();
			++stringsCount;
		}
		for ( auto& channel : pair.second.channels ) {
			memoryAnimations += sizeof(uint32_t) * 2;
			memoryStrings += sizeof(char) * channel.path.length();
			++stringsCount;
		}
	}
	for ( auto& node : graph.nodes ) {
		memoryNodes += sizeof(int32_t) * 4;
		memoryNodes += sizeof(int32_t) * node.children.size();
		memoryStrings += sizeof(char) * node.name.length();
		++stringsCount;
	}

	json["strings"]["size"] = stringsCount; json["strings"]["bytes"] = memoryStrings;
	json["images"]["size"] = graph.images.size(); json["images"]["bytes"] = memoryImages;
	json["textures"]["size"] = graph.textures.size(); json["textures"]["bytes"] = memoryTextures;
	json["materials"]["size"] = graph.materials.size(); json["materials"]["bytes"] = memoryMaterials;
	json["lights"]["size"] = graph.lights.size(); json["lights"]["bytes"] = memoryLights;
	json["meshes"]["size"] = graph.meshes.size(); json["meshes"]["bytes"] = memoryMeshes;
	json["animations"]["size"] = graph.animations.size(); json["animations"]["bytes"] = memoryAnimations;
	json["nodes"]["size"] = graph.nodes.size(); json["nodes"]["bytes"] = memoryNodes;
	json["bytes"] = (memoryTextures + memoryMaterials + memoryLights + memoryImages + memoryMeshes + memoryAnimations + memoryNodes + memoryStrings);
/*
	std::cout << "Graph stats: " << 
		"\n\tNames: Bytes: " << memoryStrings << ""
		"\n\tImages: " << graph.images.size() << " | Bytes: " << memoryImages << ""
		"\n\tTextures: " << graph.textures.size() << " | Bytes: " << memoryTextures << ""
		"\n\tMaterials: " << graph.materials.size() << " | Bytes: " << memoryMaterials << ""
		"\n\tLights: " << graph.lights.size() << " | Bytes: " << memoryLights << ""
		"\n\tMeshes: " << graph.meshes.size() << " | Bytes: " << memoryMeshes << ""
		"\n\tAnimations: " << graph.animations.size() << " | Bytes: " << memoryAnimations << ""
		"\n\tNodes: " << graph.nodes.size() << " | Bytes: " << memoryNodes << ""
		"\n\tTotal: " << (memoryTextures + memoryMaterials + memoryLights + memoryImages + memoryMeshes + memoryAnimations + memoryNodes + memoryStrings) << std::endl;
*/
	return json;
}