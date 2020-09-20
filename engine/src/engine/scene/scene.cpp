#include <uf/engine/scene/scene.h>
#include <uf/ext/vulkan/vulkan.h>
#include <uf/utils/string/ext.h>
#include <uf/utils/camera/camera.h>

#include <uf/ext/vulkan/rendermodes/deferred.h>
#include <uf/ext/vulkan/rendermodes/rendertarget.h>
#include <uf/ext/vulkan/rendermodes/compute.h>
#include <uf/ext/vulkan/rendermodes/stereoscopic_deferred.h>

UF_OBJECT_REGISTER_CPP(Scene)
void uf::Scene::initialize() {
	ext::vulkan::scenes.push_back(this);
	ext::vulkan::rebuild = true;
	uf::Object::initialize();
}
void uf::Scene::tick() {
	uf::Object::tick();

	if ( uf::scene::getCurrentScene().getUid() == this->getUid() ) {
		uf::Serializer& metadata = this->getComponent<uf::Serializer>();
		/* Update lights */ if ( metadata["light"]["should"].asBool() ) {
		//	if ( !ext::vulkan::currentRenderMode || ext::vulkan::currentRenderMode->name != "" ) return;

			auto& scene = uf::scene::getCurrentScene();

			std::vector<ext::vulkan::Graphic*> blitters;
			auto& renderMode = ext::vulkan::getRenderMode("", true);
			bool hasCompute = ext::vulkan::hasRenderMode("C:RT:" + std::to_string(this->getUid()), true);
			if ( hasCompute ) {
		//		auto& renderMode = ext::vulkan::getRenderMode("C:RT:" + std::to_string(this->getUid()), true);
		//		auto* renderModePointer = (ext::vulkan::ComputeRenderMode*) &renderMode;
		//		if ( renderModePointer->compute.initialized ) {
		//			blitters.push_back(&renderModePointer->compute);
		//		} else {
		//			hasCompute = false;
		//		}
			} else if ( renderMode.getType() == "Deferred (Stereoscopic)" ) {
				auto* renderModePointer = (ext::vulkan::StereoscopicDeferredRenderMode*) &renderMode;
				blitters.push_back(&renderModePointer->blitters.left);
				blitters.push_back(&renderModePointer->blitters.right);
			} else if ( renderMode.getType() == "Deferred" ) {
				auto* renderModePointer = (ext::vulkan::DeferredRenderMode*) &renderMode;
				blitters.push_back(&renderModePointer->blitter);
			}
			auto& controller = *scene.getController();
			auto& camera = controller.getComponent<uf::Camera>();
		
		//	auto& uniforms = blitter.uniforms;
			struct UniformDescriptor {
				struct Matrices {
					alignas(16) pod::Matrix4f view[2];
					alignas(16) pod::Matrix4f projection[2];
				} matrices;
				alignas(16) pod::Vector4f ambient;
				struct Light {
					alignas(16) pod::Vector4f position;
					alignas(16) pod::Vector4f color;
					alignas(8) pod::Vector2i type;
					alignas(16) pod::Matrix4f view;
					alignas(16) pod::Matrix4f projection;
				} lights;
			};
			
			struct SpecializationConstant {
				int32_t maxLights = 16;
			} specializationConstants;

			for ( size_t _ = 0; _ < blitters.size(); ++_ ) {
				auto& blitter = *blitters[_];
				
				uint8_t* buffer;
				size_t len;
				auto* shader = &blitter.material.shaders.front();
				
				for ( auto& _ : blitter.material.shaders ) {
					if ( _.uniforms.empty() ) continue;
					auto& userdata = _.uniforms.front();
					buffer = (uint8_t*) (void*) userdata;
					len = userdata.data().len;
					shader = &_;
					specializationConstants = _.specializationConstants.get<SpecializationConstant>();
				}

				if ( !buffer ) continue;

				UniformDescriptor* uniforms = (UniformDescriptor*) buffer;
				for ( std::size_t i = 0; i < 2; ++i ) {
					uniforms->matrices.view[i] = camera.getView( i );
					uniforms->matrices.projection[i] = camera.getProjection( i );
				}
				{
					uniforms->ambient.x = metadata["light"]["ambient"][0].asFloat();
					uniforms->ambient.y = metadata["light"]["ambient"][1].asFloat();
					uniforms->ambient.z = metadata["light"]["ambient"][2].asFloat();
					uniforms->ambient.w = metadata["light"]["kexp"].asFloat();
				}
				{
					std::vector<uf::Entity*> entities;
					std::function<void(uf::Entity*)> filter = [&]( uf::Entity* entity ) {
						if ( !entity || entity->getName() != "Light" ) return;
						entities.push_back(entity);
					};
					for ( uf::Scene* scene : ext::vulkan::scenes ) { if ( !scene ) continue;
						scene->process(filter);
					}
					{
						const pod::Vector3& position = controller.getComponent<pod::Transform<>>().position;
						std::sort( entities.begin(), entities.end(), [&]( const uf::Entity* l, const uf::Entity* r ){
							if ( !l ) return false; if ( !r ) return true;
							if ( !l->hasComponent<pod::Transform<>>() ) return false; if ( !r->hasComponent<pod::Transform<>>() ) return true;
							return uf::vector::magnitude( uf::vector::subtract( l->getComponent<pod::Transform<>>().position, position ) ) < uf::vector::magnitude( uf::vector::subtract( r->getComponent<pod::Transform<>>().position, position ) );
						} );
					}

					{
						uf::Serializer& metadata = controller.getComponent<uf::Serializer>();
						if ( metadata["light"]["should"].asBool() ) entities.push_back(&controller);
					}
					UniformDescriptor::Light* lights = (UniformDescriptor::Light*) &buffer[sizeof(UniformDescriptor) - sizeof(UniformDescriptor::Light)];
					for ( size_t i = 0; i < specializationConstants.maxLights; ++i ) {
						UniformDescriptor::Light& light = lights[i];
						light.position = { 0, 0, 0, 0 };
						light.color = { 0, 0, 0, 0 };
						light.type = { 0, 0 };
					}

					blitter.material.textures.clear();

					for ( size_t i = 0; i < specializationConstants.maxLights && i < entities.size(); ++i ) {
						UniformDescriptor::Light& light = lights[i];
						uf::Entity* entity = entities[i];

						pod::Transform<>& transform = entity->getComponent<pod::Transform<>>();
						uf::Serializer& metadata = entity->getComponent<uf::Serializer>();
						uf::Camera& camera = entity->getComponent<uf::Camera>();
						
						light.position.x = transform.position.x;
						light.position.y = transform.position.y;
						light.position.z = transform.position.z;

						light.view = camera.getView();
						light.projection = camera.getProjection();

						if ( entity == &controller ) light.position.y += 2;

						light.position.w = metadata["light"]["radius"].asFloat();

						light.color.x = metadata["light"]["color"][0].asFloat();
						light.color.y = metadata["light"]["color"][1].asFloat();
						light.color.z = metadata["light"]["color"][2].asFloat();

						light.color.w = metadata["light"]["power"].asFloat();

						light.type.x = metadata["light"]["type"].asUInt64();
						light.type.y = metadata["light"]["shadows"]["enabled"].asBool();
						
						if ( !hasCompute && entity->hasComponent<ext::vulkan::RenderTargetRenderMode>() ) {
							auto& renderMode = entity->getComponent<ext::vulkan::RenderTargetRenderMode>();
							auto& renderTarget = renderMode.renderTarget;

							uint8_t i = 0;
							for ( auto& attachment : renderTarget.attachments ) {
							//	if ( !(attachment.usage & VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT) ) continue;
							//	if ( (attachment.usage & VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT) ) continue;
								if ( (attachment.layout == VK_IMAGE_LAYOUT_PRESENT_SRC_KHR) ) continue;
								auto& texture = blitter.material.textures.emplace_back();
								texture.aliasAttachment(attachment);
								light.type.y = true;
							//	break;
							}
						} else {
							light.type.y = false;
						}
					}
				}
				blitter.getPipeline().update( blitter );
				shader->updateBuffer( (void*) buffer, len, 0, false );
			}
		}
	}
}
void uf::Scene::render() {
	uf::Object::render();

}
void uf::Scene::destroy() {
	uf::Object::destroy();

	{
		auto it = std::find(ext::vulkan::scenes.begin(), ext::vulkan::scenes.end(), this);
		if ( it != ext::vulkan::scenes.end() ) {
			ext::vulkan::scenes.erase(it);
		}
	}
	ext::vulkan::rebuild = true;
}



