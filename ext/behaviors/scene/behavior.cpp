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
#include "../voxelizer/behavior.h"
#include "../../ext.h"
#include "../../gui/gui.h"

UF_BEHAVIOR_REGISTER_CPP(ext::ExtSceneBehavior)
UF_BEHAVIOR_TRAITS_CPP(ext::ExtSceneBehavior, ticks = true, renders = false, multithread = false)
#define this ((uf::Scene*) &self)
void ext::ExtSceneBehavior::initialize( uf::Object& self ) {
	auto& assetLoader = this->getComponent<uf::Asset>();
	auto& metadata = this->getComponent<ext::ExtSceneBehavior::Metadata>();
	auto& metadataJson = this->getComponent<uf::Serializer>();

	this->addHook( "system:Quit.%UID%", [&](ext::json::Value& json){
		UF_MSG_DEBUG(json);
		ext::ready = false;
	});

	this->addHook( "asset:Load.%UID%", [&](ext::json::Value& json){
		uf::stl::string filename = json["filename"].as<uf::stl::string>();
		uf::stl::string category = json["category"].as<uf::stl::string>();

		if ( category != "" && (category != "audio" && category != "audio-stream") ) return;
		if ( category == "" && uf::io::extension(filename) != "ogg" ) return;

		if ( !assetLoader.has<uf::Audio>(filename) ) return;
		auto& asset = assetLoader.get<uf::Audio>(filename);
		auto& audio = this->getComponent<uf::Audio>();

		audio.destroy();
		audio = std::move(asset);
		assetLoader.remove<uf::Audio>(filename);

		audio.setVolume(metadataJson["volumes"]["bgm"].as<float>());
		audio.loop( true );
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
		uf::stl::string config = metadataJson["menus"]["pause"].is<uf::stl::string>() ? metadataJson["menus"]["pause"].as<uf::stl::string>() : "/scenes/worldscape/gui/pause/menu.json";
		uf::Object& gui = manager->loadChild(config, false);
		payload["uid"] = gui.getUid();

		uf::Serializer& metadataJson = gui.getComponent<uf::Serializer>();
		metadataJson["menu"] = json["menu"];
		
		gui.initialize();
	});
	this->addHook( "world:Entity.LoadAsset", [&](ext::json::Value& json){
		uf::stl::string asset = json["asset"].as<uf::stl::string>();
		uf::stl::string uid = json["uid"].as<uf::stl::string>();

		assetLoader.load(asset, "asset:Load." + uid);
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
	this->addHook( "window:Resized", [&](ext::json::Value& json){
		pod::Vector2ui size = uf::vector::decode( json["window"]["size"], pod::Vector2i{} );

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
		uf::stl::vector<uf::stl::string> filenames = {
			uf::io::root+"/textures/skybox/front.png",
			uf::io::root+"/textures/skybox/back.png",
			uf::io::root+"/textures/skybox/up.png",
			uf::io::root+"/textures/skybox/down.png",
			uf::io::root+"/textures/skybox/right.png",
			uf::io::root+"/textures/skybox/left.png",
		};
		uf::Image::container_t pixels;
		uf::stl::vector<uf::Image> images(filenames.size());

		pod::Vector2ui size = {0,0};
		auto& texture = sceneTextures.skybox;
		for ( uint32_t i = 0; i < filenames.size(); ++i ) {
			auto& filename = filenames[i];
			auto& image = images[i];
			image.open(filename);
			image.flip();

			if ( size.x == 0 && size.y == 0 ) {
				size = image.getDimensions();
			} else if ( size != image.getDimensions() ) {
				UF_MSG_ERROR("ERROR: MISMATCH CUBEMAP FACE SIZE");
			}

			auto& p = image.getPixels();
			pixels.reserve( pixels.size() + p.size() );
			pixels.insert( pixels.end(), p.begin(), p.end() );
		}
		texture.mips = 0;
		texture.fromBuffers( (void*) pixels.data(), pixels.size(), uf::renderer::enums::Format::R8G8B8A8_UNORM, size.x, size.y, 1, filenames.size() );
	}
	#endif

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

		metadataJson["system"]["renderer"]["shader"]["mode"] = metadata.shader.mode;
		metadataJson["system"]["renderer"]["shader"]["scalar"] = metadata.shader.scalar;
		metadataJson["system"]["renderer"]["shader"]["parameters"] = uf::vector::encode( metadata.shader.parameters );
	};
	metadata.deserialize = [&](){
		if ( !metadataJson["light"]["point light eye depth scale"].is<float>() )
			metadataJson["light"]["point light eye depth scale"] = ext::config["engine"]["scenes"]["lights"]["point light eye depth scale"];

		metadata.max.textures2D   = ext::config["engine"]["scenes"]["textures"]["max"]["2D"].as<uint32_t>(metadata.max.textures2D);
		metadata.max.texturesCube = ext::config["engine"]["scenes"]["textures"]["max"]["cube"].as<uint32_t>(metadata.max.texturesCube);
		metadata.max.textures3D   = ext::config["engine"]["scenes"]["textures"]["max"]["3D"].as<uint32_t>(metadata.max.textures3D);
		metadata.max.lights = ext::config["engine"]["scenes"]["lights"]["max"].as<uint32_t>(metadata.max.lights);

		metadata.light.enabled = ext::config["engine"]["scenes"]["lights"]["enabled"].as<bool>(true) && metadataJson["light"]["should"].as<bool>(true);

		metadata.shadow.enabled = ext::config["engine"]["scenes"]["shadows"]["enabled"].as<bool>(true) && metadataJson["light"]["shadows"].as<bool>(true);
		metadata.shadow.samples = ext::config["engine"]["scenes"]["shadows"]["samples"].as<uint32_t>();
		metadata.shadow.max = ext::config["engine"]["scenes"]["shadows"]["max"].as<uint32_t>();
		metadata.shadow.update = ext::config["engine"]["scenes"]["shadows"]["update"].as<uint32_t>();
		metadata.shadow.experimentalMode = ext::config["engine"]["scenes"]["shadows"]["experimental mode"].as<uint32_t>(0);
	
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

		metadata.shader.mode = metadataJson["system"]["renderer"]["shader"]["mode"].as<uint32_t>();
		metadata.shader.scalar = metadataJson["system"]["renderer"]["shader"]["scalar"].as<uint32_t>();
		metadata.shader.parameters = uf::vector::decode( metadataJson["system"]["renderer"]["shader"]["parameters"], pod::Vector4f{0,0,0,0} );
		ext::json::forEach( metadataJson["system"]["renderer"]["shader"]["parameters"], [&]( uint32_t i, const ext::json::Value& value ){
			if ( value.as<uf::stl::string>() == "time" ) metadata.shader.time = i;
		});
		if ( 0 <= metadata.shader.time && metadata.shader.time < 4 ) {
			metadata.shader.parameters[metadata.shader.time] = uf::physics::time::current;
		}
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
#if 1
	uf::hooks.call("game:Frame.Start");
	metadata.shader.invalidated = false;

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
#endif
#if 0
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
			filename = assetLoader.getOriginal(filename);
			if ( filename.find("_intro") != uf::stl::string::npos ) {
				assetLoader.load(uf::string::replace( filename, "_intro", "" ), this->formatHookName("asset:Load.%UID%"));
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
	if ( 0 <= metadata.shader.time && metadata.shader.time < 4 ) {
		metadata.shader.parameters[metadata.shader.time] = uf::physics::time::current;
	}
#endif
	/* Update lights */ if ( !uf::renderer::settings::experimental::vxgi ) {
		ext::ExtSceneBehavior::bindBuffers( *this );
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

void ext::ExtSceneBehavior::bindBuffers( uf::Object& self, const uf::stl::string& renderModeName, bool isCompute ) {
	auto& graph = this->getGraph();
	auto& controller = this->getController();
	auto& camera = controller.getComponent<uf::Camera>();
	auto& controllerMetadata = controller.getComponent<uf::Serializer>();
	auto& controllerTransform = controller.getComponent<pod::Transform<>>();
	auto& metadata = this->getComponent<ext::ExtSceneBehavior::Metadata>();
	auto& metadataVxgi = this->getComponent<ext::VoxelizerBehavior::Metadata>();
	auto& metadataJson = this->getComponent<uf::Serializer>();

	auto& renderMode = uf::renderer::getRenderMode(renderModeName, true);
	uf::stl::vector<uf::Graphic*> blitters = renderMode.getBlitters();
	
#if UF_USE_OPENGL
	struct LightInfo {
		uf::Entity* entity = NULL;
		pod::Vector3f position = {0,0,0}; 
		float w = 1; // OpenGL requires a W
		pod::Vector4f color = {0,0,0,1}; // OpenGL requires an alpha
		float distance = 0;
		float power = 0;
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
			.w = 1,
			.color = metadata.color,
			.distance = uf::vector::magnitude( uf::vector::subtract( flatten.position, controllerTransform.position ) ),
			.power = metadata.power,
		});
	}
	std::sort( entities.begin(), entities.end(), [&]( LightInfo& l, LightInfo& r ){
		return l.distance < r.distance;
	});

	static GLint glMaxLights = 0;
	if ( !glMaxLights ) glGetIntegerv(GL_MAX_LIGHTS, &glMaxLights);
	metadata.max.lights = std::min( (uint32_t) glMaxLights, metadata.max.lights );

	// add lighting
	{
		uint32_t i = 0;	
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
	struct UniformDescriptor {
		struct Matrices {
			alignas(16) pod::Matrix4f view;
			alignas(16) pod::Matrix4f projection;
			alignas(16) pod::Matrix4f iView;
			alignas(16) pod::Matrix4f iProjection;
			alignas(16) pod::Matrix4f iProjectionView;
			alignas(16) pod::Vector4f eyePos;
		} matrices[2];

		struct Mode {
			alignas(4) uint8_t mode;
			alignas(4) uint8_t scalar;
			alignas(8) pod::Vector2ui padding;
			alignas(16) pod::Vector4f parameters;
		} mode;

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

		struct VXGI {
			alignas(16) pod::Matrix4f matrix;
			alignas(4) float cascadePower;
			alignas(4) float granularity;
			alignas(4) uint32_t shadows;
			alignas(4) uint32_t padding1;
		} vxgi;

		struct Lengths {
			alignas(4) uint32_t lights = 0;
			alignas(4) uint32_t materials = 0;
			alignas(4) uint32_t textures = 0;
			alignas(4) uint32_t drawCalls = 0;
		} lengths;

		pod::Vector3f ambient;
		alignas(4) float gamma;

		alignas(4) float exposure;
		alignas(4) uint32_t msaa;
		alignas(4) uint32_t shadowSamples;		
		alignas(4) uint32_t indexSkybox;
	};

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
	entities.reserve(graph.size() / 2);

	// struct that contains our skybox cubemap, noise texture, and VXGI voxels
	auto& sceneTextures = this->getComponent<pod::SceneTextures>();
	uf::stl::vector<uf::renderer::Texture> textures2D;
	textures2D.reserve( metadata.max.textures2D );

	uf::stl::vector<uf::renderer::Texture> textures3D;
	textures3D.reserve( metadata.max.textures3D );
	
	uf::stl::vector<uf::renderer::Texture> texturesCube;
	texturesCube.reserve( metadata.max.texturesCube );

	// lighting information
	uf::stl::vector<pod::Light::Storage> lights;
	lights.reserve( metadata.max.lights );

	// material information
	uf::stl::vector<pod::Material::Storage> materials;
	materials.reserve( metadata.max.textures2D );
	materials.emplace_back().colorBase = {0,0,0,0}; // setup our fallback material information
	// texture information
	uf::stl::vector<pod::Texture::Storage> textures;
	textures.reserve( metadata.max.textures2D );
	// drawcall information
	uf::stl::vector<pod::DrawCall::Storage> drawCalls;
	drawCalls.reserve( metadata.max.textures2D );

	// traverse scene graph
	for ( auto entity : graph ) {
		// needed to grab our necessary materials/texture information, until i add it globally
		if ( entity->hasComponent<pod::Graph>() ) {
			auto& graph = entity->getComponent<pod::Graph>();
			// pass our draw call information, necessary in shader
			drawCalls.emplace_back(pod::DrawCall::Storage{
				materials.size(),
				textures.size(),
				textures2D.size(),
				0,
			});
			// pass material information
			for ( auto& material : graph.materials ) materials.emplace_back( material.storage );
			// pass texture information
			for ( auto& texture : graph.textures ) {
				textures.emplace_back( texture.storage );
				// attach the actual texture if the texture information requests it
				if ( texture.bind ) textures2D.emplace_back().aliasTexture(texture.texture);
			}
		}
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
	// prioritize closer lights; it would be nice to also prioritize lights in view, but because of VXGI it's not really something to do
	std::sort( entities.begin(), entities.end(), [&]( LightInfo& l, LightInfo& r ){
		return l.distance < r.distance;
	});

	int32_t shadowUpdateThreshold = metadata.shadow.update; // how many shadow maps we should update, based on range
	int32_t shadowCount = metadata.shadow.max; // how many shadow maps we should pass, based on range
	if ( shadowCount <= 0 ) shadowCount = std::numeric_limits<int32_t>::max();

	// disable shadows if that light is outside our threshold
	for ( auto& info : entities ) if ( info.shadows && shadowCount-- <= 0 ) info.shadows = false;

	// bind skybox
	texturesCube.emplace_back().aliasTexture(sceneTextures.skybox);
	// bind noise texture
	textures3D.emplace_back().aliasTexture(sceneTextures.noise);
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
			if ( metadata.shadow.experimentalMode > 0 && renderMode.renderTarget.views == 6 ) {
				int32_t index = -1;
				// separated texture2Ds
				if ( metadata.shadow.experimentalMode == 2 ) {
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
					.typeMap = metadata.shadow.experimentalMode,
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

	// attach VXGI voxels
	if ( uf::renderer::settings::experimental::vxgi ) {
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

	// update uniform information
	// hopefully write combining kicks in
	UniformDescriptor uniforms; {
		for ( auto i = 0; i < 2; ++i ) {
			uniforms.matrices[i] = UniformDescriptor::Matrices{
				.view = camera.getView(i),
				.projection = camera.getProjection(i),
				.iView = uf::matrix::inverse( camera.getView(i) ),
				.iProjection = uf::matrix::inverse( camera.getProjection(i) ),
				.iProjectionView = uf::matrix::inverse( camera.getProjection(i) * camera.getView(i) ),
				.eyePos = camera.getEye( i ),
			};
		}
		uniforms.mode = UniformDescriptor::Mode{
			.mode = metadata.shader.mode,
			.scalar = metadata.shader.scalar,
			.padding = pod::Vector2ui{0,0},
			.parameters = metadata.shader.parameters,
		};
		uniforms.fog = UniformDescriptor::Fog{
			.color = metadata.fog.color,
			.stepScale = metadata.fog.stepScale,

			.offset = metadata.fog.density.offset * (float) metadata.fog.density.timescale * (float) uf::physics::time::current,
			.densityScale = metadata.fog.density.scale,

			.range = metadata.fog.range,
			.densityThreshold = metadata.fog.density.threshold,
			.densityMultiplier = metadata.fog.density.multiplier,

			.absorbtion = metadata.fog.absorbtion,
		};

		uniforms.vxgi = UniformDescriptor::VXGI{
			.matrix = metadataVxgi.extents.matrix,
			.cascadePower = metadataVxgi.cascadePower,
			.granularity = metadataVxgi.granularity,
			.shadows = metadataVxgi.shadows,
		};
	
		uniforms.lengths = UniformDescriptor::Lengths{
			.lights = MIN( lights.size(), metadata.max.lights ),
			.materials = MIN( materials.size(), metadata.max.textures2D ),
			.textures = MIN( textures.size(), metadata.max.textures2D ),
			.drawCalls = MIN( drawCalls.size(), metadata.max.textures2D ),
		};

		uniforms.ambient = metadata.light.ambient;
		uniforms.gamma = metadata.light.gamma;

		uniforms.exposure = metadata.light.exposure;
		uniforms.msaa = ext::vulkan::settings::msaa;
		uniforms.shadowSamples = std::min( 0, metadata.shadow.samples );
		uniforms.indexSkybox = 0;
	}

	for ( auto* blitter : blitters ) {
		auto& graphic = *blitter;
		if ( !graphic.initialized ) continue;

		auto& shader = graphic.material.getShader(isCompute ? "compute" : "fragment");
		
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
			if ( previousTextures[i] != graphic.material.textures[i].image )
				shouldUpdate = true;
		}
		
		// update lighting buffer
		graphic.updateBuffer( (const void*) lights.data(), uniforms.lengths.lights * sizeof(pod::Light::Storage), renderMode.metadata.lightBufferIndex, false );
		//
		if ( shouldUpdate ) {
			graphic.updateBuffer( (const void*) materials.data(), uniforms.lengths.materials * sizeof(pod::Material::Storage), renderMode.metadata.materialBufferIndex, false );
			graphic.updateBuffer( (const void*) textures.data(), uniforms.lengths.textures * sizeof(pod::Texture::Storage), renderMode.metadata.textureBufferIndex, false );
			graphic.updateBuffer( (const void*) drawCalls.data(), uniforms.lengths.drawCalls * sizeof(pod::DrawCall::Storage), renderMode.metadata.drawCallBufferIndex, false );

			graphic.updatePipelines();
		}
	
	//	graphic.updateBuffer( (const void*) &uniforms, sizeof(uniforms), shader.getUniformBuffer("UBO") );	
		graphic.updateBuffer( uniforms, shader.getUniformBuffer("UBO") );
	/*
		auto& uniform = shader.getUniform("UBO");
		memcpy( (void*) uniform, &uniforms, sizeof(uniforms) );
		shader.updateUniform( "UBO", uniform );	
	*/
	}
#endif
}
#undef this