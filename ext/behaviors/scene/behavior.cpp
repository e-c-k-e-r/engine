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

#include <uf/ext/ext.h>

#include "../light/behavior.h"
#include "../../ext.h"
#include "../../gui/gui.h"

UF_BEHAVIOR_REGISTER_CPP(ext::ExtSceneBehavior)
#define this ((uf::Scene*) &self)
void ext::ExtSceneBehavior::initialize( uf::Object& self ) {
	uf::Asset& assetLoader = this->getComponent<uf::Asset>();
	uf::Serializer& metadataJson = this->getComponent<uf::Serializer>();

	this->addHook( "system:Quit.%UID%", [&](ext::json::Value& json){
		std::cout << json << std::endl;
		ext::ready = false;
	});

	this->addHook( "world:Music.LoadPrevious.%UID%", [&](ext::json::Value& json){

		if ( metadataJson["previous bgm"]["filename"] == "" ) return;

		std::string filename = metadataJson["previous bgm"]["filename"].as<std::string>();
		float timestamp = metadataJson["previous bgm"]["timestamp"].as<float>();

		uf::Audio& audio = this->getComponent<uf::Audio>();
		if ( audio.playing() ) {
			metadataJson["previous bgm"]["filename"] = audio.getFilename();
			metadataJson["previous bgm"]["timestamp"] = audio.getTime();
			audio.stop();
		}
		audio.load(filename);
		audio.setVolume(metadataJson["volumes"]["bgm"].as<float>());
		audio.setTime(timestamp);
		audio.play();
	});
	this->addHook( "asset:Load.%UID%", [&](ext::json::Value& json){
		std::string filename = json["filename"].as<std::string>();
		std::string category = json["category"].as<std::string>();
		if ( category != "" && category != "audio" ) return;
		if ( category == "" && uf::io::extension(filename) != "ogg" ) return;
		const uf::Audio* audioPointer = NULL;
		if ( !assetLoader.has<uf::Audio>(filename) ) return;
		audioPointer = &assetLoader.get<uf::Audio>(filename);
		if ( !audioPointer ) return;

		uf::Audio& audio = this->getComponent<uf::Audio>();
		if ( audio.playing() ) audio.stop();

		audio.load(filename);
		audio.setVolume(metadataJson["volumes"]["bgm"].as<float>());
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
		std::string config = metadataJson["menus"]["pause"].is<std::string>() ? metadataJson["menus"]["pause"].as<std::string>() : "/scenes/worldscape/gui/pause/menu.json";
		uf::Object& gui = manager->loadChild(config, false);
		payload["uid"] = gui.getUid();

		uf::Serializer& metadataJson = gui.getComponent<uf::Serializer>();
		metadataJson["menu"] = json["menu"];
		
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
		json["mode"] = json["mode"].as<size_t>() | metadataJson["system"]["renderer"]["shader"]["mode"].as<size_t>();
		metadataJson["system"]["renderer"]["shader"]["mode"] = json["mode"];
		metadataJson["system"]["renderer"]["shader"]["parameters"] = json["parameters"];
	});
	/* store viewport size */	
	this->addHook( "window:Resized", [&](ext::json::Value& json){
		pod::Vector2ui size = uf::vector::decode( json["window"]["size"], pod::Vector2i{} ); {
		//	size.x = json["window"]["size"]["x"].as<size_t>();
		//	size.y = json["window"]["size"]["y"].as<size_t>();
		}

		metadataJson["system"]["window"] = json["system"]["window"];
		ext::gui::size.current = size;
	});
	// lock control
	{
		uf::Serializer payload;
		payload["state"] = false;
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
			uf::io::root+"/textures/skybox/front.png",
			uf::io::root+"/textures/skybox/back.png",
			uf::io::root+"/textures/skybox/up.png",
			uf::io::root+"/textures/skybox/down.png",
			uf::io::root+"/textures/skybox/right.png",
			uf::io::root+"/textures/skybox/left.png",
		};
		uf::Image::container_t pixels;
		std::vector<uf::Image> images(filenames.size());

		pod::Vector2ui size = {0,0};
		auto& texture = sceneTextures.skybox; //this->getComponent<uf::renderer::TextureCube>();
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
	#endif

	auto& metadata = this->getComponent<ext::ExtSceneBehavior::Metadata>();
	metadata.serialize = [&]() {
		metadataJson["light"]["should"] = metadata.light.enabled;

		metadataJson["light"]["ambient"] = uf::vector::encode( metadata.light.ambient );
		metadataJson["light"]["specular"] = uf::vector::encode( metadata.light.specular );
		metadataJson["light"]["exposure"] = metadata.light.exposure;
		metadataJson["light"]["gamma"] = metadata.light.gamma;

		metadataJson["light"]["fog"]["color"] = uf::vector::encode( metadata.fog.color );
		metadataJson["light"]["fog"]["step scale"] = metadata.fog.stepScale;
		metadataJson["light"]["fog"]["absorbtion"] = metadata.fog.absorbtion;
		metadataJson["light"]["fog"]["range"] = uf::vector::encode( metadata.fog.range );
		metadataJson["light"]["fog"]["density"]["offset"] = uf::vector::encode( metadata.fog.density.offset );
		metadataJson["light"]["fog"]["density"]["timescale"] = metadata.fog.density.timescale;
		metadataJson["light"]["fog"]["density"]["threshold"] = metadata.fog.density.threshold;
		metadataJson["light"]["fog"]["density"]["multiplier"] = metadata.fog.density.multiplier;
		metadataJson["light"]["fog"]["density"]["scale"] = metadata.fog.density.scale;
	};
	metadata.deserialize = [&](){
		metadata.max.textures = ext::config["engine"]["scenes"]["textures"]["max"].as<size_t>();
		metadata.max.lights = ext::config["engine"]["scenes"]["lights"]["max"].as<size_t>();

		metadata.light.enabled = ext::config["engine"]["scenes"]["lights"]["enabled"].as<bool>() && metadataJson["light"]["should"].as<bool>();
		metadata.light.shadowSamples = ext::config["engine"]["scenes"]["lights"]["shadow samples"].as<size_t>();
		metadata.light.shadowThreshold = ext::config["engine"]["scenes"]["lights"]["shadow threshold"].as<size_t>();
		metadata.light.updateThreshold = ext::config["engine"]["scenes"]["lights"]["update threshold"].as<size_t>();
	
		metadata.light.ambient = uf::vector::decode( metadataJson["light"]["ambient"], pod::Vector4f{ 1, 1, 1, 1 } );
		metadata.light.specular = uf::vector::decode( metadataJson["light"]["specular"], pod::Vector4f{ 1, 1, 1, 1 } );
		metadata.light.exposure = metadataJson["light"]["exposure"].as<float>(1.0f);
		metadata.light.gamma = metadataJson["light"]["gamma"].as<float>(2.2f);

		metadata.fog.color = uf::vector::decode( metadataJson["light"]["fog"]["color"], pod::Vector3f{ 1, 1, 1 } );
		metadata.fog.stepScale = metadataJson["light"]["fog"]["step scale"].as<float>();
		metadata.fog.absorbtion = metadataJson["light"]["fog"]["absorbtion"].as<float>();
		metadata.fog.range = uf::vector::decode( metadataJson["light"]["fog"]["range"], pod::Vector2f{ 0, 0 } );
		metadata.fog.density.offset = uf::vector::decode( metadataJson["light"]["fog"]["density"]["offset"], pod::Vector4f{ 0, 0, 0, 0 } );
		metadata.fog.density.timescale = metadataJson["light"]["fog"]["density"]["timescale"].as<float>();
		metadata.fog.density.threshold = metadataJson["light"]["fog"]["density"]["threshold"].as<float>();
		metadata.fog.density.multiplier = metadataJson["light"]["fog"]["density"]["multiplier"].as<float>();
		metadata.fog.density.scale = metadataJson["light"]["fog"]["density"]["scale"].as<float>();

	#if UF_USE_OPENGL_FIXED_FUNCTION
		uf::renderer::states::rebuild = true;
	#endif
	};
	this->addHook( "object:UpdateMetadata.%UID%", metadata.deserialize);
	this->addHook( "object:Reload.%UID%", metadata.deserialize);
	metadata.deserialize();
}
void ext::ExtSceneBehavior::tick( uf::Object& self ) {
	auto& metadata = this->getComponent<ext::ExtSceneBehavior::Metadata>();
	auto& metadataJson = this->getComponent<uf::Serializer>();
#if 0
	uf::hooks.call("game:Frame.Start");

	uf::Serializer& metadata = this->getComponent<uf::Serializer>();
	uf::Asset& assetLoader = this->getComponent<uf::Asset>();
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
#endif
#if UF_USE_OPENAL
	auto& assetLoader = this->getComponent<uf::Asset>();
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
#endif
#if !UF_ENV_DREAMCAST
	/* Regain control if nothing requests it */ {
		if ( !this->globalFindByName("Gui: Menu") ) {
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
#endif
#if UF_ENTITY_METADATA_USE_JSON
	metadata.deserialize();
#else
	if ( !metadata.max.textures ) metadata.max.textures = ext::config["engine"]["scenes"]["textures"]["max"].as<size_t>();
	if ( !metadata.max.lights ) metadata.max.lights = ext::config["engine"]["scenes"]["lights"]["max"].as<size_t>();

	if ( !metadata.light.enabled ) metadata.light.enabled = ext::config["engine"]["scenes"]["lights"]["enabled"].as<bool>() && metadataJson["light"]["should"].as<bool>();
	if ( !metadata.light.shadowSamples ) metadata.light.shadowSamples = ext::config["engine"]["scenes"]["lights"]["shadow samples"].as<size_t>();
	if ( !metadata.light.shadowThreshold ) metadata.light.shadowThreshold = ext::config["engine"]["scenes"]["lights"]["shadow threshold"].as<size_t>();
	if ( !metadata.light.updateThreshold ) metadata.light.updateThreshold = ext::config["engine"]["scenes"]["lights"]["update threshold"].as<size_t>();
	if ( !metadata.light.exposure ) metadata.light.exposure = metadataJson["light"]["exposure"].as<float>(1.0f);
	if ( !metadata.light.gamma ) metadata.light.gamma = metadataJson["light"]["gamma"].as<float>(2.2f);
#endif
	/* Update lights */ if ( uf::renderer::settings::experimental::deferredMode != "vxgi" ) {
		ext::ExtSceneBehavior::bindBuffers( *this );
	}

#if UF_ENTITY_METADATA_USE_JSON
	metadata.deserialize();
#endif
}
void ext::ExtSceneBehavior::render( uf::Object& self ) {}
void ext::ExtSceneBehavior::destroy( uf::Object& self ) {}

void ext::ExtSceneBehavior::bindBuffers( uf::Object& self, const std::string& renderModeName, bool isCompute ) {
	auto& controller = this->getController();
	auto& camera = controller.getComponent<uf::Camera>();
	auto& controllerMetadata = controller.getComponent<uf::Serializer>();
	auto& controllerTransform = controller.getComponent<pod::Transform<>>();
	auto& metadata = this->getComponent<ext::ExtSceneBehavior::Metadata>();
	auto& metadataJson = this->getComponent<uf::Serializer>();

	auto& renderMode = uf::renderer::getRenderMode(renderModeName, true);
	std::vector<uf::Graphic*> blitters = renderMode.getBlitters();
	
#if UF_USE_OPENGL
	struct LightInfo {
		uf::Entity* entity = NULL;
		pod::Vector4f position = {0,0,0,1};
		pod::Vector4f color = {0,0,0,1};
		float distance = 0;
		float power = 0;
	};
	std::vector<LightInfo> entities;
	auto graph = uf::scene::generateGraph();
	for ( auto entity : graph ) {
		if ( entity == &controller || entity == this ) continue;
	//	if ( entity->getName() != "Light" && !entity->hasComponent<ext::LightBehavior::Metadata>() ) continue;
		if ( !entity->hasComponent<ext::LightBehavior::Metadata>() ) continue;
	//	if ( !entity->hasBehavior(pod::Behavior{.type = ext::LightBehavior::type}) ) continue;
		auto& metadata = entity->getComponent<ext::LightBehavior::Metadata>();
		if ( metadata.power <= 0 ) continue;
		LightInfo& info = entities.emplace_back();
		auto& transform = entity->getComponent<pod::Transform<>>();
		auto flatten = uf::transform::flatten( transform );
		info.entity = entity;
		info.position = flatten.position;
		info.position.w = 1;
		info.color = metadata.color;
		info.color.w = 1;
		info.distance = uf::vector::magnitude( uf::vector::subtract( flatten.position, controllerTransform.position ) );
		info.power = metadata.power;
	}
	std::sort( entities.begin(), entities.end(), [&]( LightInfo& l, LightInfo& r ){
		return l.distance < r.distance;
	});

	static GLint glMaxLights = 0;
	if ( !glMaxLights ) glGetIntegerv(GL_MAX_LIGHTS, &glMaxLights);
	metadata.max.lights = std::min( (size_t) glMaxLights, metadata.max.lights );

	// add lighting
	{
		size_t i = 0;	
		for ( ; i < entities.size() && i < metadata.max.lights; ++i ) {
			auto& info = entities[i];
			uf::Entity* entity = info.entity;
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
		for ( ; i < metadata.max.lights; ++i ) GL_ERROR_CHECK(glDisable(GL_LIGHT0+i));
	}
#elif UF_USE_VULKAN
	size_t maxTextures = metadata.max.textures;
	struct UniformDescriptor {
		struct Matrices {
			alignas(16) pod::Matrix4f view[2];
			alignas(16) pod::Matrix4f projection[2];
			alignas(16) pod::Matrix4f iView[2];
			alignas(16) pod::Matrix4f iProjection[2];
			alignas(16) pod::Matrix4f iProjectionView[2];
			alignas(16) pod::Vector4f eyePos[2];
			alignas(16) pod::Matrix4f vxgi;
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
	//	alignas(16) pod::Vector4f ambient;
		pod::Vector3f ambient;
		alignas(4) float gamma;

		alignas(4) float exposure;
		alignas(4) uint32_t msaa;
		alignas(4) uint32_t shadowSamples;
		alignas(4) float cascadePower;
	};
	struct SpecializationConstant {
		uint32_t maxTextures = 512;
	} specializationConstants;
	specializationConstants.maxTextures = maxTextures;

	struct LightInfo {
		uf::Entity* entity = NULL;
		pod::Vector4f color = {0,0,0,0};
		pod::Vector3f position = {0,0,0};
		float power = 0;
		float distance = 0;
		float bias = 0;
		bool shadows = false;
		int32_t type = 0;
	};
	std::vector<LightInfo> entities;
	std::vector<pod::Graph*> graphs;
	
	auto graph = uf::scene::generateGraph();
	for ( auto entity : graph ) {
		if ( entity == &controller || entity == this ) continue;
		if ( entity->hasComponent<pod::Graph>() ) graphs.emplace_back(&entity->getComponent<pod::Graph>());
		if ( !entity->hasComponent<ext::LightBehavior::Metadata>() ) continue;
		auto& metadata = entity->getComponent<ext::LightBehavior::Metadata>();
		if ( entity->hasComponent<uf::renderer::RenderTargetRenderMode>() ) {
			auto& renderMode = entity->getComponent<uf::renderer::RenderTargetRenderMode>();
			if ( metadata.renderer.mode == "in-range" ) {
			//	UF_DEBUG_MSG( renderModeName << "\t" << entity->getName() << "\t" << renderMode.execute );
				renderMode.execute = false;
			}
		}
		if ( metadata.power <= 0 ) continue;
		LightInfo& info = entities.emplace_back();
		auto& transform = entity->getComponent<pod::Transform<>>();
		auto flatten = uf::transform::flatten( transform );
		info.entity = entity;
		info.position = flatten.position;
		info.color = metadata.color;
		info.color.w = metadata.power;
		info.distance = uf::vector::magnitude( uf::vector::subtract( flatten.position, controllerTransform.position ) );
		info.shadows = metadata.shadows;
		info.bias = metadata.bias;
		info.type = metadata.type;
	}
	std::sort( entities.begin(), entities.end(), [&]( LightInfo& l, LightInfo& r ){
		return l.distance < r.distance;
	});

	int shadowSamples = metadata.light.shadowSamples;
	int shadowThreshold = metadata.light.shadowThreshold;
	if ( shadowSamples < 0 ) shadowSamples = 0;
	if ( shadowThreshold <= 0 ) shadowThreshold = std::numeric_limits<int>::max();

	{
		std::vector<LightInfo> scratch;
		scratch.reserve(entities.size());
		for ( size_t i = 0; i < entities.size(); ++i ) {
			auto& info = entities[i];
			if ( info.shadows && --shadowThreshold <= 0 ) info.shadows = false;
			if ( renderModeName != "" ) info.shadows = false;
			scratch.emplace_back(info);
		}
		entities = scratch;
	}
	auto& sceneTextures = this->getComponent<pod::SceneTextures>();
	for ( auto* blitter : blitters ) {
		auto& graphic = *blitter;
		if ( !graphic.initialized ) continue;

		auto& shader = graphic.material.getShader(isCompute ? "compute" : "fragment");
		auto& uniform = shader.getUniform("UBO");
		uint8_t* buffer = (uint8_t*) (void*) uniform;

		UniformDescriptor* uniforms = (UniformDescriptor*) buffer;

		for ( std::size_t i = 0; i < 2; ++i ) {
			uniforms->matrices.view[i] = camera.getView( i );
			uniforms->matrices.projection[i] = camera.getProjection( i );
			uniforms->matrices.iView[i] = uf::matrix::inverse( uniforms->matrices.view[i] );
			uniforms->matrices.iProjection[i] = uf::matrix::inverse( uniforms->matrices.projection[i] );
			uniforms->matrices.iProjectionView[i] = uf::matrix::inverse( uniforms->matrices.projection[i] * uniforms->matrices.view[i] );

			uniforms->matrices.eyePos[i] = camera.getEye( i );
		}

		uniforms->ambient = metadata.light.ambient;
		uniforms->msaa = ext::vulkan::settings::msaa;
		uniforms->shadowSamples = shadowSamples;
		uniforms->exposure = metadata.light.exposure;
		uniforms->gamma = metadata.light.gamma;
	
		uniforms->fog.color = metadata.fog.color;
		uniforms->fog.color.w = metadata.fog.stepScale;

		float timescale = metadata.fog.density.timescale;
		uniforms->fog.offset = metadata.fog.density.offset * uf::physics::time::current * timescale;
		uniforms->fog.offset.w = metadata.fog.density.scale;

		uniforms->fog.densityThreshold = metadata.fog.density.threshold;
		uniforms->fog.densityMultiplier = metadata.fog.density.multiplier;
		uniforms->fog.absorbtion = metadata.fog.absorbtion;

		uniforms->fog.range = metadata.fog.range;

		uniforms->mode.type.x = metadataJson["system"]["renderer"]["shader"]["mode"].as<size_t>();
		uniforms->mode.type.y = metadataJson["system"]["renderer"]["shader"]["scalar"].as<size_t>();
		uniforms->mode.parameters = uf::vector::decode( metadataJson["system"]["renderer"]["shader"]["parameters"], uniforms->mode.parameters );
		if ( metadataJson["system"]["renderer"]["shader"]["parameters"][3].as<std::string>() == "time" ) {
			uniforms->mode.parameters.w = uf::physics::time::current;
		}

		std::vector<VkImage> previousTextures;
		for ( auto& texture : graphic.material.textures ) previousTextures.emplace_back(texture.image);

		graphic.material.textures.clear();
		if ( uf::renderer::settings::experimental::deferredMode == "vxgi" ) {
			for ( auto& t : sceneTextures.voxels.id ) {
				graphic.material.textures.emplace_back().aliasTexture(t);
			}
			for ( auto& t : sceneTextures.voxels.uv ) {
				graphic.material.textures.emplace_back().aliasTexture(t);
			}
			for ( auto& t : sceneTextures.voxels.normal ) {
				graphic.material.textures.emplace_back().aliasTexture(t);
			}
			for ( auto& t : sceneTextures.voxels.radiance ) {
				graphic.material.textures.emplace_back().aliasTexture(t);
			}
		}
		
		graphic.material.textures.emplace_back().aliasTexture(sceneTextures.noise);
		graphic.material.textures.emplace_back().aliasTexture(sceneTextures.skybox);

		int32_t updateThreshold = metadata.light.updateThreshold;
		size_t textureSlot = 0;

		std::vector<pod::Light::Storage> lights;
		lights.reserve( metadata.max.lights );

		std::vector<pod::Material::Storage> materials;
		materials.reserve(maxTextures);
		materials.emplace_back().colorBase = {0,0,0,0};

		std::vector<pod::Texture::Storage> textures;
		textures.reserve(maxTextures);
		
		std::vector<pod::DrawCall::Storage> drawCalls;
		drawCalls.reserve(maxTextures);

		pod::Vector3f min = { std::numeric_limits<float>::max(), std::numeric_limits<float>::max(), std::numeric_limits<float>::max() };
		pod::Vector3f max = { -std::numeric_limits<float>::max(), -std::numeric_limits<float>::max(), -std::numeric_limits<float>::max() };

		// add materials
		for ( auto* g : graphs ) {
			auto& graph = *g;

			drawCalls.emplace_back(pod::DrawCall::Storage{
				materials.size(),
				graph.materials.size(),
				textures.size(),
				graph.textures.size()
			});
			
			for ( auto& material : graph.materials ) materials.emplace_back( material.storage );
			for ( auto& texture : graph.textures ) textures.emplace_back( texture.storage );

			for ( auto& texture : graph.textures ) {
				if ( !texture.bind ) continue;
				graphic.material.textures.emplace_back().aliasTexture(texture.texture);
				++textureSlot;
			}
		}
		uniforms->matrices.vxgi = sceneTextures.voxels.matrix;
		uniforms->cascadePower = sceneTextures.voxels.cascadePower;

		uniforms->lengths.materials = std::min( materials.size(), maxTextures );
		uniforms->lengths.textures = std::min( textures.size(), maxTextures );
		uniforms->lengths.drawCalls = std::min( drawCalls.size(), maxTextures );
		// add lighting
		for ( size_t i = 0; i < entities.size() && lights.size() < metadata.max.lights; ++i ) {
			auto& info = entities[i];
			uf::Entity* entity = info.entity;

			auto& transform = entity->getComponent<pod::Transform<>>();
			auto& metadata = entity->getComponent<ext::LightBehavior::Metadata>();
			auto& camera = entity->getComponent<uf::Camera>();
			metadata.renderer.rendered = true;

			pod::Light::Storage light;
			light.position = info.position;

			light.color = info.color;
			light.type = info.type;
			light.mapIndex = -1;

			light.depthBias = info.bias;
			if ( info.shadows && entity->hasComponent<uf::renderer::RenderTargetRenderMode>() ) {
				auto& renderMode = entity->getComponent<uf::renderer::RenderTargetRenderMode>();
				if ( metadata.renderer.mode == "in-range" && --updateThreshold > 0 ) {
					renderMode.execute = true;
				//	UF_DEBUG_MSG( renderModeName << "\t" << entity->getName() << "\t" << renderMode.execute );
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
		}

		{
			uniforms->lengths.lights = std::min( lights.size(), metadata.max.lights );
			bool shouldUpdate = graphic.material.textures.size() != previousTextures.size();
			for ( size_t i = 0; !shouldUpdate && i < previousTextures.size() && i < graphic.material.textures.size(); ++i ) {
				if ( previousTextures[i] != graphic.material.textures[i].image )
					shouldUpdate = true;
			}
			
			size_t lightBufferIndex = renderMode.metadata["lightBufferIndex"].as<size_t>();
			graphic.updateBuffer( (void*) lights.data(), uniforms->lengths.lights * sizeof(pod::Light::Storage), lightBufferIndex, false );
			
			if ( shouldUpdate ) {
				size_t materialBufferIndex = renderMode.metadata["materialBufferIndex"].as<size_t>();
				graphic.updateBuffer( (void*) materials.data(), uniforms->lengths.materials * sizeof(pod::Material::Storage), materialBufferIndex, false );

				size_t textureBufferIndex = renderMode.metadata["textureBufferIndex"].as<size_t>();
				graphic.updateBuffer( (void*) textures.data(), uniforms->lengths.textures * sizeof(pod::Texture::Storage), textureBufferIndex, false );
				
				size_t drawCallBufferIndex = renderMode.metadata["drawCallBufferIndex"].as<size_t>();
				graphic.updateBuffer( (void*) drawCalls.data(), uniforms->lengths.drawCalls * sizeof(pod::DrawCall::Storage), drawCallBufferIndex, false );

				graphic.updatePipelines();
			}
			
			shader.updateUniform( "UBO", uniform );	
		}
	}
#endif
}
#undef this