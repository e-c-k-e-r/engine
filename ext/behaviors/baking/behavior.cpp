#include "behavior.h"

#include <uf/utils/renderer/renderer.h>

#include <uf/utils/math/transform.h>
#include <uf/utils/math/physics.h>
#include <uf/utils/camera/camera.h>
#include <uf/ext/gltf/gltf.h>
#include <uf/engine/asset/asset.h>
#include <uf/utils/graphic/graphic.h>
#include <uf/ext/xatlas/xatlas.h>

#include "../light/behavior.h"
#include "../scene/behavior.h"

UF_BEHAVIOR_REGISTER_CPP(ext::BakingBehavior)
UF_BEHAVIOR_TRAITS_CPP(ext::BakingBehavior, ticks = true, renders = false, multithread = false)
#define this (&self)
void ext::BakingBehavior::initialize( uf::Object& self ) {
#if 0
#if UF_USE_VULKAN
	UF_MSG_DEBUG("Ping");

	this->addHook( "entity:PostInitialization.%UID%", [&]( ext::json::Value& ){
		auto& metadataJson = this->getComponent<uf::Serializer>();
		auto& metadata = this->getComponent<ext::BakingBehavior::Metadata>();
		auto& scene = uf::scene::getCurrentScene();
		auto& controller = scene.getController();
		auto& controllerTransform = controller.getComponent<pod::Transform<>>();

		metadata.names.model = this->grabURI( metadataJson["baking"]["model"].as<uf::stl::string>(), metadataJson["system"]["root"].as<uf::stl::string>() );
		if ( metadataJson["baking"]["output"]["model"].is<uf::stl::string>() )
			metadata.names.output.model = this->grabURI( metadataJson["baking"]["output"]["model"].as<uf::stl::string>(), metadataJson["system"]["root"].as<uf::stl::string>() );
		metadata.names.output.map = this->grabURI( metadataJson["baking"]["output"]["map"].as<uf::stl::string>(), metadataJson["system"]["root"].as<uf::stl::string>() );
		metadata.names.renderMode = "B:" + std::to_string((int) this->getUid());
		metadata.trigger.mode = metadataJson["baking"]["trigger"]["mode"].as<uf::stl::string>();
		metadata.trigger.value = metadataJson["baking"]["trigger"]["value"].as<uf::stl::string>();
		if ( metadataJson["baking"]["resolution"].is<size_t>() )
			metadata.size = { metadataJson["baking"]["resolution"].as<size_t>(), metadataJson["baking"]["resolution"].as<size_t>() };
		metadata.max.lights = metadataJson["baking"]["lights"].as<size_t>(metadata.max.lights);
		metadata.max.shadows = metadataJson["baking"]["shadows"].as<size_t>(metadata.max.shadows);
		metadata.cull = metadataJson["baking"]["cull"].as<bool>();

		{
			auto& scene = uf::scene::getCurrentScene();
			auto& sceneMetadata = scene.getComponent<ext::ExtSceneBehavior::Metadata>();

			metadata.previous.max = sceneMetadata.shadow.max;
			metadata.previous.update = sceneMetadata.shadow.update;

			sceneMetadata.shadow.max = 1024;
			sceneMetadata.shadow.update = 1024;

			UF_MSG_DEBUG("Temporarily altering shadow limits...");
		}

		auto& renderMode = this->getComponent<uf::renderer::RenderTargetRenderMode>();
		uf::renderer::addRenderMode( &renderMode, metadata.names.renderMode );

		renderMode.execute = false;
		renderMode.metadata.type = "single";
		renderMode.metadata.samples = 1;

		renderMode.width = metadata.size.x;
		renderMode.height = metadata.size.y;
		renderMode.blitter.process = false;

		UF_MSG_DEBUG("Loading " << metadata.names.model );

		auto& graph = this->getComponent<pod::Graph>();
		graph = uf::graph::load( metadata.names.model, uf::graph::LoadMode::ATLAS );
		graph.metadata["export"] = metadataJson["baking"]["export"];

		if ( ext::json::isNull(graph.metadata["wrapped"]) ) {
			UF_MSG_DEBUG("Unwrapping...");
			auto size = ext::xatlas::unwrap( graph );
			if ( size.x == 0 && size.y == 0 ) return;
			if ( metadata.size.x == 0 && metadata.size.y == 0 ) metadata.size = size;
			if ( metadata.names.output.model != "" ) {
				UF_MSG_DEBUG("Saving to " << metadata.names.output.model);
				uf::graph::save( graph, metadata.names.output.model );
			}
			UF_MSG_DEBUG("Unwrapped model");
		}

		auto& graphic = this->getComponent<uf::Graphic>();

		UF_MSG_DEBUG("Binding textures...");
		uf::stl::vector<uf::renderer::Texture> textures2D;
		textures2D.reserve( metadata.max.textures2D );

		uf::stl::vector<uf::renderer::Texture> textures3D;
		textures3D.reserve( metadata.max.textures3D );
		
		uf::stl::vector<uf::renderer::Texture> texturesCube;
		texturesCube.reserve( metadata.max.texturesCube );
		if ( graph.atlas.generated() ) {
			for ( auto& texture : graph.textures ) {
				++texture.storage.index;
			}
			auto& image = *graph.images.emplace(graph.images.begin(), graph.atlas.getAtlas());
			auto& texture = *graph.textures.emplace(graph.textures.begin());
			texture.name = "atlas";
			texture.bind = true;
			texture.storage.index = 0;
			texture.storage.sampler = 0;
			texture.storage.remap = 0;
			texture.storage.blend = 0;

			if ( graph.metadata["filter"].is<uf::stl::string>() ) {
				uf::stl::string filter = graph.metadata["filter"].as<uf::stl::string>();
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
		} else {
			for ( size_t i = 0; i < graph.textures.size() && i < graph.images.size(); ++i ) {
				auto& image = graph.images[i];
				auto& texture = graph.textures[i];
				texture.bind = true;
				texture.storage.index = i;
				if ( graph.metadata["filter"].is<uf::stl::string>() ) {
					uf::stl::string filter = graph.metadata["filter"].as<uf::stl::string>();
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
		{
			size_t textureSlot = 0;
			for ( auto& texture : graph.textures ) {
				texture.storage.index = -1;
				if ( !texture.bind ) continue;
				texture.storage.index = textureSlot++;
			}
		}
		for ( auto& texture : graph.textures ) {
			if ( !texture.bind ) continue;
			graphic.material.textures.emplace_back().aliasTexture(texture.texture);
		}
		
		UF_MSG_DEBUG("Binding meshes...");
		auto& mesh = this->getComponent<pod::Graph::Mesh>();
	#if UF_GRAPH_EXPERIMENTAL
		for ( auto& nodeMesh : graph.meshes ) {
			if ( nodeMesh.is<uf::graph::mesh::Base>() ) mesh.insert<uf::graph::mesh::Base>( nodeMesh );
			else if ( nodeMesh.is<uf::graph::mesh::ID>() ) mesh.insert<uf::graph::mesh::ID>( nodeMesh );
			else if ( nodeMesh.is<uf::graph::mesh::Skinned>() ) mesh.insert<uf::graph::mesh::Skinned>( nodeMesh );
		}
	#else
		for ( auto& m : graph.meshes ) mesh.insert(m);
	#endif
		mesh.updateDescriptor();

		UF_MSG_DEBUG("initializing graphic attributes...");
		graphic.process = false;
		graphic.descriptor.cullMode = metadata.cull ? uf::renderer::enums::CullMode::BACK : uf::renderer::enums::CullMode::NONE;
		graphic.device = &ext::vulkan::device;
		graphic.material.device = graphic.device;
	#if UF_GRAPH_EXPERIMENTAL
		graphic.initializeAttributes( mesh.attributes );
	#else
		graphic.initializeMesh( mesh );
	#endif

		// attach lighting info
		UF_MSG_DEBUG("Calculating lights...");
		{
			auto& sceneGraph = scene.getGraph();

			struct LightInfo {
				uf::Entity* entity = NULL;
				pod::Vector4f color = {0,0,0,0};
				pod::Vector3f position = {0,0,0};
				float distance = 0;
				float bias = 0;
				int32_t type = 0;
				bool shadows = false;
			};
			uf::stl::vector<LightInfo> entities;
			entities.reserve(sceneGraph.size());

			uf::stl::vector<pod::Light::Storage> lights;
			lights.reserve(metadata.max.lights);

			int32_t shadowUpdateThreshold = metadata.max.shadows; // how many shadow maps we should update, based on range
			int32_t shadowCount = metadata.max.shadows; // how many shadow maps we should pass, based on range
			if ( shadowCount <= 0 ) shadowCount = std::numeric_limits<int32_t>::max();
			int32_t experimentalMode = 1;

			for ( auto entity : sceneGraph ) {
				// ignore this scene, our controller, and anything that isn't actually a light
				if ( entity == this || entity == &controller || !entity->hasComponent<ext::LightBehavior::Metadata>() ) continue;
				auto& metadata = entity->getComponent<ext::LightBehavior::Metadata>();
				// disables shadow mappers that activate when in range
				bool hasRT = entity->hasComponent<uf::renderer::RenderTargetRenderMode>();
				if ( hasRT ) {
					auto& renderMode = entity->getComponent<uf::renderer::RenderTargetRenderMode>();
					if ( metadata.renderer.mode == "in-range" ) renderMode.execute = false;
				}
				if ( metadata.power <= 0 ) continue;
				auto flatten = uf::transform::flatten( entity->getComponent<pod::Transform<>>() );
				LightInfo& info = entities.emplace_back(LightInfo{
					.entity = entity,
					.color = pod::Vector4f{ metadata.color.x, metadata.color.y, metadata.color.z, metadata.power },
					.position = flatten.position,
					.distance = uf::vector::magnitude( uf::vector::subtract( flatten.position, controllerTransform.position ) ),
					.bias = metadata.bias,
					.type = metadata.type,
					.shadows = metadata.shadows && hasRT,
				});
			}
			std::sort( entities.begin(), entities.end(), [&]( LightInfo& l, LightInfo& r ){
				return l.distance < r.distance;
			});
			// bind lighting and requested shadow maps
			for ( uint32_t i = 0; i < entities.size() && lights.size() < metadata.max.lights; ++i ) {
				auto& info = entities[i];
				uf::Entity* entity = info.entity;

				if ( !info.shadows ) {
					lights.emplace_back(pod::Light::Storage{
						.position = info.position,
						.color = info.color,
						.type = info.type,
						.typeMap = 0,
						.indexMap = -1,
						.depthBias = info.bias,
						.view = uf::matrix::identity(),
						.projection = uf::matrix::identity(),
					});
				} else {
					auto& renderMode = entity->getComponent<uf::renderer::RenderTargetRenderMode>();
					auto& lightCamera = entity->getComponent<uf::Camera>();
					auto& lightMetadata = entity->getComponent<ext::LightBehavior::Metadata>();
					lightMetadata.renderer.rendered = true;
					// activate our shadow mapper if it's range-basedd
					if ( lightMetadata.renderer.mode == "in-range" && shadowUpdateThreshold-- > 0 ) renderMode.execute = true;
					// if point light, and combining is requested
					if ( experimentalMode > 0 && renderMode.renderTarget.views == 6 ) {
						int32_t index = -1;
						// separated texture2Ds
						if ( experimentalMode == 2 ) {
							index = textures2D.size();
							for ( auto& attachment : renderMode.renderTarget.attachments ) {
								if ( !(attachment.descriptor.usage & VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT) ) continue;
								for ( size_t view = 0; view < renderMode.renderTarget.views; ++view ) {			
									textures2D.emplace_back().aliasAttachment(attachment, view);
								}
								break;
							}
						// cubemapped
						} else {
							index = texturesCube.size();
							for ( auto& attachment : renderMode.renderTarget.attachments ) {
								if ( !(attachment.descriptor.usage & VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT) ) continue;
								texturesCube.emplace_back().aliasAttachment(attachment);
								break;
							}
						}
						lights.emplace_back(pod::Light::Storage{
							.position = info.position,
							.color = info.color,
							.type = info.type,
							.typeMap = experimentalMode,
							.indexMap = index,
							.depthBias = info.bias,
							.view = lightCamera.getView(0),
							.projection = lightCamera.getProjection(0),
						});
					// any other shadowing light, even point lights, are split by shadow maps
					} else {
						for ( auto& attachment : renderMode.renderTarget.attachments ) {
							if ( !(attachment.descriptor.usage & VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT) ) continue;
							if ( attachment.descriptor.layout == VK_IMAGE_LAYOUT_PRESENT_SRC_KHR ) continue;
							for ( size_t view = 0; view < renderMode.renderTarget.views; ++view ) {
								lights.emplace_back(pod::Light::Storage{
									.position = info.position,
									.color = info.color,
									.type = info.type,
									.typeMap = 0,
									.indexMap = textures2D.size(),
									.depthBias = info.bias,
									.view = lightCamera.getView(view),
									.projection = lightCamera.getProjection(view),
								});
								textures2D.emplace_back().aliasAttachment(attachment, view);
							}
						}
					}
				}
			}
			UF_MSG_DEBUG("Attaching storage buffers...");
			// Models storage buffer
			uf::stl::vector<pod::Matrix4f> instances;
			instances.reserve( graph.nodes.size() );
			for ( auto& node : graph.nodes ) {
				instances.emplace_back( uf::transform::model( node.transform ) );
			}
			metadata.buffers.instance = shader.initializeBuffer(
				(const void*) instances.data(),
				instances.size() * sizeof(pod::Matrix4f),
				uf::renderer::enums::Buffer::STORAGE
			);
			// Materials storage buffer
			uf::stl::vector<pod::Material::Storage> materials( graph.materials.size() );
			for ( size_t i = 0; i < graph.materials.size(); ++i ) {
				materials[i] = graph.materials[i].storage;
			}
			metadata.buffers.material = shader.initializeBuffer(
				(const void*) materials.data(),
				materials.size() * sizeof(pod::Material::Storage),
				uf::renderer::enums::Buffer::STORAGE
			);
			// Texture storage buffer
			uf::stl::vector<pod::Texture::Storage> textures( graph.textures.size() );
			for ( size_t i = 0; i < graph.textures.size(); ++i ) {
				textures[i] = graph.textures[i].storage;
			}
			metadata.buffers.texture = shader.initializeBuffer(
				(const void*) textures.data(),
				textures.size() * sizeof(pod::Texture::Storage),
				uf::renderer::enums::Buffer::STORAGE
			);
			// Light storage buffer
			metadata.buffers.light = shader.initializeBuffer(
				(const void*) lights.data(),
				lights.size() * sizeof(pod::Light::Storage),
				uf::renderer::enums::Buffer::STORAGE
			);
		}
		// attach shader
		UF_MSG_DEBUG("Attaching shaders...");
		{

			size_t texture2Ds = textures2D.size();
			size_t texture3Ds = textures3D.size();
			size_t textureCubes = texturesCube.size();

			for ( auto& texture : graphic.material.textures ) {
				if ( texture.width > 1 && texture.height > 1 && texture.depth == 1 && texture.layers == 1 ) ++texture2Ds;
				else if ( texture.width > 1 && texture.height > 1 && texture.depth > 1 && texture.layers == 1 ) ++texture3Ds;
			}
			
			// attach textures
			for ( auto& t : textures2D ) graphic.material.textures.emplace_back().aliasTexture(t);
			for ( auto& t : texturesCube ) graphic.material.textures.emplace_back().aliasTexture(t);
			for ( auto& t : textures3D ) graphic.material.textures.emplace_back().aliasTexture(t);

			// standard pipeline
			{
				uf::stl::string vertexShaderFilename = "/graph/baking/bake.vert.spv";
				uf::stl::string geometryShaderFilename = "";
				uf::stl::string fragmentShaderFilename = "/graph/baking/bake.frag.spv";
				if ( uf::renderer::settings::experimental::deferredSampling ) {
					fragmentShaderFilename = uf::string::replace( fragmentShaderFilename, "frag", "deferredSampling.frag" );
				}
				{
					vertexShaderFilename = uf::io::resolveURI( vertexShaderFilename );
					graphic.material.attachShader(vertexShaderFilename, uf::renderer::enums::Shader::VERTEX);
				}
				{
					fragmentShaderFilename = uf::io::resolveURI( fragmentShaderFilename );
					graphic.material.attachShader(fragmentShaderFilename, uf::renderer::enums::Shader::FRAGMENT);
				}
				if ( geometryShaderFilename != "" && uf::renderer::device.enabledFeatures.geometryShader ) {
					geometryShaderFilename = uf::io::resolveURI( geometryShaderFilename );
					graphic.material.attachShader(geometryShaderFilename, uf::renderer::enums::Shader::GEOMETRY);
				}
				{
					uint32_t maxPasses = 6;

					auto& shader = graphic.material.getShader("vertex");
					uint32_t* specializationConstants = (uint32_t*) (void*) &shader.specializationConstants;
					ext::json::forEach( shader.metadata.json["specializationConstants"], [&]( size_t i, ext::json::Value& sc ){
						uf::stl::string name = sc["name"].as<uf::stl::string>();
						if ( name == "PASSES" ) sc["value"] = (specializationConstants[i] = maxPasses);
					});
				}
				{
					uint32_t maxTextures2D = texture2Ds;
					uint32_t maxTexturesCube = textureCubes;
					uint32_t maxTextures3D = texture3Ds;
					uint32_t maxCascades = 1;

					auto& shader = graphic.material.getShader("fragment");
					uint32_t* specializationConstants = (uint32_t*) (void*) &shader.specializationConstants;
					ext::json::forEach( shader.metadata.json["specializationConstants"], [&]( size_t i, ext::json::Value& sc ){
						uf::stl::string name = sc["name"].as<uf::stl::string>();
						if ( name == "TEXTURES" ) sc["value"] = (specializationConstants[i] = maxTextures2D);
						else if ( name == "CUBEMAPS" ) sc["value"] = (specializationConstants[i] = maxTexturesCube);
						else if ( name == "CASCADES" ) sc["value"] = (specializationConstants[i] = maxCascades);
					});
					ext::json::forEach( shader.metadata.json["definitions"]["textures"], [&]( ext::json::Value& t ){
						size_t binding = t["binding"].as<size_t>();
						uf::stl::string name = t["name"].as<uf::stl::string>();
						for ( auto& layout : shader.descriptorSetLayoutBindings ) {
							if ( layout.binding != binding ) continue;
							if ( name == "samplerTextures" ) layout.descriptorCount = maxTextures2D;
							else if ( name == "samplerCubemaps" ) layout.descriptorCount = maxTexturesCube;
							else if ( name == "voxelId" ) layout.descriptorCount = maxCascades;
							else if ( name == "voxelUv" ) layout.descriptorCount = maxCascades;
							else if ( name == "voxelNormal" ) layout.descriptorCount = maxCascades;
							else if ( name == "voxelRadiance" ) layout.descriptorCount = maxCascades;
						}
					});
				}
			}
		}
		UF_MSG_DEBUG("Finished initialiation.");
	});

	this->queueHook( "entity:PostInitialization.%UID%", ext::json::null(), 1 );
#endif
#endif
}
void ext::BakingBehavior::tick( uf::Object& self ) {
#if 0
#if UF_USE_VULKAN
	if ( !this->hasComponent<uf::renderer::RenderTargetRenderMode>() ) return;
	auto& metadata = this->getComponent<ext::BakingBehavior::Metadata>();
	auto& renderMode = this->getComponent<uf::renderer::RenderTargetRenderMode>();
	if ( renderMode.executed && !metadata.initialized.renderMode ) goto PREPARE;
	else if ( renderMode.executed && !metadata.initialized.map ) {
		TIMER(1.0, (metadata.trigger.mode == "rendered" || (metadata.trigger.mode == "key" && uf::Window::isKeyPressed(metadata.trigger.value))) && ) {
			goto SAVE;
		}
	}
	return;
PREPARE: {
	UF_MSG_DEBUG("Preparing graphics to bake...");
	auto& graphic = this->getComponent<uf::Graphic>();

	graphic.initialize( metadata.names.renderMode );
	graphic.process = true;
	metadata.initialized.renderMode = true;
	renderMode.execute = true;
	uf::renderer::states::rebuild = true;

	UF_MSG_DEBUG("Graphic configured, ready to bake");
	return;
}
SAVE: {
	renderMode.execute = false;
	UF_MSG_DEBUG("Baking...");
	auto image = renderMode.screenshot();
	uf::stl::string filename = metadata.names.output.map;
	bool status = image.save(filename);
	UF_MSG_DEBUG("Writing to " << filename << ": " << status);
	UF_MSG_DEBUG("Baked.");
	metadata.initialized.map = true;

	{
		auto& scene = uf::scene::getCurrentScene();
		auto& sceneMetadata = scene.getComponent<ext::ExtSceneBehavior::Metadata>();

		sceneMetadata.shadow.max = metadata.previous.max;
		sceneMetadata.shadow.update = metadata.previous.update;

		UF_MSG_DEBUG("Reverted shadow limits");
	}

	uf::Serializer payload;
	payload["uid"] = this->getUid();
	uf::scene::getCurrentScene().queueHook("system:Destroy", payload);
	return;
}
#endif
#endif
}
void ext::BakingBehavior::render( uf::Object& self ){}
void ext::BakingBehavior::destroy( uf::Object& self ){
	if ( this->hasComponent<uf::renderer::RenderTargetRenderMode>() ) {
		auto& renderMode = this->getComponent<uf::renderer::RenderTargetRenderMode>();
		uf::renderer::removeRenderMode( &renderMode, false );
	//	this->deleteComponent<uf::renderer::RenderTargetRenderMode>();
	}
	if ( this->hasComponent<pod::Graph>() ) {
		auto& graph = this->getComponent<pod::Graph>();
		uf::graph::destroy( graph );
	}
}
void ext::BakingBehavior::Metadata::serialize( uf::Object& self, uf::Serializer& serializer ) {}
void ext::BakingBehavior::Metadata::deserialize( uf::Object& self, uf::Serializer& serializer ) {}
#undef this