/*
uf::Entity* uf::Scene::getController() {
	return this->findByName("Player");
}
const uf::Entity* uf::Scene::getController() const {
	return this->findByName("Player");
}
*/
uf::Entity* uf::Scene::getController() {
	static uf::Entity* cachedController = NULL;
	if ( ext::vulkan::currentRenderMode ) {
		static ext::vulkan::RenderMode* cachedRenderMode = NULL;
		auto& renderMode = *ext::vulkan::currentRenderMode;

		if ( cachedRenderMode == &renderMode && cachedController && cachedController->getUid() > 0 ) {
			return cachedController;
		}
		cachedController = NULL;
		cachedRenderMode = &renderMode;
		auto split = uf::string::split( renderMode.name, ":" );
		if ( split.front() == "RT" ) {
			uint64_t uid = std::stoi( split.back() );
			uf::Entity* ent = this->findByUid( uid );
			if ( ent ) return cachedController = ent;
		}
	}
	if ( cachedController && cachedController->getUid() > 0 ) return cachedController;
	return cachedController = this->findByName("Player");
	// return this;
}
const uf::Entity* uf::Scene::getController() const {
	static const uf::Entity* cachedController = NULL;
	if ( ext::vulkan::currentRenderMode ) {
		static ext::vulkan::RenderMode* cachedRenderMode = NULL;
		auto& renderMode = *ext::vulkan::currentRenderMode;

		if ( cachedRenderMode == &renderMode && cachedController && cachedController->getUid() > 0 ) {
			return cachedController;
		}
		cachedController = NULL;
		cachedRenderMode = &renderMode;
		auto split = uf::string::split( renderMode.name, ":" );
		if ( split.front() == "RT" ) {
			uint64_t uid = std::stoi( split.back() );
			const uf::Entity* ent = this->findByUid( uid );
			if ( ent ) return cachedController = ent;
		}
	}
	if ( cachedController && cachedController->getUid() > 0 ) return cachedController;
	return cachedController = this->findByName("Player");
	// return this;
}

