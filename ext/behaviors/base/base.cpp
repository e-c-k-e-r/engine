#include "base.h"

#include <uf/utils/time/time.h>
#include <uf/utils/io/iostream.h>
#include <uf/utils/math/vector.h>
#include <uf/utils/math/transform.h>
#include <uf/utils/window/window.h>
#include <uf/utils/graphic/graphic.h>

#include <uf/utils/audio/audio.h>
#include <uf/utils/thread/thread.h>
#include <uf/utils/camera/camera.h>

#include <uf/engine/asset/asset.h>
#include <uf/engine/asset/masterdata.h>

#include <uf/utils/renderer/renderer.h>

#include <uf/ext/gltf/gltf.h>

#include <uf/utils/math/collision.h>

#include "../../ext.h"
#include "../../gui/gui.h"

EXT_BEHAVIOR_REGISTER_CPP(SceneBehavior)
#define this ((uf::Scene*) &self)
void ext::SceneBehavior::initialize( uf::Object& self ) {
	uf::Asset& assetLoader = this->getComponent<uf::Asset>();
	uf::Serializer& metadata = this->getComponent<uf::Serializer>();

	this->addHook( "system:Quit.%UID%", [&](const std::string& event)->std::string{
		std::cout << event << std::endl;
		ext::ready = false;
		return "true";
	});

	this->addHook( "world:Music.LoadPrevious.%UID%", [&](const std::string& event)->std::string{
		uf::Serializer json = event;

		if ( metadata["previous bgm"]["filename"] == "" ) return "false";

		std::string filename = metadata["previous bgm"]["filename"].asString();
		float timestamp = metadata["previous bgm"]["timestamp"].asFloat();

//		std::cout << metadata["previous bgm"] << std::endl;

		uf::Audio& audio = this->getComponent<uf::Audio>();
		if ( audio.playing() ) {
			metadata["previous bgm"]["filename"] = audio.getFilename();
			metadata["previous bgm"]["timestamp"] = audio.getTime();
			audio.stop();
		}
		audio.load(filename);
		audio.setVolume(metadata["volumes"]["bgm"].asFloat());
		audio.setTime(timestamp);
		audio.play();

		return "true";
	});
	this->addHook( "asset:Load.%UID%", [&](const std::string& event)->std::string{	
		uf::Serializer json = event;
		std::string filename = json["filename"].asString();

		if ( uf::string::extension(filename) != "ogg" ) return "false";
		const uf::Audio* audioPointer = NULL;
		try { audioPointer = &assetLoader.get<uf::Audio>(filename); } catch ( ... ) {}
		if ( !audioPointer ) return "false";

		uf::Audio& audio = this->getComponent<uf::Audio>();
		if ( audio.playing() ) audio.stop();

		audio.load(filename);
		audio.setVolume(metadata["volumes"]["bgm"].asFloat());
		audio.play();

		return "true";
	});

	this->addHook( "menu:Pause", [&](const std::string& event)->std::string{
		static uf::Timer<long long> timer(false);
		if ( !timer.running() ) timer.start();
		if ( timer.elapsed().asDouble() < 1 ) return "false";
		timer.reset();

		uf::Serializer json = event;
		ext::Gui* manager = (ext::Gui*) this->findByName("Gui Manager");
		if ( !manager ) return "false";
		uf::Serializer payload;
		ext::Gui* gui = (ext::Gui*) manager->findByUid( (payload["uid"] = manager->loadChild("/scenes/worldscape/gui/pause/menu.json", false)).asUInt64() );
		uf::Serializer& metadata = gui->getComponent<uf::Serializer>();
		metadata["menu"] = json["menu"];
		gui->initialize();
		return payload;
	});
	this->addHook( "world:Entity.LoadAsset", [&](const std::string& event)->std::string{
		uf::Serializer json = event;

		std::string asset = json["asset"].asString();
		std::string uid = json["uid"].asString();

		assetLoader.load(asset, "asset:Load." + uid);

		return "true";
	});
	/* store viewport size */ {
		metadata["window"]["size"]["x"] = uf::renderer::width;
		metadata["window"]["size"]["y"] = uf::renderer::height;
		
		this->addHook( "window:Resized", [&](const std::string& event)->std::string{
			uf::Serializer json = event;

			pod::Vector2ui size; {
				size.x = json["window"]["size"]["x"].asUInt64();
				size.y = json["window"]["size"]["y"].asUInt64();
			}

			metadata["window"] = json["window"];

			return "true";
		});
	}

	// lock control
	{
		uf::Serializer payload;
		payload["state"] = false;
		uf::hooks.call("window:Mouse.CursorVisibility", payload);
		uf::hooks.call("window:Mouse.Lock");
	}
}
void ext::SceneBehavior::tick( uf::Object& self ) {
	uf::Serializer& metadata = this->getComponent<uf::Serializer>();
	uf::Asset& assetLoader = this->getComponent<uf::Asset>();

	/* check if audio needs to loop */ try {
		uf::Audio& bgm = this->getComponent<uf::Audio>();
		float current = bgm.getTime();
		float end = bgm.getDuration();
		float epsilon = 0.005f;
		if ( current + epsilon >= end || !bgm.playing() ) {
			// intro to main transition
			std::string filename = bgm.getFilename();
			filename = assetLoader.getOriginal(filename);
			if ( filename.find("_intro") != std::string::npos ) {
				assetLoader.load(uf::string::replace( filename, "_intro", "" ), "asset:Load." + std::to_string(this->getUid()));
			// loop
			} else {
				bgm.setTime(0);
				if ( !bgm.playing() ) bgm.play();
			}
		}
	} catch ( ... ) {

	}

	/* Regain control if nothing requests it */ {
		ext::Gui* menu = (ext::Gui*) this->findByName("Gui: Menu");
		if ( !menu ) {
			uf::Serializer payload;
			payload["state"] = false;
			uf::hooks.call("window:Mouse.CursorVisibility", payload);
			uf::hooks.call("window:Mouse.Lock");
		}
	}

	/* Print Entity Information */  {
		static uf::Timer<long long> timer(false);
		if ( !timer.running() ) timer.start();
		if ( uf::Window::isKeyPressed("U") && timer.elapsed().asDouble() >= 1 ) { timer.reset();
			auto& allocations = uf::Entity::memoryPool.allocations();
			uf::iostream << "Current size: " << allocations.size() << "\n"; //" | UIDs: " << uf::Entity::uids << "\n";
			uint orphans = 0;
			uint empty = 0;
			for ( auto& allocation : allocations ) {
				uf::Entity* e = (uf::Entity*) allocation.pointer;
				if ( !e->hasParent() ) {
					++orphans;
					uf::iostream << "Orphan: " << e->getName() << ": " << e << "\n";
				}
			}
			uf::iostream << "Orphans: " << orphans << "\n";
			uf::iostream << "Empty: " << empty << "\n";
		}
	}
	
	/* Updates Sound Listener */ {
		auto& controller = this->getController();
		auto& transform = controller.getComponent<pod::Transform<>>();
		
		ext::oal.listener( "POSITION", { transform.position.x, transform.position.y, transform.position.z } );
		ext::oal.listener( "VELOCITY", { 0, 0, 0 } );
		ext::oal.listener( "ORIENTATION", { 0, 0, 1, 1, 0, 0 } );
	}

	if ( uf::scene::getCurrentScene().getUid() == this->getUid() ) {
		/* Update lights */ if ( metadata["light"]["should"].asBool() ) {
		//	if ( !uf::renderer::currentRenderMode || uf::renderer::currentRenderMode->name != "" ) return;

			auto& scene = uf::scene::getCurrentScene();

			std::vector<uf::Graphic*> blitters;
			auto& renderMode = uf::renderer::getRenderMode("", true);
			bool hasCompute = uf::renderer::hasRenderMode("C:RT:" + std::to_string(this->getUid()), true);
			if ( hasCompute ) {
		//		auto& renderMode = uf::renderer::getRenderMode("C:RT:" + std::to_string(this->getUid()), true);
		//		auto* renderModePointer = (uf::renderer::ComputeRenderMode*) &renderMode;
		//		if ( renderModePointer->compute.initialized ) {
		//			blitters.push_back(&renderModePointer->compute);
		//		} else {
		//			hasCompute = false;
		//		}
			} else if ( renderMode.getType() == "Deferred (Stereoscopic)" ) {
				auto* renderModePointer = (uf::renderer::StereoscopicDeferredRenderMode*) &renderMode;
				blitters.push_back(&renderModePointer->blitters.left);
				blitters.push_back(&renderModePointer->blitters.right);
			} else if ( renderMode.getType() == "Deferred" ) {
				auto* renderModePointer = (uf::renderer::DeferredRenderMode*) &renderMode;
				blitters.push_back(&renderModePointer->blitter);
			}
			auto& controller = scene.getController();
			auto& camera = controller.getComponent<uf::Camera>();
		
		//	auto& uniforms = blitter.uniforms;
			struct UniformDescriptor {
				struct Matrices {
					alignas(16) pod::Matrix4f view[2];
					alignas(16) pod::Matrix4f projection[2];
				} matrices;
				alignas(16) pod::Vector4f ambient;
				struct {
					alignas(8) pod::Vector2f range;
					alignas(16) pod::Vector4f color;
				} fog;
				struct Light {
					alignas(16) pod::Vector4f position;
					alignas(16) pod::Vector4f color;
					alignas(8) pod::Vector2i type;
					alignas(16) pod::Matrix4f view;
					alignas(16) pod::Matrix4f projection;
				} lights;
			};
			
			struct SpecializationConstant {
				int32_t maxLights = 32;
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
					uniforms->fog.color.x = metadata["light"]["fog"]["color"][0].asFloat();
					uniforms->fog.color.y = metadata["light"]["fog"]["color"][1].asFloat();
					uniforms->fog.color.z = metadata["light"]["fog"]["color"][2].asFloat();

					uniforms->fog.range.x = metadata["light"]["fog"]["range"][0].asFloat();
					uniforms->fog.range.y = metadata["light"]["fog"]["range"][1].asFloat();
				}
				{
					std::vector<uf::Entity*> entities;
					std::function<void(uf::Entity*)> filter = [&]( uf::Entity* entity ) {
						if ( !entity || entity->getName() != "Light" ) return;
						entities.push_back(entity);
					};
					for ( uf::Scene* scene : uf::renderer::scenes ) { if ( !scene ) continue;
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

						light.position.w = metadata["light"]["radius"][1].asFloat();

						light.color.x = metadata["light"]["color"][0].asFloat();
						light.color.y = metadata["light"]["color"][1].asFloat();
						light.color.z = metadata["light"]["color"][2].asFloat();

						light.color.w = metadata["light"]["power"].asFloat();

						light.type.x = metadata["light"]["type"].asUInt64();
						light.type.y = metadata["light"]["shadows"]["enabled"].asBool();

						if ( !hasCompute && entity->hasComponent<uf::renderer::RenderTargetRenderMode>() ) {
							auto& renderMode = entity->getComponent<uf::renderer::RenderTargetRenderMode>();
							auto& renderTarget = renderMode.renderTarget;

							uint8_t i = 0;
							for ( auto& attachment : renderTarget.attachments ) {
								if ( !(attachment.usage & VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT) ) continue;
							//	if ( (attachment.usage & VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT) ) continue;
								if ( (attachment.layout == VK_IMAGE_LAYOUT_PRESENT_SRC_KHR) ) continue;
								auto& texture = blitter.material.textures.emplace_back();
								texture.aliasAttachment(attachment);
								light.type.y = true;
								break;
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
void ext::SceneBehavior::render( uf::Object& self ) {
}
void ext::SceneBehavior::destroy( uf::Object& self ) {
}
#undef this