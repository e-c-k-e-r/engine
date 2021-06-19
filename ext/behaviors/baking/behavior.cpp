#include "behavior.h"

#include <uf/utils/renderer/renderer.h>

#include <uf/utils/math/transform.h>
#include <uf/utils/math/physics.h>
#include <uf/utils/camera/camera.h>
#include <uf/ext/gltf/gltf.h>
#include <uf/engine/asset/asset.h>

#include <uf/ext/xatlas/xatlas.h>

#include "../light/behavior.h"

UF_BEHAVIOR_REGISTER_CPP(ext::BakingBehavior)
#define this (&self)
void ext::BakingBehavior::initialize( uf::Object& self ) {
#if UF_USE_VULKAN
	this->addHook( "entity:PostInitialization.%UID%", [&](){
		auto& metadataJson = this->getComponent<uf::Serializer>();
		auto& metadata = this->getComponent<ext::BakingBehavior::Metadata>();
		metadata.names.model = this->grabURI( metadataJson["baking"]["model"].as<std::string>(), metadataJson["system"]["root"].as<std::string>() );
		if ( metadataJson["baking"]["output"]["model"].is<std::string>() ) metadata.names.output.model = this->grabURI( metadataJson["baking"]["output"]["model"].as<std::string>(), metadataJson["system"]["root"].as<std::string>() );
		metadata.names.output.map = this->grabURI( metadataJson["baking"]["output"]["map"].as<std::string>(), metadataJson["system"]["root"].as<std::string>() );
		metadata.names.renderMode = "B:" + std::to_string((int) this->getUid());
		metadata.trigger.mode = metadataJson["baking"]["trigger"]["mode"].as<std::string>();
		metadata.trigger.value = metadataJson["baking"]["trigger"]["value"].as<std::string>();
		if ( metadataJson["baking"]["resolution"].is<size_t>() ) metadata.size = { metadataJson["baking"]["resolution"].as<size_t>(), metadataJson["baking"]["resolution"].as<size_t>() };
		metadata.shadows = metadataJson["baking"]["shadows"].as<bool>();

		UF_DEBUG_MSG("Unwrapping...");
		auto& graph = this->getComponent<pod::Graph>();
		graph = ext::gltf::load( metadata.names.model, uf::graph::LoadMode::ATLAS );

		auto size = ext::xatlas::unwrap( graph );
		if ( size.x == 0 && size.y == 0 ) return;
		if ( metadata.size.x == 0 && metadata.size.y == 0 ) metadata.size = size;
		if ( metadata.names.output.model != "" ) {
			UF_DEBUG_MSG("Saving to " << metadata.names.output.model);
			uf::graph::save( graph, metadata.names.output.model );
		}

		auto& graphic = this->getComponent<uf::Graphic>();
		graphic.process = false;
		graphic.descriptor.cullMode = uf::renderer::enums::CullMode::NONE;

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
		} else {
			for ( size_t i = 0; i < graph.textures.size() && i < graph.images.size(); ++i ) {
				auto& image = graph.images[i];
				auto& texture = graph.textures[i];
				texture.bind = true;
				texture.storage.index = i;
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
		
		auto& mesh = this->getComponent<uf::graph::mesh_t>();
		for ( auto& m : graph.meshes ) mesh.insert(m);

		auto& renderMode = this->getComponent<uf::renderer::RenderTargetRenderMode>();
		uf::renderer::addRenderMode( &renderMode, metadata.names.renderMode );
		renderMode.metadata["type"] = "single";
		renderMode.metadata["samples"] = 1;

		renderMode.width = metadata.size.x;
		renderMode.height = metadata.size.y;
		renderMode.blitter.process = false;

		graphic.device = &ext::vulkan::device;
		graphic.material.device = graphic.device;
		std::vector<pod::Matrix4f> instances;
		instances.reserve( graph.nodes.size() );
		for ( auto& node : graph.nodes ) {
			instances.emplace_back( uf::transform::model( node.transform ) );
		}
		// Models storage buffer
		graphic.initializeBuffer(
			(void*) instances.data(),
			instances.size() * sizeof(pod::Matrix4f),
			uf::renderer::enums::Buffer::STORAGE
		);
		// Materials storage buffer
		std::vector<pod::Material::Storage> materials( graph.materials.size() );
		for ( size_t i = 0; i < graph.materials.size(); ++i ) {
			materials[i] = graph.materials[i].storage;
		}
		graphic.initializeBuffer(
			(void*) materials.data(),
			materials.size() * sizeof(pod::Material::Storage),
			uf::renderer::enums::Buffer::STORAGE
		);
		// Texture storage buffer
		std::vector<pod::Texture::Storage> textures( graph.textures.size() );
		for ( size_t i = 0; i < graph.textures.size(); ++i ) {
			textures[i] = graph.textures[i].storage;
		}
		graphic.initializeBuffer(
			(void*) textures.data(),
			textures.size() * sizeof(pod::Texture::Storage),
			uf::renderer::enums::Buffer::STORAGE
		);

		UF_DEBUG_MSG("Unwrapped model");
	});

	this->queueHook( "entity:PostInitialization.%UID%", ext::json::null(), 2.0f );
#endif
}
void ext::BakingBehavior::tick( uf::Object& self ) {
#if 0
#if UF_USE_VULKAN
	if ( !this->hasComponent<uf::renderer::RenderTargetRenderMode>() ) return;
	auto& metadata = this->getComponent<ext::BakingBehavior::Metadata>();
	auto& renderMode = this->getComponent<uf::renderer::RenderTargetRenderMode>();
	if ( renderMode.executed && !metadata.initialized.renderMode ) {
		UF_DEBUG_MSG("Preparing graphics to bake...");
		auto& graph = this->getComponent<pod::Graph>();
		auto& mesh = this->getComponent<uf::graph::mesh_t>();
		auto& renderMode = this->getComponent<uf::renderer::RenderTargetRenderMode>();
		auto& graphic = this->getComponent<uf::Graphic>();
		auto& metadata = this->getComponent<ext::BakingBehavior::Metadata>();

		graphic.initialize( metadata.names.renderMode );
		graphic.initializeMesh( mesh );
		// Light storage buffer
		std::vector<pod::Light::Storage> lights;
		lights.reserve(1024);
		if ( metadata.shadows ) {
			auto& scene = uf::scene::getCurrentScene();
			auto& graph = scene.getGraph();
			auto& controller = scene.getController();

			struct LightInfo {
				uf::Entity* entity = NULL;
				pod::Vector3f color = {0,0,0,0};
				pod::Vector3f position = {0,0,0};
				float power = 0;
				float bias = 0;
				int32_t type = 0;
				bool shadows = false;
			};
			std::vector<LightInfo> infos;
			for ( auto entity : graph ) {
				if ( entity == &controller || entity == this ) continue;
				if ( entity->getName() != "Light" && !entity->hasComponent<ext::LightBehavior::Metadata>() ) continue;
				auto& metadata = entity->getComponent<ext::LightBehavior::Metadata>();
				if ( metadata.power <= 0 ) continue;
				auto& transform = entity->getComponent<pod::Transform<>>();
				auto flatten = uf::transform::flatten( transform );
				LightInfo& info = infos.emplace_back(LightInfo{
					.entity = entity,
					.color = metadata.color,
					.position = flatten.position,
					.power = metadata.power,
					.bias = metadata.bias,
					.type = metadata.type,
					.shadows = metadata.shadows,
				});
			}
			size_t textureSlot = graphic.material.textures.size();
			for ( auto& info : infos ) {
				uf::Entity* entity = info.entity;

				auto& transform = entity->getComponent<pod::Transform<>>();
				auto& metadata = entity->getComponent<ext::LightBehavior::Metadata>();
				auto& camera = entity->getComponent<uf::Camera>();

				pod::Light::Storage light;
				light.position = info.position;

				light.color = info.color;
				light.type = info.type;
				light.indexMap = -1;

				light.depthBias = info.bias;
				if ( info.shadows && entity->hasComponent<uf::renderer::RenderTargetRenderMode>() ) {
					auto& renderMode = entity->getComponent<uf::renderer::RenderTargetRenderMode>();
					size_t view = 0;
					for ( auto& attachment : renderMode.renderTarget.attachments ) {
						if ( !(attachment.descriptor.usage & VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT) ) continue;
						if ( attachment.descriptor.layout == VK_IMAGE_LAYOUT_PRESENT_SRC_KHR ) continue;

						graphic.material.textures.emplace_back().aliasAttachment(attachment);

						light.indexMap = textureSlot++;
						light.view = camera.getView(view);
						light.projection = camera.getProjection(view);
						lights.emplace_back(light);

						++view;
					}
					light.indexMap = -1;
				} else {
					lights.emplace_back(light);
				}
			}
		} else {
			for ( auto& l : graph.lights ) {
				for ( auto& node : graph.nodes ) {
					if ( l.name != node.name ) continue;
					auto transform = uf::transform::flatten( node.transform );
					auto& light = lights.emplace_back();
					light.position = transform.position;
					light.color = l.color;
					light.color.w = l.intensity;
					light.position.w = l.range;

					break;
				}
			}
		}
		graphic.initializeBuffer(
			(void*) lights.data(),
			lights.size() * sizeof(pod::Light::Storage),
			uf::renderer::enums::Buffer::STORAGE
		);

		graphic.material.attachShader(uf::io::root+"/shaders/gltf/baking/bake.vert.spv", uf::renderer::enums::Shader::VERTEX);
		graphic.material.attachShader(uf::io::root+"/shaders/gltf/baking/bake.frag.spv", uf::renderer::enums::Shader::FRAGMENT);
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

		graphic.process = true;
		metadata.initialized.renderMode = true;
		UF_DEBUG_MSG("Graphic configured, ready to bake");
	} else if ( renderMode.executed && !metadata.initialized.map ) {
		TIMER(1.0, (metadata.trigger.mode == "rendered" || (metadata.trigger.mode == "key" && uf::Window::isKeyPressed(metadata.trigger.value))) && ) {
			goto SAVE;
		}
	}
	return;
SAVE:
	renderMode.execute = false;
	UF_DEBUG_MSG("Baking...");
	auto image = renderMode.screenshot();
	std::string filename = metadata.names.output.map;
	bool status = image.save(filename);
	UF_DEBUG_MSG("Writing to " << filename << ": " << status);
	UF_DEBUG_MSG("Baked.");
	metadata.initialized.map = true;

	uf::Serializer payload;
	payload["uid"] = this->getUid();
	uf::scene::getCurrentScene().queueHook("system:Destroy", payload);
#endif
#endif
}
void ext::BakingBehavior::render( uf::Object& self ){}
void ext::BakingBehavior::destroy( uf::Object& self ){
	if ( this->hasComponent<uf::renderer::RenderTargetRenderMode>() ) {
		auto& renderMode = this->getComponent<uf::renderer::RenderTargetRenderMode>();
		uf::renderer::removeRenderMode( &renderMode, false );
		this->deleteComponent<uf::renderer::RenderTargetRenderMode>();
	}
	if ( this->hasComponent<pod::Graph>() ) {
		auto& graph = this->getComponent<pod::Graph>();
		uf::graph::destroy( graph );
	}
}
#undef this