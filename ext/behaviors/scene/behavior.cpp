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
#include <uf/utils/noise/noise.h>

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

	// initialize perlin noise
	{
		auto& texture = this->getComponent<uf::renderer::Texture3D>();
		texture.sampler.descriptor.addressMode = {
			uf::renderer::enums::AddressMode::MIRRORED_REPEAT,
			uf::renderer::enums::AddressMode::MIRRORED_REPEAT,
			uf::renderer::enums::AddressMode::MIRRORED_REPEAT
		};

		auto& noiseGenerator = this->getComponent<uf::PerlinNoise>();
		auto& metadata = this->getComponent<uf::Serializer>();
		noiseGenerator.seed(rand());

		float high = std::numeric_limits<float>::min();
		float low = std::numeric_limits<float>::max();
		float amplitude = metadata["noise"]["amplitude"].is<float>() ? metadata["noise"]["amplitude"].as<float>() : 1.5;
		pod::Vector3ui size = uf::vector::decode(metadata["noise"]["size"], pod::Vector3ui{256, 256, 256});
		pod::Vector3d coefficients = uf::vector::decode(metadata["noise"]["coefficients"], pod::Vector3d{3.0, 3.0, 3.0});

		std::vector<uint8_t> pixels(size.x * size.y * size.z);
		std::vector<float> perlins(size.x * size.y * size.z);
	#pragma omp parallel for
		for ( size_t z = 0; z < size.z; ++z ) {
		for ( size_t y = 0; y < size.y; ++y ) {
		for ( size_t x = 0; x < size.x; ++x ) {
			float nx = (float) x / (float) size.x;
			float ny = (float) y / (float) size.y;
			float nz = (float) z / (float) size.z;

			float n = amplitude * noiseGenerator.noise(coefficients.x * nx, coefficients.y * ny, coefficients.z * nz);
			high = std::max( high, n );
			low = std::min( low, n );
			perlins[x + y * size.x + z * size.x * size.y] = n;
		}
		}
		}
		for ( size_t i = 0; i < perlins.size(); ++i ) {
			float n = perlins[i];
			n = n - floor(n);
			float normalized = (n - low) / (high - low);
			if ( normalized >= 1.0f ) normalized = 1.0f;
			pixels[i] = static_cast<uint8_t>(floor(normalized * 255));
		}
		texture.fromBuffers( (void*) pixels.data(), pixels.size(), uf::renderer::enums::Format::R8_UNORM, size.x, size.y, size.z, 1 );
	}

	// initialize cubemap
	{
		std::vector<std::string> filenames = {
			"./data/textures/skybox/front.png",
			"./data/textures/skybox/back.png",
			"./data/textures/skybox/up.png",
			"./data/textures/skybox/down.png",
			"./data/textures/skybox/right.png",
			"./data/textures/skybox/left.png",
		};
		uf::Image::container_t pixels;
		std::vector<uf::Image> images(filenames.size());

		pod::Vector2ui size = {0,0};
		auto& texture = this->getComponent<uf::renderer::TextureCube>();
		for ( size_t i = 0; i < filenames.size(); ++i ) {
			auto& filename = filenames[i];
			auto& image = images[i];
			image.open(filename);
			image.flip();

			if ( size.x == 0 && size.y == 0 ) {
				size = image.getDimensions();
			} else if ( size != image.getDimensions() ) {
				std::cout << "ERROR: MISMATCH CUBEMAP FACE SIZE" << std::endl;
			}

			auto& p = image.getPixels();
			pixels.reserve( pixels.size() + p.size() );
			pixels.insert( pixels.end(), p.begin(), p.end() );
		}
		texture.mips = 0;
		texture.fromBuffers( (void*) pixels.data(), pixels.size(), uf::renderer::enums::Format::R8G8B8A8_UNORM, size.x, size.y, 1, filenames.size() );
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
	{
		uf::hooks.call("game:Frame.Start");
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

	/* Update lights */ if ( metadata["light"]["should"].as<bool>() ) {
#if UF_USE_VULKAN
		auto& scene = uf::scene::getCurrentScene();
		auto& controller = scene.getController();
		auto& camera = controller.getComponent<uf::Camera>();
		auto& renderMode = uf::renderer::getRenderMode("", true);
		auto& controllerMetadata = controller.getComponent<uf::Serializer>();
		auto& controllerTransform = controller.getComponent<pod::Transform<>>();
		std::vector<uf::Graphic*> blitters = renderMode.getBlitters();
		
		size_t maxTextures = metadata["system"]["config"]["engine"]["scenes"]["textures"]["max"].as<size_t>();

		struct UniformDescriptor {
			struct Matrices {
				alignas(16) pod::Matrix4f view[2];
				alignas(16) pod::Matrix4f projection[2];
				alignas(16) pod::Matrix4f iView[2];
				alignas(16) pod::Matrix4f iProjection[2];
				alignas(16) pod::Matrix4f iProjectionView[2];
			} matrices;
			struct Mode {
				alignas(8) pod::Vector2ui type;
				alignas(8) pod::Vector2ui padding;
				alignas(16) pod::Vector4f parameters;
			} mode;
			struct {
				alignas(16) pod::Vector4f color; // w: stepScale
				alignas(16) pod::Vector4f offset; // w: densityScale
				alignas(4) float densityThreshold;
				alignas(4) float densityMultiplier;

				alignas(4) float absorbtion;
				alignas(4) float padding1;

				alignas(8) pod::Vector2f range;
				alignas(4) float padding2;
				alignas(4) float padding3;
			} fog;
			struct {
				alignas(4) uint32_t lights = 0;
				alignas(4) uint32_t materials = 0;
				alignas(4) uint32_t textures = 0;
				alignas(4) uint32_t drawCalls = 0;
			} lengths;
			alignas(16) pod::Vector4f ambient;
		//	alignas(16) pod::Vector4f position;
		};
		struct SpecializationConstant {
			uint32_t maxTextures = 512;
		} specializationConstants;
		specializationConstants.maxTextures = maxTextures;

		struct LightInfo {
			uf::Entity* entity = NULL;
			pod::Vector3f position = {0,0,0};
			float distance = 0;
			bool shadows = false;
		};
		std::vector<LightInfo> entities;
		std::vector<pod::Graph*> graphs;

		this->process([&]( uf::Entity* entity ) { if ( !entity ) return;
			auto& metadata = entity->getComponent<uf::Serializer>();
			if ( entity == &controller ) return;
			if ( entity == this ) return;
			if ( entity->hasComponent<pod::Graph>() ) graphs.emplace_back(&entity->getComponent<pod::Graph>());
		//	if ( entity->hasComponent<pod::Graph>() && entity->hasComponent<uf::Graphic>() ) graphs.emplace_back(entity);
			//
			if ( entity->getName() != "Light" && !ext::json::isObject( metadata["light"] ) ) return;
			//
			if ( entity->hasComponent<uf::renderer::RenderTargetRenderMode>() ) {
				auto& renderMode = entity->getComponent<uf::renderer::RenderTargetRenderMode>();
				metadata["system"]["renderer"]["rendered"] = false;
				if ( metadata["system"]["renderer"]["mode"].as<std::string>() == "in-range" ) {
					renderMode.execute = false;
				}
			}
			// is a component of an shadowing point light
			if ( metadata["light"]["bound"].as<bool>() ) return;
			LightInfo& info = entities.emplace_back();
			auto& transform = entity->getComponent<pod::Transform<>>();
			auto flatten = uf::transform::flatten( transform );
			info.entity = entity;
			info.position = flatten.position;
			info.distance = uf::vector::magnitude( uf::vector::subtract( flatten.position, controllerTransform.position ) );
			info.shadows = metadata["light"]["shadows"].as<bool>();
		});
		std::sort( entities.begin(), entities.end(), [&]( LightInfo& l, LightInfo& r ){
			return l.distance < r.distance;
		});
	
		int shadowThreshold = metadata["system"]["config"]["engine"]["scenes"]["lights"]["shadow threshold"].as<size_t>();
		if ( shadowThreshold <= 0 ) shadowThreshold = std::numeric_limits<int>::max();
		{
			std::vector<LightInfo> scratch;
			scratch.reserve(entities.size());
			for ( size_t i = 0; i < entities.size(); ++i ) {
				auto& info = entities[i];
				auto& metadata = info.entity->getComponent<uf::Serializer>();
				if ( info.shadows && --shadowThreshold <= 0 ) info.shadows = false;
				scratch.emplace_back(info);
			}
			entities = scratch;
		}
	
		if ( controllerMetadata["light"]["should"].as<bool>() ) {
			auto& info = entities.emplace_back();
			info.entity = &controller;
			info.position = controllerTransform.position;
			info.distance = 0;
			info.shadows = false;
		}

		if ( !metadata["light"]["fog"]["step scale"].is<float>() ) metadata["light"]["fog"]["step scale"] = 16.0f;
		if ( !metadata["light"]["fog"]["absorbtion"].is<float>() ) metadata["light"]["fog"]["absorbtion"] = 0.85f;
		if ( !metadata["light"]["fog"]["density"]["threshold"].is<float>() ) metadata["light"]["fog"]["density"]["threshold"] = 0.5f;
		if ( !metadata["light"]["fog"]["density"]["multiplier"].is<float>() ) metadata["light"]["fog"]["density"]["multiplier"] = 5.0f;
		if ( !metadata["light"]["fog"]["density"]["scale"].is<float>() ) metadata["light"]["fog"]["density"]["scale"] = 50.0f;

		for ( auto* blitter : blitters ) {
			auto& graphic = *blitter;
			auto& shader = graphic.material.getShader("fragment");
			auto& uniform = shader.getUniform("UBO");
			uint8_t* buffer = (uint8_t*) (void*) uniform;

			UniformDescriptor* uniforms = (UniformDescriptor*) buffer;

			for ( std::size_t i = 0; i < 2; ++i ) {
				uniforms->matrices.view[i] = camera.getView( i );
				uniforms->matrices.projection[i] = camera.getProjection( i );
				uniforms->matrices.iView[i] = uf::matrix::inverse( uniforms->matrices.view[i] );
				uniforms->matrices.iProjection[i] = uf::matrix::inverse( uniforms->matrices.projection[i] );
				uniforms->matrices.iProjectionView[i] = uf::matrix::inverse( uniforms->matrices.projection[i] * uniforms->matrices.view[i] );
			}

			uniforms->ambient = uf::vector::decode( metadata["light"]["ambient"], uniforms->ambient );
		/*
			pod::Transform<> transform = controller.getComponent<pod::Transform<>>();
			if ( controller.hasComponent<uf::Camera>() ) {
				auto& camera = controller.getComponent<uf::Camera>();
				transform.position += camera.getTransform().position;
				transform = uf::transform::reorient( transform );
			}
			uniforms->position = transform.position;
		*/

			uniforms->fog.color = uf::vector::decode( metadata["light"]["fog"]["color"], uniforms->fog.color );
			uniforms->fog.color.w = metadata["light"]["fog"]["step scale"].as<float>();

			float timescale = metadata["light"]["fog"]["density"]["timescale"].as<float>();
			uniforms->fog.offset = uf::vector::decode( metadata["light"]["fog"]["density"]["offset"], uniforms->fog.offset ) * uf::physics::time::current * timescale;
			uniforms->fog.offset.w = metadata["light"]["fog"]["density"]["scale"].as<float>();

			uniforms->fog.densityThreshold = metadata["light"]["fog"]["density"]["threshold"].as<float>();
			uniforms->fog.densityMultiplier = metadata["light"]["fog"]["density"]["multiplier"].as<float>();
			uniforms->fog.absorbtion = metadata["light"]["fog"]["absorbtion"].as<float>();

			uniforms->fog.range = uf::vector::decode( metadata["light"]["fog"]["range"], uniforms->fog.range );

			uniforms->mode.type.x = metadata["system"]["renderer"]["shader"]["mode"].as<size_t>();
			uniforms->mode.type.y = metadata["system"]["renderer"]["shader"]["scalar"].as<size_t>();
			
			uniforms->mode.parameters = uf::vector::decode( metadata["system"]["renderer"]["shader"]["parameters"], uniforms->mode.parameters );
			if ( metadata["system"]["renderer"]["shader"]["parameters"][3].as<std::string>() == "time" ) {
				uniforms->mode.parameters.w = uf::physics::time::current;
			}

			std::vector<VkImage> previousTextures;
			for ( auto& texture : graphic.material.textures ) previousTextures.emplace_back(texture.image);

			graphic.material.textures.clear();
			// add noise texture
			graphic.material.textures.emplace_back().aliasTexture(this->getComponent<uf::renderer::Texture3D>());
			graphic.material.textures.emplace_back().aliasTexture(this->getComponent<uf::renderer::TextureCube>());

			size_t updateThreshold = metadata["system"]["config"]["engine"]["scenes"]["lights"]["update threshold"].as<size_t>();
			size_t maxLights = metadata["system"]["config"]["engine"]["scenes"]["lights"]["max"].as<size_t>();
			size_t textureSlot = 0;

			std::vector<pod::Light::Storage> lights;
			lights.reserve( maxLights );

			std::vector<pod::Material::Storage> materials;
			materials.reserve(maxTextures);
			materials.emplace_back().colorBase = {0,0,0,0};

			std::vector<pod::Texture::Storage> textures;
			textures.reserve(maxTextures);
			
			std::vector<pod::DrawCall::Storage> drawCalls;
			drawCalls.reserve(maxTextures);

			// add materials
			{
				for ( auto* entity : graphs ) {
					auto& graph = *entity;

					drawCalls.emplace_back(pod::DrawCall::Storage{
						materials.size(),
						graph.materials.size(),
						textures.size(),
						graph.textures.size()
					});
					
					for ( auto& material : graph.materials ) materials.emplace_back( material.storage );
					for ( auto& texture : graph.textures ) textures.emplace_back( texture.storage );

					for ( auto& texture : graph.textures ) {
						if ( !texture.texture.device ) continue;

						graphic.material.textures.emplace_back().aliasTexture(texture.texture);
						++textureSlot;

						if ( graph.atlas ) break;
					}
				}

				uniforms->lengths.materials = std::min( materials.size(), maxTextures );
				uniforms->lengths.textures = std::min( textures.size(), maxTextures );
				uniforms->lengths.drawCalls = std::min( drawCalls.size(), maxTextures );
			}
			// add lighting
			for ( size_t i = 0; i < entities.size() && lights.size() < maxLights; ++i ) {
				auto& info = entities[i];
				uf::Entity* entity = info.entity;

				auto& transform = entity->getComponent<pod::Transform<>>();
				auto& metadata = entity->getComponent<uf::Serializer>();
				auto& camera = entity->getComponent<uf::Camera>();
				metadata["system"]["renderer"]["rendered"] = true;

				pod::Light::Storage light;
				light.position = info.position;

				light.color = uf::vector::decode( metadata["light"]["color"], light.color );
				light.color.w = metadata["light"]["power"].as<float>();

				if ( metadata["light"]["type"].is<size_t>() ) {
					light.type = metadata["light"]["type"].as<size_t>();
				} else if ( metadata["light"]["type"].is<std::string>() ) {
					std::string lightType = metadata["light"]["type"].as<std::string>();
					if ( lightType == "point" ) light.type = 1;
					else if ( lightType == "spot" ) light.type = 2;
				}

				light.mapIndex = -1;

				light.depthBias = metadata["light"]["bias"]["shader"].as<float>();
				if ( info.shadows && entity->hasComponent<uf::renderer::RenderTargetRenderMode>() ) {
					auto& renderMode = entity->getComponent<uf::renderer::RenderTargetRenderMode>();
					if ( metadata["system"]["renderer"]["mode"].as<std::string>() == "in-range" && --updateThreshold > 0 ) {
						renderMode.execute = true;
					}
					size_t view = 0;
					for ( auto& attachment : renderMode.renderTarget.attachments ) {
						if ( !(attachment.descriptor.usage & VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT) ) continue;
						if ( attachment.descriptor.layout == VK_IMAGE_LAYOUT_PRESENT_SRC_KHR ) continue;

						graphic.material.textures.emplace_back().aliasAttachment(attachment);

						light.mapIndex = textureSlot++;
						light.view = camera.getView(view);
						light.projection = camera.getProjection(view);
						lights.emplace_back(light);

						++view;
					}
					light.mapIndex = -1;
				} else {
					lights.emplace_back(light);
				}
				uniforms->lengths.lights = std::min( lights.size(), maxLights );
			}

			{
				bool shouldUpdate = graphic.material.textures.size() != previousTextures.size();
				for ( size_t i = 0; !shouldUpdate && i < previousTextures.size() && i < graphic.material.textures.size(); ++i ) {
					if ( previousTextures[i] != graphic.material.textures[i].image )
						shouldUpdate = true;
				}
				if ( shouldUpdate ) {
					size_t lightBufferIndex = renderMode.metadata["lightBufferIndex"].as<size_t>();
					size_t materialBufferIndex = renderMode.metadata["materialBufferIndex"].as<size_t>();
					size_t textureBufferIndex = renderMode.metadata["textureBufferIndex"].as<size_t>();
					size_t drawCallBufferIndex = renderMode.metadata["drawCallBufferIndex"].as<size_t>();

					graphic.updateBuffer( (void*) lights.data(), uniforms->lengths.lights * sizeof(pod::Light::Storage), lightBufferIndex, false );
					graphic.updateBuffer( (void*) materials.data(), uniforms->lengths.materials * sizeof(pod::Material::Storage), materialBufferIndex, false );
					graphic.updateBuffer( (void*) textures.data(), uniforms->lengths.textures * sizeof(pod::Texture::Storage), textureBufferIndex, false );
					graphic.updateBuffer( (void*) drawCalls.data(), uniforms->lengths.drawCalls * sizeof(pod::DrawCall::Storage), drawCallBufferIndex, false );

					graphic.updatePipelines();
				}
				shader.updateUniform( "UBO", uniform );	
			}
		}
#endif
	}
}
void ext::ExtSceneBehavior::render( uf::Object& self ) {}
void ext::ExtSceneBehavior::destroy( uf::Object& self ) {}
#undef this