std::vector<uf::Scene*> uf::scene::scenes;
uf::Scene& uf::scene::loadScene( const std::string& name, const std::string& filename ) {
	uf::Scene* scene = (uf::Scene*) uf::instantiator::instantiate( name );
	uf::scene::scenes.push_back(scene);
	scene->load(filename != "" ? filename : "./scenes/"+uf::string::lowercase(name)+"/scene.json");
	scene->initialize();
	return *scene;
}
uf::Scene& uf::scene::loadScene( const std::string& name, const uf::Serializer& data ) {
	uf::Scene* scene = (uf::Scene*) uf::instantiator::instantiate( name );
	uf::scene::scenes.push_back(scene);
	if ( data != "" ) scene->load(data);
	scene->initialize();
	return *scene;
}
void uf::scene::unloadScene() {
	uf::Scene* current = uf::scene::scenes.back();
	current->destroy();
	uf::scene::scenes.pop_back();
	delete current;
}
uf::Scene& uf::scene::getCurrentScene() {
	return *uf::scene::scenes.back();
}
void uf::scene::tick() {
	// uf::scene::getCurrentScene().tick();
	for ( auto scene : scenes ) scene->tick();
}
void uf::scene::render() {
	// uf::scene::getCurrentScene().render();
	for ( auto scene : scenes ) scene->render();
}
void uf::scene::destroy() {
	while ( !scenes.empty() ) {
		unloadScene();
	}
}
/*
uf::Camera& uf::Scene::getCamera() {
	if ( !::camera ) ::camera = this->getPlayer().getComponentPointer<uf::Camera>();
	return *::camera;
}
uf::Player& uf::Scene::getPlayer() {
	return *((uf::Player*) this->findByName("Player"));
}
const uf::Player& uf::Scene::getPlayer() const {
	return *((const uf::Player*) this->findByName("Player"));
}
*/