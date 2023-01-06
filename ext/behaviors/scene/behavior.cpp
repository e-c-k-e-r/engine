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

#include <uf/utils/io/inputs.h>
#include <uf/utils/renderer/renderer.h>
#include <uf/utils/noise/noise.h>

#include <uf/ext/gltf/gltf.h>

#include <uf/utils/math/collision.h>
#include <uf/utils/window/payloads.h>

#include <uf/ext/ext.h>
#include <uf/ext/ffx/fsr.h>

#include "../light/behavior.h"
#include "../voxelizer/behavior.h"
#include "../raytrace/behavior.h"
#include "../../ext.h"
#include "../../gui/gui.h"

UF_BEHAVIOR_REGISTER_CPP(ext::ExtSceneBehavior)
UF_BEHAVIOR_TRAITS_CPP(ext::ExtSceneBehavior, ticks = true, renders = false, multithread = false) // hangs on initialization
#define this ((uf::Scene*) &self)
void ext::ExtSceneBehavior::initialize( uf::Object& self ) {
//	auto& assetLoader = this->getComponent<uf::asset>();
	auto& metadata = this->getComponent<ext::ExtSceneBehavior::Metadata>();
	auto& metadataJson = this->getComponent<uf::Serializer>();

	this->addHook( "system:Quit.%UID%", [&](ext::json::Value& payload){
		uf::renderer::settings::experimental::dedicatedThread = false;
		ext::ready = false;
	});

	this->addHook( "asset:Load.%UID%", [&](pod::payloads::assetLoad& payload){
		if ( !uf::asset::isExpected( payload, uf::asset::Type::AUDIO ) ) return;
	//	if ( !uf::asset::has( payload ) ) uf::asset::load( payload );
		auto& audio = uf::asset::get<uf::Audio>( payload );
	//	if ( !uf::asset::has(payload.filename) ) uf::asset::load( payload );
	//	auto& audio = this->getComponent<uf::Audio>();

	#if UF_AUDIO_MAPPED_VOLUMES
		audio.setVolume(uf::audio::volumes.count("bgm") > 0 ? uf::audio::volumes.at("bgm") : 1.0);
	#else
		audio.setVolume(uf::audio::volumes::bgm);
	#endif
		audio.loop( true );
		audio.play();

		if ( !payload.asComponent ) {
			this->moveComponent<uf::Audio>( uf::asset::get( payload.filename ) );
		}
	});

	this->addHook( "menu:Open", [&](pod::payloads::menuOpen& payload){
		TIMER(1) {
			uf::Object* manager = (uf::Object*) this->globalFindByName("Gui Manager");
			if ( !manager ) return;

			uf::stl::string key = payload.name;
			uf::Object& gui = manager->loadChild(metadataJson["menus"][key].as<uf::stl::string>("/entites/gui/"+key+"/menu.json"), false);
			uf::Serializer& metadataJson = gui.getComponent<uf::Serializer>();
			gui.initialize();
		};
	});
	this->addHook( "world:Entity.LoadAsset", [&](pod::payloads::assetLoad& payload){
		if ( !payload.object ) return;

		uf::asset::load("asset:Load." + std::to_string(payload.object.uid), payload);
	});
	this->addHook( "shader:Update.%UID%", [&](ext::json::Value& json){
		metadata.shader.mode = json["mode"].as<uint32_t>();
		metadata.shader.scalar = json["scalar"].as<uint32_t>();
		metadata.shader.parameters = uf::vector::decode( json["parameters"], metadata.shader.parameters );
		ext::json::forEach( metadataJson["system"]["renderer"]["shader"]["parameters"], [&]( uint32_t i, const ext::json::Value& value ){
			if ( value.as<uf::stl::string>() == "time" ) metadata.shader.time = i;
		});
		if ( 0 <= metadata.shader.time && metadata.shader.time < 4 ) {
			metadata.shader.parameters[metadata.shader.time] = uf::physics::time::current;
		}
	});
	/* store viewport size */	
#if 0
	this->addHook( "window:Resized", [&](pod::payloads::windowResized& payload){
		ext::gui::size.current = payload.window.size;
	});
#endif
	/* Spawn an entity */
	this->addHook( "entity:Spawn", [&](pod::payloads::entitySpawn& payload){
		if ( payload.parent.uid ) {
			payload.parent.pointer = (uf::Object*) this->globalFindByUid(payload.parent.uid);
		}
		if ( !payload.parent.pointer ) payload.parent.pointer = this;

		uf::Object* child = NULL;

		if ( payload.filename != "" ) child = payload.parent.pointer->loadChildPointer( payload.filename );
		else child = payload.parent.pointer->loadChildPointer( payload.metadata );


		if ( !child ) return;
		auto& transform = child->getComponent<pod::Transform<>>();
		if ( payload.transform.position != pod::Vector3f{0,0,0} ) transform.position = payload.transform.position;
		if ( payload.transform.orientation != uf::quaternion::identity() ) transform.orientation = payload.transform.orientation;
		if ( !payload.transform.reference ) transform.reference = payload.transform.reference;
	});

	UF_BEHAVIOR_METADATA_BIND_SERIALIZER_HOOKS(metadata, metadataJson);

	// lock control
	{
		pod::payloads::windowMouseCursorVisibility payload;
		payload.mouse.visible = false;
		uf::hooks.call("window:Mouse.CursorVisibility", payload);
		uf::hooks.call("window:Mouse.Lock");
	}

	auto& sceneTextures = this->getComponent<pod::SceneTextures>();
	// initialize perlin noise
	#if UF_USE_VULKAN
	{
		auto& texture = sceneTextures.noise; //this->getComponent<uf::renderer::Texture3D>();
		texture.sampler.descriptor.addressMode = {
			uf::renderer::enums::AddressMode::MIRRORED_REPEAT,
			uf::renderer::enums::AddressMode::MIRRORED_REPEAT,
			uf::renderer::enums::AddressMode::MIRRORED_REPEAT
		};

		auto& noiseGenerator = this->getComponent<uf::PerlinNoise>();
		noiseGenerator.seed(rand());

		float high = std::numeric_limits<float>::min();
		float low = std::numeric_limits<float>::max();
		float amplitude = metadataJson["noise"]["amplitude"].is<float>() ? metadataJson["noise"]["amplitude"].as<float>() : 1.5;
		pod::Vector3ui size = uf::vector::decode(metadataJson["noise"]["size"], pod::Vector3ui{64, 64, 64});
		pod::Vector3d coefficients = uf::vector::decode(metadataJson["noise"]["coefficients"], pod::Vector3d{3.0, 3.0, 3.0});

		uf::stl::vector<uint8_t> pixels(size.x * size.y * size.z);
		uf::stl::vector<float> perlins(size.x * size.y * size.z);
	#pragma omp parallel for
		for ( uint32_t z = 0; z < size.z; ++z ) {
		for ( uint32_t y = 0; y < size.y; ++y ) {
		for ( uint32_t x = 0; x < size.x; ++x ) {
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
		for ( uint32_t i = 0; i < perlins.size(); ++i ) {
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
		const uf::stl::vector<uf::stl::string> filenames = {
			"front",
			"back",
			"up",
			"down",
			"right",
			"left",
		};
		uf::Image::container_t pixels;
		uf::stl::vector<uf::Image> images(filenames.size());

		pod::Vector2ui size = {0,0};
		auto& texture = sceneTextures.skybox;
		for ( uint32_t i = 0; i < filenames.size(); ++i ) {
			auto filename = uf::string::replace( this->resolveURI(metadata.sky.box.filename), "%d", filenames[i] );
			auto& image = images[i];
			image.open(filename);
			image.flip();

			if ( size.x == 0 && size.y == 0 ) size = image.getDimensions();
			else if ( size != image.getDimensions() ) UF_EXCEPTION("ERROR: MISMATCH CUBEMAP FACE SIZE");

			auto& p = image.getPixels();
			pixels.reserve( pixels.size() + p.size() );
			pixels.insert( pixels.end(), p.begin(), p.end() );
		}
	//	texture.mips = 0;
		if ( size.x > 0 && size.y > 0 ) {
			texture.fromBuffers( (void*) pixels.data(), pixels.size(), uf::renderer::enums::Format::R8G8B8A8_UNORM, size.x, size.y, 1, filenames.size() );
		}
	}
	#endif

	if ( false ) {
		uf::physics::terminate();
		uf::graph::destroy();
		
		uf::physics::initialize();
		uf::graph::initialize();
	}
}
void ext::ExtSceneBehavior::tick( uf::Object& self ) {
//	auto& assetLoader = this->getComponent<uf::asset>();
	uf::asset::processQueue();
	
	auto& metadata = this->getComponent<ext::ExtSceneBehavior::Metadata>();
	auto& metadataJson = this->getComponent<uf::Serializer>();
	uf::hooks.call("game:Frame.Start");

	++metadata.shader.frameAccumulate;
	if ( !metadata.shader.frameAccumulateReset && metadata.shader.frameAccumulateLimit && metadata.shader.frameAccumulate > metadata.shader.frameAccumulateLimit ) {
		metadata.shader.frameAccumulateReset = true;
	}
	if ( metadata.shader.frameAccumulateReset ) {
		metadata.shader.frameAccumulate = 0;
		metadata.shader.frameAccumulateReset = false;
	}

	static bool lagged = false;
	/* funi sound */ if ( lagged || uf::time::current - uf::time::previous > 5 ) {
		lagged = true;
		auto& controller = this->getController().as<uf::Object>();
		if ( controller.getUid() != this->getUid() ) {
			lagged = false;
			ext::json::Value payload;
			payload["filename"] = "/ui/pl_drown1.ogg";
			payload["volume"] = "sfx";
			payload["spatial"] = true;
			controller.queueHook("sound:Emit.%UID%", payload);
		}
	}

//	uf::renderer::states::frameAccumulate = metadata.shader.frameAccumulate;
//	uf::renderer::states::frameAccumulateReset = metadata.shader.frameAccumulateReset;

	/* Print World Tree */ {
		TIMER(1, uf::inputs::kbm::states::U || uf::scene::printTaskCalls )
		{
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
#if UF_USE_VULKAN
	/* Screenshot */ {
		TIMER(1, uf::inputs::kbm::states::F11 ) {
			auto& renderMode = uf::renderer::getRenderMode("Swapchain", true);
			auto image = renderMode.screenshot(uf::renderer::states::currentBuffer ? 0 : 1);

			std::time_t t = std::time(nullptr);
			const uf::stl::string filename = ::fmt::format("{}/screenshots/{:%Y-%m-%d_%H-%M-%S}.png", uf::io::root, ::fmt::localtime(t));
			image.save(filename);
			UF_MSG_DEBUG("Screenshot saved to {}", filename);
		}
	}
#if 0
	/* Force Rebuild */ {
		TIMER(1, uf::inputs::kbm::states::R ) {
			uf::renderer::states::rebuild = true;
			UF_MSG_DEBUG("Rebuild requested");
		}
	}
#endif
#endif
#if 0
	/* Mark as ready for multithreading */ {
		TIMER(1, uf::inputs::kbm::states::M ) {
			uf::renderer::settings::experimental::dedicatedThread = !uf::renderer::settings::experimental::dedicatedThread;
			UF_MSG_DEBUG("Toggling multithreaded rendering...");
		}
	}
	/* Print World Tree */ {
		TIMER(1, uf::inputs::kbm::states::U ) {
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
#endif

#if UF_USE_OPENAL
	/* check if audio needs to loop */ {
		auto& audio = this->getComponent<uf::Audio>();
		if ( !audio.playing() ) audio.play();
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
		
		ext::al::listener( transform );
	}
#if 0
	/* check if audio needs to loop */ {
		auto& bgm = this->getComponent<uf::Audio>();
		float current = bgm.getTime();
		float end = bgm.getDuration();
		float epsilon = 0.005f;
		if ( (current + epsilon >= end || !bgm.playing()) && !bgm.loops() ) {
			// intro to main transition
			uf::stl::string filename = bgm.getFilename();
			filename = uf::asset::getOriginal(filename);
			if ( filename.find("_intro") != uf::stl::string::npos ) {
				uf::asset::load(uf::string::replace( filename, "_intro", "" ), this->formatHookName("asset:Load.%UID%"));
			// loop
			} else {
				bgm.setTime(0);
				if ( !bgm.playing() ) bgm.play();
			}
		}
	}
#endif
#endif
#if !UF_ENV_DREAMCAST
	/* Regain control if nothing requests it */ {
		pod::payloads::windowMouseCursorVisibility payload;
		payload.mouse.visible = this->globalFindByName("Gui: Menu");
		if ( !payload.mouse.visible ) uf::hooks.call("window:Mouse.Lock");
		uf::hooks.call("window:Mouse.CursorVisibility", payload);
	}
#endif
#if UF_ENTITY_METADATA_USE_JSON
	metadata.deserialize(self, metadataJson);
#else
	if ( 0 <= metadata.shader.time && metadata.shader.time < 4 ) {
		metadata.shader.parameters[metadata.shader.time] = uf::physics::time::current;
	}
#endif
#if UF_USE_OPENGL
	if ( metadata.light.enabled ) {
		auto/*&*/ graph = this->getGraph();
		auto& controller = this->getController();
	//	auto& camera = controller.getComponent<uf::Camera>();
		auto& camera = this->getCamera( controller );
		auto& controllerMetadata = controller.getComponent<uf::Serializer>();
		auto& controllerTransform = controller.getComponent<pod::Transform<>>();
		auto& metadata = this->getComponent<ext::ExtSceneBehavior::Metadata>();
		auto& metadataVxgi = this->getComponent<ext::VoxelizerSceneBehavior::Metadata>();
		auto& metadataJson = this->getComponent<uf::Serializer>();

		struct LightInfo {
			uf::Entity* entity = NULL;
			pod::Vector4f position = {0,0,0,1}; // OpenGL requires a W
			pod::Vector4f color = {0,0,0,1}; // OpenGL requires an alpha
			float distance = 0;
			float power = 0;
			bool global = 0;
		};
		uf::stl::vector<LightInfo> entities;
		for ( auto entity : graph ) {
			if ( entity == this || entity == &controller || !entity->hasComponent<ext::LightBehavior::Metadata>() ) continue;
			auto& metadata = entity->getComponent<ext::LightBehavior::Metadata>();
			if ( metadata.power <= 0 ) continue;
			auto flatten = uf::transform::flatten( entity->getComponent<pod::Transform<>>() );
			LightInfo& info = entities.emplace_back(LightInfo{
				.entity = entity,
				.position = flatten.position,
				.color = metadata.color,
				.distance = uf::vector::magnitude( uf::vector::subtract( flatten.position, controllerTransform.position ) ),
				.power = metadata.power,
				.global = metadata.global,
			});
			info.position.w = 1;
			info.color.w = 1;
		}
		std::sort( entities.begin(), entities.end(), [&]( LightInfo& l, LightInfo& r ){
			if ( l.global && !r.global ) return true;
			if ( !l.global && r.global ) return false;
			return l.distance < r.distance;
		});

		static GLint glMaxLights = 0;
		if ( !glMaxLights ) glGetIntegerv(GL_MAX_LIGHTS, &glMaxLights);
		metadata.light.max = std::min( (uint32_t) glMaxLights, metadata.light.max );

		// add lighting
		{
			uint32_t i = 0;	
			for ( ; i < entities.size() && i < metadata.light.max; ++i ) {
				auto& info = entities[i];
			//	uf::Entity* entity = info.entity;
				GLenum target = GL_LIGHT0+i;
				GL_ERROR_CHECK(glEnable(target));
				GL_ERROR_CHECK(glLightfv(target, GL_AMBIENT, &metadata.light.ambient[0]));
				GL_ERROR_CHECK(glLightfv(target, GL_SPECULAR, &metadata.light.specular[0]));
				GL_ERROR_CHECK(glLightfv(target, GL_DIFFUSE, &info.color[0]));
				GL_ERROR_CHECK(glLightfv(target, GL_POSITION, &info.position[0]));
				GL_ERROR_CHECK(glLightf(target, GL_CONSTANT_ATTENUATION, 0.0f));
				GL_ERROR_CHECK(glLightf(target, GL_LINEAR_ATTENUATION, 0));
				GL_ERROR_CHECK(glLightf(target, GL_QUADRATIC_ATTENUATION, 1.0f / info.power));
			}
			for ( ; i < metadata.light.max; ++i ) GL_ERROR_CHECK(glDisable(GL_LIGHT0+i));
		}
	}
#elif UF_USE_VULKAN
	{
		auto/*&*/ graph = this->getGraph();
		auto& controller = this->getController();
	//	auto& camera = controller.getComponent<uf::Camera>();
		auto& camera = this->getCamera( controller );
		auto& controllerMetadata = controller.getComponent<uf::Serializer>();
		auto& controllerTransform = controller.getComponent<pod::Transform<>>();
		auto& metadata = this->getComponent<ext::ExtSceneBehavior::Metadata>();
		auto& metadataVxgi = this->getComponent<ext::VoxelizerSceneBehavior::Metadata>();
		auto& metadataJson = this->getComponent<uf::Serializer>();

		struct LightInfo {
			uf::Entity* entity = NULL;
			pod::Vector3f position = {0,0,0};
			float range = 0.0f;
			pod::Vector3f color = {0,0,0};
			float intensity = 0.0f;
			float distance = 0;
			float bias = 0;
			int32_t type = 0;
			bool shadows = false;
			bool global = false;
		};
		
		uf::stl::vector<LightInfo> entities; entities.reserve(graph.size() / 2);

		uf::graph::storage.lights.clear(); uf::graph::storage.lights.reserve(metadata.light.max);
		uf::graph::storage.shadow2Ds.clear(); uf::graph::storage.shadow2Ds.reserve(metadata.light.max);
		uf::graph::storage.shadowCubes.clear(); uf::graph::storage.shadowCubes.reserve(metadata.light.max);

		// traverse scene graph
		for ( auto entity : graph ) {
			// ignore this scene, our controller, and anything that isn't actually a light
			if ( entity == this || entity == &controller || !entity->hasComponent<ext::LightBehavior::Metadata>() ) continue;
			auto& metadata = entity->getComponent<ext::LightBehavior::Metadata>();
			// disables shadow mappers that activate when in range
			bool hasRT = entity->hasComponent<uf::renderer::RenderTargetRenderMode>();
			if ( hasRT ) {
				auto& renderMode = entity->getComponent<uf::renderer::RenderTargetRenderMode>();
				if ( metadata.renderer.mode == "in-range" ) {
					renderMode.execute = false;
					renderMode.metadata.limiter.execute = false;
				}
			}
			if ( metadata.power <= 0 ) continue;
			auto flatten = uf::transform::flatten( entity->getComponent<pod::Transform<>>() );
			LightInfo& info = entities.emplace_back(LightInfo{
				.entity = entity,
				.position = flatten.position,
				.range = 0,
				.color = pod::Vector4f{ metadata.color.x, metadata.color.y, metadata.color.z },
				.intensity = metadata.power,
				.distance = uf::vector::magnitude( uf::vector::subtract( flatten.position, controllerTransform.position ) ),
				.bias = metadata.bias,
				.type = metadata.type,
				.shadows = metadata.shadows && hasRT,
				.global = metadata.global,
			});
		}
		// prioritize closer lights; it would be nice to also prioritize lights in view, but because of VXGI it's not really something to do
		std::sort( entities.begin(), entities.end(), [&]( LightInfo& l, LightInfo& r ){
			if ( l.global && !r.global ) return true;
			if ( !l.global && r.global ) return false;
			return l.distance < r.distance;
		});

		int32_t shadowUpdateThreshold = metadata.shadow.update; // how many shadow maps we should update, based on range
		int32_t shadowCount = metadata.shadow.max; // how many shadow maps we should pass, based on range
		if ( shadowCount <= 0 ) shadowCount = std::numeric_limits<int32_t>::max();

		// disable shadows if that light is outside our threshold
		for ( auto& info : entities ) if ( info.shadows && shadowCount-- <= 0 ) info.shadows = false;

		// bind lighting and requested shadow maps
		for ( uint32_t i = 0; i < entities.size() && uf::graph::storage.lights.size() < metadata.light.max; ++i ) {
			auto& info = entities[i];
			uf::Entity* entity = info.entity;

			if ( !info.shadows ) {
				uf::graph::storage.lights.emplace_back(pod::Light{
					.view = uf::matrix::identity(),
					.projection = uf::matrix::identity(),
					.position = info.position,
					.range = info.range,
					.color = info.color,
					.intensity = info.intensity,
					.type = info.type,
					.typeMap = 0,
					.indexMap = -1,
					.depthBias = info.bias,
				});
			} else {
				auto& renderMode = entity->getComponent<uf::renderer::RenderTargetRenderMode>();
				auto& lightCamera = entity->getComponent<uf::Camera>();
				auto& lightMetadata = entity->getComponent<ext::LightBehavior::Metadata>();
				lightMetadata.renderer.rendered = true;
				// activate our shadow mapper if it's range-basedd
				if ( lightMetadata.renderer.mode == "in-range" && shadowUpdateThreshold-- > 0 ) {
					renderMode.execute = true;
					renderMode.metadata.limiter.execute = true;
				}
				constexpr uint32_t MODE_SPLIT = 0;
				constexpr uint32_t MODE_CUBEMAP = 1;
				constexpr uint32_t MODE_SEPARATE_2DS = 2;
				// if point light, and combining is requested
				if ( metadata.shadow.typeMap > MODE_SPLIT && renderMode.renderTarget.views == 6 ) {
					int32_t index = -1;
					// separated texture2Ds
					if ( metadata.shadow.typeMap == MODE_SEPARATE_2DS ) {
						UF_MSG_WARNING("deprecated feature used: separate Texture2Ds for shadow maps");
						index = uf::graph::storage.shadow2Ds.size();
						for ( auto& attachment : renderMode.renderTarget.attachments ) {
							if ( !(attachment.descriptor.usage & VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT) ) continue;
							for ( size_t view = 0; view < renderMode.renderTarget.views; ++view ) {			
								uf::graph::storage.shadow2Ds.emplace_back().aliasAttachment(attachment, view);
							}
							break;
						}
					// cubemapped
					} else if ( metadata.shadow.typeMap == MODE_CUBEMAP ) {
						index = uf::graph::storage.shadowCubes.size();
						for ( auto& attachment : renderMode.renderTarget.attachments ) {
							if ( !(attachment.descriptor.usage & VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT) ) continue;
							uf::graph::storage.shadowCubes.emplace_back().aliasAttachment(attachment);
							break;
						}
					}
					uf::graph::storage.lights.emplace_back(pod::Light{
						.view = lightCamera.getView(0),
						.projection = lightCamera.getProjection(0),
						.position = info.position,
						.range = info.range,
						.color = info.color,
						.intensity = info.intensity,
						.type = info.type,
						.typeMap = metadata.shadow.typeMap,
						.indexMap = index,
						.depthBias = info.bias,
					});
				// any other shadowing light, even point lights, are split by shadow maps
				} else {
					for ( auto& attachment : renderMode.renderTarget.attachments ) {
						if ( !(attachment.descriptor.usage & VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT) ) continue;
						if ( attachment.descriptor.layout == VK_IMAGE_LAYOUT_PRESENT_SRC_KHR ) continue;
						for ( size_t view = 0; view < renderMode.renderTarget.views; ++view ) {
							uf::graph::storage.lights.emplace_back(pod::Light{
								.view = lightCamera.getView(view),
								.projection = lightCamera.getProjection(view),
								.position = info.position,
								.range = info.range,
								.color = info.color,
								.intensity = info.intensity,
								.type = info.type,
								.typeMap = 0,
								.indexMap = uf::graph::storage.shadow2Ds.size(),
								.depthBias = info.bias,
							});
							uf::graph::storage.shadow2Ds.emplace_back().aliasAttachment(attachment, view);
						}
					}
				}
			}
		}

		for ( auto& texture : uf::graph::storage.shadowCubes ) {
			texture.sampler.descriptor.filter.min = uf::renderer::enums::Filter::LINEAR;
			texture.sampler.descriptor.filter.mag = uf::renderer::enums::Filter::LINEAR;
		}
		for ( auto& texture : uf::graph::storage.shadow2Ds ) {
			texture.sampler.descriptor.filter.min = uf::renderer::enums::Filter::LINEAR;
			texture.sampler.descriptor.filter.mag = uf::renderer::enums::Filter::LINEAR;
		}
		
		uf::graph::storage.buffers.light.update( (const void*) uf::graph::storage.lights.data(), uf::graph::storage.lights.size() * sizeof(pod::Light) );
	}
#endif
#if UF_USE_VULKAN
	/* Update lights */ if ( uf::renderer::settings::pipelines::deferred && !uf::renderer::settings::pipelines::vxgi ) {
		auto& deferredRenderMode = uf::renderer::getRenderMode("", true);
		auto& deferredBlitter = deferredRenderMode.getBlitter();
		if ( deferredBlitter.material.hasShader("compute", "deferred") ) {
			ext::ExtSceneBehavior::bindBuffers( *this, "", "compute", "deferred" );
		} else {
			ext::ExtSceneBehavior::bindBuffers( *this, "", "fragment", "deferred" );
		}
	}
#endif

	/* Update post processing */ {
		auto& renderMode = uf::renderer::getRenderMode("", true);
		auto& blitter = renderMode.getBlitter();
		if ( blitter.material.hasShader("fragment") ) {
			auto& shader = blitter.material.getShader("fragment");

			struct {
				float curTime = 0;
				float gamma = 1.0;
				float exposure = 1.0;
				uint32_t padding = 0;
			} uniforms = {
				.curTime = uf::time::current,
				.gamma = metadata.light.gamma,
				.exposure = metadata.light.exposure
			};


			shader.updateBuffer( (const void*) &uniforms, sizeof(uniforms), shader.getUniformBuffer("UBO") );
		}
	}
}
void ext::ExtSceneBehavior::render( uf::Object& self ) {}
void ext::ExtSceneBehavior::destroy( uf::Object& self ) {
	if ( this->hasComponent<pod::SceneTextures>() ) {
		auto& sceneTextures = this->getComponent<pod::SceneTextures>();
		for ( auto& t : sceneTextures.voxels.id ) t.destroy();
		sceneTextures.voxels.id.clear();

		for ( auto& t : sceneTextures.voxels.normal ) t.destroy();
		sceneTextures.voxels.normal.clear();

		for ( auto& t : sceneTextures.voxels.uv ) t.destroy();
		sceneTextures.voxels.uv.clear();

		for ( auto& t : sceneTextures.voxels.radiance ) t.destroy();
		sceneTextures.voxels.radiance.clear();

		for ( auto& t : sceneTextures.voxels.depth ) t.destroy();
		sceneTextures.voxels.depth.clear();

		sceneTextures.noise.destroy();
		sceneTextures.skybox.destroy();
	}
}
void ext::ExtSceneBehavior::Metadata::serialize( uf::Object& self, uf::Serializer& serializer ) {
	serializer["light"]["should"] = /*this->*/light.enabled;

	serializer["light"]["ambient"] = uf::vector::encode( /*this->*/light.ambient );
	serializer["light"]["exposure"] = /*this->*/light.exposure;
	serializer["light"]["gamma"] = /*this->*/light.gamma;
	serializer["light"]["useLightmaps"] = /*this->*/light.useLightmaps;

	serializer["light"]["fog"]["color"] = uf::vector::encode( /*this->*/fog.color );
	serializer["light"]["fog"]["step scale"] = /*this->*/fog.stepScale;
	serializer["light"]["fog"]["absorbtion"] = /*this->*/fog.absorbtion;
	serializer["light"]["fog"]["range"] = uf::vector::encode( /*this->*/fog.range );
	serializer["light"]["fog"]["density"]["offset"] = uf::vector::encode( /*this->*/fog.density.offset );
	serializer["light"]["fog"]["density"]["timescale"] = /*this->*/fog.density.timescale;
	serializer["light"]["fog"]["density"]["threshold"] = /*this->*/fog.density.threshold;
	serializer["light"]["fog"]["density"]["multiplier"] = /*this->*/fog.density.multiplier;
	serializer["light"]["fog"]["density"]["scale"] = /*this->*/fog.density.scale;
	serializer["sky"]["box"]["filename"] = /*this->*/sky.box.filename;

	serializer["system"]["renderer"]["shader"]["mode"] = /*this->*/shader.mode;
	serializer["system"]["renderer"]["shader"]["scalar"] = /*this->*/shader.scalar;
	serializer["system"]["renderer"]["shader"]["parameters"] = uf::vector::encode( /*this->*/shader.parameters );
}
void ext::ExtSceneBehavior::Metadata::deserialize( uf::Object& self, uf::Serializer& serializer ) {
	// merge light settings with global settings
	{
		const auto& globalSettings = ext::config["engine"]["scenes"]["lights"];
		ext::json::forEach( globalSettings, [&]( const uf::stl::string& key, const ext::json::Value& value ){
			if ( !ext::json::isNull( serializer["light"][key] ) ) return;
			serializer["light"][key] = value;
		} );
	}
	// merge bloom settings with global settings
	{
		const auto& globalSettings = ext::config["engine"]["scenes"]["lights"]["bloom"];
		ext::json::forEach( globalSettings, [&]( const uf::stl::string& key, const ext::json::Value& value ){
			if ( !ext::json::isNull( serializer["light"]["bloom"][key] ) ) return;
			serializer["light"]["bloom"][key] = value;
		} );
	}
	// merge shadows settings with global settings
	{
		const auto& globalSettings = ext::config["engine"]["scenes"]["lights"]["shadows"];
		ext::json::forEach( globalSettings, [&]( const uf::stl::string& key, const ext::json::Value& value ){
			if ( !ext::json::isNull( serializer["light"]["shadows"][key] ) ) return;
			serializer["light"]["shadows"][key] = value;
		} );
	}
	// merge fog settings with global settings
	{
		const auto& globalSettings = ext::config["engine"]["scenes"]["lights"]["fog"];
		ext::json::forEach( globalSettings, [&]( const uf::stl::string& key, const ext::json::Value& value ){
			if ( !ext::json::isNull( serializer["light"]["fog"][key] ) ) return;
			serializer["light"]["fog"][key] = value;
		} );
	}

	/*this->*/max.textures2D   = ext::config["engine"]["scenes"]["textures"]["max"]["2D"].as(/*this->*/max.textures2D);
	/*this->*/max.texturesCube = ext::config["engine"]["scenes"]["textures"]["max"]["cube"].as(/*this->*/max.texturesCube);
	/*this->*/max.textures3D   = ext::config["engine"]["scenes"]["textures"]["max"]["3D"].as(/*this->*/max.textures3D);

	/*this->*/shadow.enabled = serializer["light"]["shadows"]["enabled"].as(/*this->*/shadow.enabled);
	/*this->*/shadow.samples = serializer["light"]["shadows"]["samples"].as(/*this->*/shadow.samples);
	/*this->*/shadow.max = serializer["light"]["shadows"]["max"].as(/*this->*/shadow.max);
	/*this->*/shadow.update = serializer["light"]["shadows"]["update"].as(/*this->*/shadow.update);
	/*this->*/shadow.typeMap = serializer["light"]["shadows"]["map type"].as(/*this->*/shadow.typeMap);

	/*this->*/light.enabled = serializer["light"]["enabled"].as(/*this->*/light.enabled) && serializer["light"]["should"].as(/*this->*/light.enabled);
	/*this->*/light.max = serializer["light"]["max"].as(/*this->*/light.max);
	/*this->*/light.ambient = uf::vector::decode( serializer["light"]["ambient"], /*this->*/light.ambient);

//	if ( uf::renderer::settings::pipelines::fsr ) serializer["light"]["exposure"] = 1;
	/*this->*/light.exposure = serializer["light"]["exposure"].as(/*this->*/light.exposure);
	/*this->*/light.gamma = serializer["light"]["gamma"].as(/*this->*/light.gamma);
	/*this->*/light.useLightmaps = serializer["light"]["useLightmaps"].as(/*this->*/light.useLightmaps);

	/*this->*/bloom.threshold = serializer["light"]["bloom"]["threshold"].as(/*this->*/bloom.threshold);
	/*this->*/bloom.scale = serializer["light"]["bloom"]["scale"].as(/*this->*/bloom.scale);
	/*this->*/bloom.strength = serializer["light"]["bloom"]["strength"].as(/*this->*/bloom.strength);
	/*this->*/bloom.sigma = serializer["light"]["bloom"]["sigma"].as(/*this->*/bloom.sigma);
	/*this->*/bloom.samples = serializer["light"]["bloom"]["samples"].as(/*this->*/bloom.samples);

	/*this->*/fog.color = uf::vector::decode( serializer["light"]["fog"]["color"], /*this->*/fog.color );
	/*this->*/fog.stepScale = serializer["light"]["fog"]["step scale"].as( /*this->*/fog.stepScale );
	/*this->*/fog.absorbtion = serializer["light"]["fog"]["absorbtion"].as( /*this->*/fog.absorbtion );
	/*this->*/fog.range = uf::vector::decode( serializer["light"]["fog"]["range"], /*this->*/fog.range );
	/*this->*/fog.density.offset = uf::vector::decode( serializer["light"]["fog"]["density"]["offset"], /*this->*/fog.density.offset );
	/*this->*/fog.density.timescale = serializer["light"]["fog"]["density"]["timescale"].as(/*this->*/fog.density.timescale);
	/*this->*/fog.density.threshold = serializer["light"]["fog"]["density"]["threshold"].as(/*this->*/fog.density.threshold);
	/*this->*/fog.density.multiplier = serializer["light"]["fog"]["density"]["multiplier"].as(/*this->*/fog.density.multiplier);
	/*this->*/fog.density.scale = serializer["light"]["fog"]["density"]["scale"].as(/*this->*/fog.density.scale);
	
	/*this->*/sky.box.filename = serializer["sky"]["box"]["filename"].as(/*this->*/sky.box.filename);

	/*this->*/shader.mode = serializer["system"]["renderer"]["shader"]["mode"].as(/*this->*/shader.mode);
	/*this->*/shader.scalar = serializer["system"]["renderer"]["shader"]["scalar"].as(/*this->*/shader.scalar);
	/*this->*/shader.parameters = uf::vector::decode( serializer["system"]["renderer"]["shader"]["parameters"], /*this->*/shader.parameters );
	/*this->*/shader.frameAccumulateLimit = serializer["system"]["renderer"]["shader"]["frame accumulate limit"].as(/*this->*/shader.frameAccumulateLimit);

#if UF_AUDIO_MAPPED_VOLUMES
	ext::json::forEach( serializer["volumes"], []( const uf::stl::string& key, ext::json::Value& value ){
		float volume; volume = value.as(volume);
		uf::audio::volumes[key] = volume;
	});
#else
	if ( ext::json::isObject( serializer["volumes"] ) ) {
		uf::audio::volumes::bgm = serializer["volumes"]["bgm"].as(uf::audio::volumes::bgm);
		uf::audio::volumes::sfx = serializer["volumes"]["sfx"].as(uf::audio::volumes::sfx);
		uf::audio::volumes::voice = serializer["volumes"]["voice"].as(uf::audio::volumes::voice);
	}
#endif

	ext::json::forEach( serializer["system"]["renderer"]["shader"]["parameters"], [&]( uint32_t i, const ext::json::Value& value ){
		if ( value.as<uf::stl::string>() == "time" ) /*this->*/shader.time = i;
	});

	if ( 0 <= /*this->*/shader.time && /*this->*/shader.time < 4 ) {
		/*this->*/shader.parameters[/*this->*/shader.time] = uf::physics::time::current;
	}
#if UF_USE_OPENGL_FIXED_FUNCTION
	uf::renderer::states::rebuild = true;
	if ( light.enabled ) {
		GL_ERROR_CHECK(glEnable(GL_LIGHTING));
	} else {
		GL_ERROR_CHECK(glDisable(GL_LIGHTING));
	}
#endif
#if UF_USE_VULKAN
	if ( uf::renderer::settings::pipelines::bloom ) {
		auto& renderMode = uf::renderer::getRenderMode("", true);
		auto& blitter = renderMode.getBlitter();
		auto& shader = blitter.material.getShader("compute", "bloom");

		struct UniformDescriptor {
			float scale;
			float strength;
			float threshold;
			float sigma;

			float gamma;
			float exposure;
			uint32_t samples;
			uint32_t padding;
		};

		UniformDescriptor uniforms = {
			.scale = bloom.scale,
			.strength = bloom.strength,
			.threshold = bloom.threshold,
			.sigma = bloom.sigma,

			.gamma = light.gamma,
			.exposure = light.exposure,
			.samples = bloom.samples,
		};

		shader.updateBuffer( (const void*) &uniforms, sizeof(uniforms), shader.getUniformBuffer("UBO") );
	}
#endif
}

void ext::ExtSceneBehavior::bindBuffers( uf::Object& self, const uf::stl::string& renderModeName, const uf::stl::string& shaderType, const uf::stl::string& shaderPipeline ) {
	auto& renderMode = uf::renderer::getRenderMode(renderModeName, true);
	bindBuffers( self, renderMode.getBlitter(), shaderType, shaderPipeline );
}
void ext::ExtSceneBehavior::bindBuffers( uf::Object& self, uf::renderer::Graphic& graphic, const uf::stl::string& shaderType, const uf::stl::string& shaderPipeline ) {
	auto/*&*/ graph = this->getGraph();
	auto& controller = this->getController();
//	auto& camera = controller.getComponent<uf::Camera>();
	auto& camera = this->getCamera( controller );
	auto& controllerMetadata = controller.getComponent<uf::Serializer>();
	auto& controllerTransform = controller.getComponent<pod::Transform<>>();
	auto& metadata = this->getComponent<ext::ExtSceneBehavior::Metadata>();
	auto& metadataVxgi = this->getComponent<ext::VoxelizerSceneBehavior::Metadata>();
	auto& metadataRt = this->getComponent<ext::RayTraceSceneBehavior::Metadata>();
	auto& metadataJson = this->getComponent<uf::Serializer>();

	if ( !graphic.initialized ) return;

#if UF_USE_VULKAN
	struct UniformDescriptor {
		struct Matrices {
			alignas(16) pod::Matrix4f view;
			alignas(16) pod::Matrix4f projection;

			alignas(16) pod::Matrix4f model;
			alignas(16) pod::Matrix4f previous;

			alignas(16) pod::Matrix4f iView;
			alignas(16) pod::Matrix4f iProjection;
		//	alignas(16) pod::Matrix4f iProjectionView;
			
			alignas(16) pod::Vector4f eyePos;
		} matrices[2];

		struct Settings {
			struct Lengths {
				alignas(4) uint32_t lights = 0;
				alignas(4) uint32_t materials = 0;
				alignas(4) uint32_t textures = 0;
				alignas(4) uint32_t drawCommands = 0;
			} lengths;

			struct Mode {
				alignas(16) pod::Vector4f parameters;

				alignas(4) uint32_t mode;
				alignas(4) uint32_t scalar;
				
				alignas(4) uint32_t msaa;
				alignas(4) uint32_t frameAccumulate;
			} mode;

			struct Lighting {
				pod::Vector3f ambient;
				alignas(4) uint32_t indexSkybox;
				
				alignas(4) uint32_t maxShadows;
				alignas(4) uint32_t shadowSamples;
				alignas(4) uint32_t useLightmaps;
			} lighting;

			struct Fog {
				pod::Vector3f color;
				alignas(4) float stepScale;

				pod::Vector3f offset;
				alignas(4) float densityScale;

				alignas(8) pod::Vector2f range;
				alignas(4) float densityThreshold;
				alignas(4) float densityMultiplier;
				
				alignas(4) float absorbtion;
				alignas(4) float padding1;
				alignas(4) float padding2;
				alignas(4) float padding3;
			} fog;

			struct Bloom {
				alignas(4) float exposure;
				alignas(4) float gamma;
				alignas(4) float threshold;
				alignas(4) uint32_t padding2;
			} bloom;

			struct VXGI {
				alignas(16) pod::Matrix4f matrix;
				alignas(4)  float cascadePower;
				alignas(4)  float granularity;
				alignas(4)  float voxelizeScale;
				alignas(4)  float occlusionFalloff;
				
				alignas(4)  float traceStartOffsetFactor;
				alignas(4)  uint32_t shadows;
				alignas(4)  uint32_t padding2;
				alignas(4)  uint32_t padding3;
			} vxgi;

			struct RT {
				alignas(8) pod::Vector2f defaultRayBounds;
				alignas(4) float alphaTestOffset;
				alignas(4) float padding1;

				alignas(4) uint samples;
				alignas(4) uint paths;
				alignas(4) uint frameAccumulationMinimum;
				alignas(4) uint padding2;
			} rt;
		} settings;
	};

	// struct that contains our skybox cubemap, noise texture, and VXGI voxels
	auto& sceneTextures = this->getComponent<pod::SceneTextures>();
	uf::stl::vector<uf::renderer::Texture> textures2D;
	textures2D.reserve( metadata.max.textures2D );

	uf::stl::vector<uf::renderer::Texture> textures3D;
	textures3D.reserve( metadata.max.textures3D );
	
	uf::stl::vector<uf::renderer::Texture> texturesCube;
	texturesCube.reserve( metadata.max.texturesCube );

	// bind scene textures
	for ( auto& key : uf::graph::storage.texture2Ds.keys ) textures2D.emplace_back().aliasTexture( uf::graph::storage.texture2Ds.map[key] );
	
	// bind shadow maps
	for ( auto& texture : uf::graph::storage.shadow2Ds ) textures2D.emplace_back().aliasTexture(texture);
	for ( auto& texture : uf::graph::storage.shadowCubes ) texturesCube.emplace_back().aliasTexture(texture);

	// bind skybox
	size_t indexSkybox = texturesCube.size();
	texturesCube.emplace_back().aliasTexture(sceneTextures.skybox);
	// bind noise texture
	size_t indexNoise = textures3D.size();
	textures3D.emplace_back().aliasTexture(sceneTextures.noise);

	// attach VXGI voxels
	if ( uf::renderer::settings::pipelines::vxgi ) {
		for ( auto& t : sceneTextures.voxels.id ) textures3D.emplace_back().aliasTexture(t);
		for ( auto& t : sceneTextures.voxels.normal ) textures3D.emplace_back().aliasTexture(t);
		for ( auto& t : sceneTextures.voxels.uv ) textures3D.emplace_back().aliasTexture(t);
		for ( auto& t : sceneTextures.voxels.radiance ) textures3D.emplace_back().aliasTexture(t);
		for ( auto& t : sceneTextures.voxels.depth ) textures3D.emplace_back().aliasTexture(t);
	}
	// bind textures
	while ( textures2D.size() < metadata.max.textures2D ) textures2D.emplace_back().aliasTexture(uf::renderer::Texture2D::empty);
	while ( texturesCube.size() < metadata.max.texturesCube ) texturesCube.emplace_back().aliasTexture(uf::renderer::TextureCube::empty);
	while ( textures3D.size() < metadata.max.textures3D ) textures3D.emplace_back().aliasTexture(uf::renderer::Texture3D::empty);

	static uf::stl::unordered_map<uf::stl::string, UniformDescriptor> previousUniformsMap;
	auto& previousUniforms = previousUniformsMap[shaderType+":"+shaderPipeline+":"+std::to_string(&graphic)];

	// update uniform information
	// hopefully write combining kicks in
	UniformDescriptor uniforms; {
		for ( auto i = 0; i < 2; ++i ) {
		#if UF_USE_FFX_FSR
			auto projection = ext::fsr::getJitterMatrix() * camera.getProjection(i);
		#else
			auto projection = camera.getProjection(i);
		#endif
			uniforms.matrices[i] = UniformDescriptor::Matrices{
				.view = camera.getView(i),
				.projection = projection,
				
				.model = projection * camera.getView(i),
				.previous = previousUniforms.matrices[i].model,

				.iView = uf::matrix::inverse( camera.getView(i) ),
				.iProjection = uf::matrix::inverse( projection ),
			//	.iProjectionView = uf::matrix::inverse( projection * camera.getView(i) ),

				.eyePos = camera.getEye( i ),
			};
		}
		uniforms.settings.lengths = UniformDescriptor::Settings::Lengths{
			.lights = MIN( uf::graph::storage.lights.size(), metadata.light.max ),
			.materials = MIN( uf::graph::storage.materials.keys.size(), metadata.max.textures2D ),
			.textures = MIN( uf::graph::storage.textures.keys.size(), metadata.max.textures2D ),
			.drawCommands = MIN( 0, metadata.max.textures2D ),
		};
		uniforms.settings.mode = UniformDescriptor::Settings::Mode{
			.parameters = metadata.shader.parameters,
			
			.mode = metadata.shader.mode,
			.scalar = metadata.shader.scalar,
			
			.msaa = ext::vulkan::settings::msaa,
			.frameAccumulate = metadata.shader.frameAccumulate,
		};
		uniforms.settings.lighting = UniformDescriptor::Settings::Lighting{
			.ambient = metadata.light.ambient,
			.indexSkybox = indexSkybox,

			.maxShadows = metadata.shadow.max,
			.shadowSamples = std::min( 0, metadata.shadow.samples ),
			.useLightmaps = metadata.light.useLightmaps,
		};
		uniforms.settings.fog = UniformDescriptor::Settings::Fog{
			.color = metadata.fog.color,
			.stepScale = metadata.fog.stepScale,

			.offset = metadata.fog.density.offset * (float) metadata.fog.density.timescale * (float) uf::physics::time::current,
			.densityScale = metadata.fog.density.scale,

			.range = metadata.fog.range,
			.densityThreshold = metadata.fog.density.threshold,
			.densityMultiplier = metadata.fog.density.multiplier,

			.absorbtion = metadata.fog.absorbtion,
		};
		uniforms.settings.bloom = UniformDescriptor::Settings::Bloom{
			.exposure = metadata.light.exposure,
			.gamma = metadata.light.gamma,
			.threshold = metadata.bloom.threshold,
		};
		uniforms.settings.vxgi = UniformDescriptor::Settings::VXGI{
			.matrix = metadataVxgi.extents.matrix,
			.cascadePower = metadataVxgi.cascadePower,
			.granularity = metadataVxgi.granularity,
			.voxelizeScale = 1.0f / (metadataVxgi.voxelizeScale * std::max<uint32_t>( metadataVxgi.voxelSize.x, std::max<uint32_t>(metadataVxgi.voxelSize.y, metadataVxgi.voxelSize.z))),
			.occlusionFalloff = metadataVxgi.occlusionFalloff,
			
			.traceStartOffsetFactor = metadataVxgi.traceStartOffsetFactor,
			.shadows = metadataVxgi.shadows,
		};
		uniforms.settings.rt = UniformDescriptor::Settings::RT{
			.defaultRayBounds = metadataRt.settings.defaultRayBounds, // { 0.001, 4096.0 },
			.alphaTestOffset = metadataRt.settings.alphaTestOffset, //0.001,

			.samples = metadataRt.settings.samples, // 1,
			.paths = metadataRt.settings.paths, // 1,
			.frameAccumulationMinimum = metadataRt.settings.frameAccumulationMinimum, // 0,
		};
	}

	uf::stl::vector<VkImage> previousTextures;
	for ( auto& texture : graphic.material.textures ) previousTextures.emplace_back(texture.image);
	graphic.material.textures.clear();

	// attach our textures to the graphic
	for ( auto& t : textures2D ) graphic.material.textures.emplace_back().aliasTexture(t);
	for ( auto& t : texturesCube ) graphic.material.textures.emplace_back().aliasTexture(t);
	for ( auto& t : textures3D ) graphic.material.textures.emplace_back().aliasTexture(t);

	// trigger an update when we have differing bound texture sizes
	bool shouldUpdate = metadata.shader.invalidated || graphic.material.textures.size() != previousTextures.size();
	for ( uint32_t i = 0; !shouldUpdate && i < previousTextures.size() && i < graphic.material.textures.size(); ++i ) {
		if ( previousTextures[i] != graphic.material.textures[i].image ) shouldUpdate = true;
	}
	if ( shouldUpdate ) {
		graphic.updatePipelines();
		metadata.shader.invalidated = false;
	}

	auto& shader = graphic.material.getShader(shaderType, shaderPipeline);
	shader.updateBuffer( (const void*) &uniforms, sizeof(uniforms), shader.getUniformBuffer("UBO") );

	bool shouldUpdate2 = !uf::matrix::equals( uniforms.matrices[0].view, previousUniforms.matrices[0].view, 0.0001f );
	if ( shouldUpdate || shouldUpdate2 ) {
		metadata.shader.frameAccumulateReset = true;
		uf::renderer::states::frameAccumulateReset = true;
		previousUniforms = uniforms;
	}
#endif
}
#undef this