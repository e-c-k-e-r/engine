#include "behavior.h"

#include <uf/utils/time/time.h>
#include <uf/utils/io/iostream.h>
#include <uf/utils/math/vector.h>
#include <uf/utils/math/transform.h>
#include <uf/utils/window/window.h>
#include <uf/utils/graphic/graphic.h>

#include <uf/utils/audio/audio.h>
#include <uf/utils/thread/thread.h>
#include <uf/utils/camera/camera.h>
#include <uf/utils/math/physics.h>

#include <uf/engine/asset/asset.h>
#include <uf/engine/asset/masterdata.h>

#include <uf/utils/renderer/renderer.h>

#include <uf/ext/gltf/gltf.h>

#include <uf/utils/math/collision.h>

#include "../../ext.h"
#include "../../gui/gui.h"

UF_BEHAVIOR_REGISTER_CPP(ext::ExtSceneBehavior)
#define this ((uf::Scene*) &self)
void ext::ExtSceneBehavior::initialize( uf::Object& self ) {
	uf::Asset& assetLoader = this->getComponent<uf::Asset>();
	uf::Serializer& metadata = this->getComponent<uf::Serializer>();

	this->addHook( "system:Quit.%UID%", [&](ext::json::Value& json){
		std::cout << json << std::endl;
		ext::ready = false;
	});

	this->addHook( "world:Music.LoadPrevious.%UID%", [&](ext::json::Value& json){

		if ( metadata["previous bgm"]["filename"] == "" ) return;

		std::string filename = metadata["previous bgm"]["filename"].as<std::string>();
		float timestamp = metadata["previous bgm"]["timestamp"].as<float>();

//		std::cout << metadata["previous bgm"] << std::endl;

		uf::Audio& audio = this->getComponent<uf::Audio>();
		if ( audio.playing() ) {
			metadata["previous bgm"]["filename"] = audio.getFilename();
			metadata["previous bgm"]["timestamp"] = audio.getTime();
			audio.stop();
		}
		audio.load(filename);
		audio.setVolume(metadata["volumes"]["bgm"].as<float>());
		audio.setTime(timestamp);
		audio.play();
	});
	this->addHook( "asset:Load.%UID%", [&](ext::json::Value& json){
		std::string filename = json["filename"].as<std::string>();

		if ( uf::io::extension(filename) != "ogg" ) return;
		const uf::Audio* audioPointer = NULL;
		try { audioPointer = &assetLoader.get<uf::Audio>(filename); } catch ( ... ) {}
		if ( !audioPointer ) return;

		uf::Audio& audio = this->getComponent<uf::Audio>();
		if ( audio.playing() ) audio.stop();

		audio.load(filename);
		audio.setVolume(metadata["volumes"]["bgm"].as<float>());
		audio.play();
	});

	this->addHook( "menu:Pause", [&](ext::json::Value& json){
		static uf::Timer<long long> timer(false);
		if ( !timer.running() ) timer.start( uf::Time<>(-1000000) );
		if ( timer.elapsed().asDouble() < 1 ) return;
		timer.reset();

		uf::Object* manager = (uf::Object*) this->globalFindByName("Gui Manager");
		if ( !manager ) return;
		uf::Serializer payload;
		std::string config = metadata["menus"]["pause"].is<std::string>() ? metadata["menus"]["pause"].as<std::string>() : "/scenes/worldscape/gui/pause/menu.json";
		uf::Object& gui = manager->loadChild(config, false);
		payload["uid"] = gui.getUid();

		uf::Serializer& metadata = gui.getComponent<uf::Serializer>();
		metadata["menu"] = json["menu"];
		
		gui.initialize();
	//	return payload;
	});
	this->addHook( "world:Entity.LoadAsset", [&](ext::json::Value& json){
		std::string asset = json["asset"].as<std::string>();
		std::string uid = json["uid"].as<std::string>();

		assetLoader.load(asset, "asset:Load." + uid);
	});
	this->addHook( "shader:Update.%UID%", [&](ext::json::Value& _json){
		uf::Serializer json = _json;
		json["mode"] = json["mode"].as<size_t>() | metadata["system"]["renderer"]["shader"]["mode"].as<size_t>();
		metadata["system"]["renderer"]["shader"]["mode"] = json["mode"];
		metadata["system"]["renderer"]["shader"]["parameters"] = json["parameters"];
	});
	/* store viewport size */ {
//		metadata["system"]["window"]["size"]["x"] = uf::renderer::settings::width;
//		metadata["system"]["window"]["size"]["y"] = uf::renderer::settings::height;
//		ext::gui::size.current.x = uf::renderer::settings::width;
//		ext::gui::size.current.y = uf::renderer::settings::height;
		
		this->addHook( "window:Resized", [&](ext::json::Value& json){
			pod::Vector2ui size; {
				size.x = json["window"]["size"]["x"].as<size_t>();
				size.y = json["window"]["size"]["y"].as<size_t>();
			}

			metadata["system"]["window"] = json["system"]["window"];
			ext::gui::size.current = size;
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
void ext::ExtSceneBehavior::tick( uf::Object& self ) {
	uf::Serializer& metadata = this->getComponent<uf::Serializer>();
	uf::Asset& assetLoader = this->getComponent<uf::Asset>();

	/* check if audio needs to loop */ {
		auto& bgm = this->getComponent<uf::Audio>();
		float current = bgm.getTime();
		float end = bgm.getDuration();
		float epsilon = 0.005f;
		if ( current + epsilon >= end || !bgm.playing() ) {
			// intro to main transition
			std::string filename = bgm.getFilename();
			filename = assetLoader.getOriginal(filename);
			if ( filename.find("_intro") != std::string::npos ) {
				assetLoader.load(uf::string::replace( filename, "_intro", "" ), this->formatHookName("asset:Load.%UID%"));
			// loop
			} else {
				bgm.setTime(0);
				if ( !bgm.playing() ) bgm.play();
			}
		}
	}

	/* Regain control if nothing requests it */ {
		uf::Object* menu = (uf::Object*) this->globalFindByName("Gui: Menu");
		if ( !menu ) {
			uf::Serializer payload;
			payload["state"] = false;
			uf::hooks.call("window:Mouse.CursorVisibility", payload);
			uf::hooks.call("window:Mouse.Lock");
		} else {
			uf::Serializer payload;
			payload["state"] = true;
			uf::hooks.call("window:Mouse.CursorVisibility", payload);
		}
	}

	/* Print World Tree */ {
		TIMER(1, uf::Window::isKeyPressed("U") && ) {
			std::function<void(uf::Entity*, int)> filter = []( uf::Entity* entity, int indent ) {
				for ( int i = 0; i < indent; ++i ) uf::iostream << "\t";
				uf::iostream << uf::string::toString(entity->as<uf::Object>()) << " ";
				if ( entity->hasComponent<pod::Transform<>>() ) {
					pod::Transform<> t = uf::transform::flatten(entity->getComponent<pod::Transform<>>());
					uf::iostream << uf::string::toString(t.position) << " " << uf::string::toString(t.orientation);
				}
				uf::iostream << "\n";
			};
			for ( uf::Scene* scene : uf::scene::scenes ) {
				if ( !scene ) continue;
				uf::iostream << "Scene: " << scene->getName() << ": " << scene << "\n";
				scene->process(filter, 1);
			}
		}
	}
	/* Print World Tree */ {
		TIMER(1, uf::Window::isKeyPressed("U") && false && ) {
			std::function<void(uf::Entity*, int)> filter = []( uf::Entity* entity, int indent ) {
				for ( int i = 0; i < indent; ++i ) uf::iostream << "\t";
				uf::iostream << uf::string::toString(entity->as<uf::Object>()) << " [";
				for ( auto& behavior : entity->getBehaviors() ) {
					uf::iostream << uf::instantiator::behaviors->names[behavior.type] << ", ";
				}
				uf::iostream << "]\n";
			};
			for ( uf::Scene* scene : uf::scene::scenes ) {
				if ( !scene ) continue;
				uf::iostream << "Scene: " << scene->getName() << ": " << scene << "\n";
				scene->process(filter, 1);
			}
			uf::Serializer instantiator;
			{
				int i = 0;
				for ( auto& pair : uf::instantiator::objects->names ) {
					instantiator["objects"][i++] = pair.second;
				}
			}
			{
				int i = 0;
				for ( auto& pair : uf::instantiator::behaviors->names ) {
					instantiator["behaviors"][i++] = pair.second;
				}
			}
			uf::iostream << instantiator << "\n";
		}
	}
	
	/* Updates Sound Listener */ {
		auto& controller = this->getController();
		// copy
		pod::Transform<> transform = controller.getComponent<pod::Transform<>>();
		if ( controller.hasComponent<uf::Camera>() ) {
			auto& camera = controller.getComponent<uf::Camera>();
			transform.position += camera.getTransform().position;
			transform = uf::transform::reorient( transform );
		}
		transform.forward *= -1;
		ext::oal.listener( "POSITION", { transform.position.x, transform.position.y, transform.position.z } );
		ext::oal.listener( "VELOCITY", { 0, 0, 0 } );
		ext::oal.listener( "ORIENTATION", { transform.forward.x, transform.forward.y, transform.forward.z, transform.up.x, transform.up.y, transform.up.z } );
	}

	if ( uf::scene::getCurrentScene().getUid() == this->getUid() ) {
		/* Update lights */ if ( metadata["light"]["should"].as<bool>() ) {
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
			} else if ( renderMode.getType() == "Deferred (Stereoscopic, Multiview)" ) {
				auto* renderModePointer = (uf::renderer::MultiviewStereoscopicDeferredRenderMode*) &renderMode;
				blitters.push_back(&renderModePointer->blitter);
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
				struct Mode {
					alignas(8) pod::Vector2ui type;
					alignas(8) pod::Vector2ui padding;
					alignas(16) pod::Vector4f parameters;
				} mode;
				struct {
					alignas(16) pod::Vector4f color;
					alignas(8) pod::Vector2f range;
					alignas(8) pod::Vector2f padding;
				} fog;
				struct Light {
					alignas(16) pod::Vector4f position;
					alignas(16) pod::Vector4f color;
					alignas(4) uint32_t type;
					alignas(4) float depthBias;
					alignas(8) pod::Vector2f padding;
					alignas(16) pod::Matrix4f view;
					alignas(16) pod::Matrix4f projection;
				} lights;
			};
			
			struct SpecializationConstant {
				uint32_t maxLights = 32;
			} specializationConstants;

			std::vector<uf::Entity*> entities;
			{
				std::function<void(uf::Entity*)> filter = [&]( uf::Entity* entity ) {
					if ( !entity ) return;
					bool isLight = entity->getName() == "Light";
					auto& metadata = entity->getComponent<uf::Serializer>();
					if ( ext::json::isObject( metadata["light"] ) ) isLight = true;
					if ( !isLight ) return;
					entities.push_back(entity);
				};
				for ( uf::Scene* scene : uf::scene::scenes ) { if ( !scene ) continue;
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
					if ( metadata["light"]["should"].as<bool>() ) entities.push_back(&controller);
				}
			}
			for ( size_t _ = 0; _ < blitters.size(); ++_ ) {
				auto& blitter = *blitters[_];
				
				uint8_t* buffer = NULL;
				size_t len = 0;
				auto* shader = &blitter.material.shaders.front();
				
				for ( auto& _ : blitter.material.shaders ) {
					if ( _.uniforms.empty() ) continue;
					auto& userdata = _.uniforms.front();
					buffer = (uint8_t*) (void*) userdata;
					len = userdata.data().len;
					shader = &_;
					specializationConstants = *((SpecializationConstant*) &_.specializationConstants[0]);
				}

				if ( !buffer ) continue;

				UniformDescriptor* uniforms = (UniformDescriptor*) buffer;
				for ( std::size_t i = 0; i < 2; ++i ) {
					uniforms->matrices.view[i] = camera.getView( i );
					uniforms->matrices.projection[i] = camera.getProjection( i );
				}
				{
					uniforms->ambient.x = metadata["light"]["ambient"][0].as<float>();
					uniforms->ambient.y = metadata["light"]["ambient"][1].as<float>();
					uniforms->ambient.z = metadata["light"]["ambient"][2].as<float>();
					uniforms->ambient.w = metadata["light"]["kexp"].as<float>();
				}
				{
					uniforms->fog.color.x = metadata["light"]["fog"]["color"][0].as<float>();
					uniforms->fog.color.y = metadata["light"]["fog"]["color"][1].as<float>();
					uniforms->fog.color.z = metadata["light"]["fog"]["color"][2].as<float>();

					uniforms->fog.range.x = metadata["light"]["fog"]["range"][0].as<float>();
					uniforms->fog.range.y = metadata["light"]["fog"]["range"][1].as<float>();
				}
				{
					uniforms->mode.type.x = metadata["system"]["renderer"]["shader"]["mode"].as<size_t>();
					uniforms->mode.type.y = metadata["system"]["renderer"]["shader"]["scalar"].as<size_t>();
					uniforms->mode.parameters.x = metadata["system"]["renderer"]["shader"]["parameters"][0].as<float>();
					uniforms->mode.parameters.y = metadata["system"]["renderer"]["shader"]["parameters"][1].as<float>();
					uniforms->mode.parameters.z = metadata["system"]["renderer"]["shader"]["parameters"][2].as<float>();

					if ( metadata["system"]["renderer"]["shader"]["parameters"][3].as<std::string>() == "time" ) {
						uniforms->mode.parameters.w = uf::physics::time::current;
					} else {
						uniforms->mode.parameters.w = metadata["system"]["renderer"]["shader"]["parameters"][3].as<float>();
					}
				}
				{
					UniformDescriptor::Light* lights = (UniformDescriptor::Light*) &buffer[sizeof(UniformDescriptor) - sizeof(UniformDescriptor::Light)];
					for ( size_t i = 0; i < specializationConstants.maxLights; ++i ) {
						UniformDescriptor::Light& light = lights[i];
						light.position = { 0, 0, 0, 0 };
						light.color = { 0, 0, 0, 0 };
						light.type = 0;
						light.depthBias = 0;
					}

					std::vector<VkImage> previousTextures;
					for ( auto& texture : blitter.material.textures ) {
						previousTextures.emplace_back(texture.image);
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

						if ( entity == &controller ) {
							light.position.x += metadata["light"]["position"][0].as<float>();
							light.position.y += metadata["light"]["position"][1].as<float>();
							light.position.z += metadata["light"]["position"][2].as<float>();
						}

						light.position.w = metadata["light"]["radius"][1].as<float>();

						light.color.x = metadata["light"]["color"][0].as<float>();
						light.color.y = metadata["light"]["color"][1].as<float>();
						light.color.z = metadata["light"]["color"][2].as<float>();

						light.color.w = metadata["light"]["power"].as<float>();

						light.type = metadata["light"]["type"].as<size_t>();
						light.depthBias = metadata["light"]["shadows"]["bias"].as<float>();

						if ( !hasCompute && entity->hasComponent<uf::renderer::RenderTargetRenderMode>() ) {
							auto& renderMode = entity->getComponent<uf::renderer::RenderTargetRenderMode>();
							auto& renderTarget = renderMode.renderTarget;

							uint8_t i = 0;
							for ( auto& attachment : renderTarget.attachments ) {
								if ( !(attachment.usage & VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT) ) continue;
								if ( attachment.layout == VK_IMAGE_LAYOUT_PRESENT_SRC_KHR ) continue;
								auto& texture = blitter.material.textures.emplace_back();
								texture.aliasAttachment(attachment);
								if ( light.type == 0 ) light.type = 1;
								break;
							}
						}
					}
					size_t i = 0;
					bool shouldUpdate = blitter.material.textures.size() != previousTextures.size();
					while ( !shouldUpdate && i < blitter.material.textures.size() ) {
						auto& texture = blitter.material.textures[i];
						auto& previousTexture = previousTextures[i];
						if ( texture.image != previousTexture ) shouldUpdate = true;
						++i;
					}
					// check if we actually do need to update our pipeline, to avoid the command buffer rebuild cost
					if ( shouldUpdate ) {
					//	blitter.getPipeline().update( blitter );
						blitter.updatePipelines();
					}
				}
				shader->updateBuffer( (void*) buffer, len, 0, false );
				
			}
		}
	}
}
void ext::ExtSceneBehavior::render( uf::Object& self ) {
}
void ext::ExtSceneBehavior::destroy( uf::Object& self ) {
}
#undef this