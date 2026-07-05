#include <uf/engine/ext.h>

#include <fstream>
#include <iostream>
#include <regex>
#include <cstdlib>

#include <sys/stat.h>

#include <uf/utils/time/time.h>
#include <uf/utils/audio/audio.h>
#include <uf/utils/hook/hook.h>
#include <uf/utils/io/iostream.h>
#include <uf/utils/math/vector.h>
#include <uf/utils/math/transform.h>
#include <uf/utils/string/string.h>
#include <uf/utils/string/ext.h>
#include <uf/utils/serialize/serializer.h>
#include <uf/utils/userdata/userdata.h>
#include <uf/utils/image/image.h>
#include <uf/utils/thread/thread.h>
#include <uf/utils/graphic/graphic.h>
#include <uf/utils/camera/camera.h>
#include <uf/utils/http/http.h>
#include <uf/utils/renderer/renderer.h>
#include <uf/utils/io/console.h>
#include <uf/utils/io/inputs.h>
#include <uf/spec/terminal/terminal.h>
#include <uf/spec/controller/controller.h>
#include <uf/utils/memory/string.h>

#include <uf/engine/entity/entity.h>
#include <uf/engine/graph/graph.h>
#include <uf/engine/scene/scene.h>
#include <uf/engine/asset/asset.h>

#include <uf/utils/math/physics.h>

#include <uf/ext/ext.h>
#include <uf/ext/openal/openal.h>
#include <uf/ext/freetype/freetype.h>
#include <uf/ext/discord/discord.h>
#include <uf/ext/openvr/openvr.h>
#include <uf/ext/lua/lua.h>
#include <uf/ext/ultralight/ultralight.h>
#include <uf/ext/ffx/fsr.h>
#include <uf/ext/imgui/imgui.h>
#include <uf/ext/vall_e/vall_e.h>
#include <uf/ext/valve/vpk.h>

#if UF_USE_OPENVR && UF_USE_VULKAN
#include <uf/ext/vulkan/rendermodes/vr.h>
#endif

bool uf::ready = false;
uf::stl::vector<uf::stl::string> uf::arguments;
uf::Serializer uf::config;

namespace {
	struct {
		uf::stl::string input;
		std::ofstream output;

		struct {
			uf::stl::string output;
		} filenames;
	} io;

	struct {
		uf::Timer<> sys = uf::Timer<>(false);
		size_t frames = 0;
		float limiter = 1.0 / 144.0;
		struct {
			size_t frames = 0;
			float time = 0;
		} total;
	} times;

	struct {
		struct {
			struct {
				size_t mode;
				bool announce;
				float every;
				bool enabled;
			} gc;

			struct {
				struct {
					bool enabled;
				} ultralight, discord, imgui;
				struct {
					bool enabled;
					std::string model_path = "";
					std::string encodec_path = "";
				} vall_e;
			} ext;

			struct {
				bool print;
				float every;
			} limiter, fps;
		} engine;
	} config;

	bool requestDedicatedRenderThread = false;
	bool requestDeferredCommandBufferSubmit = false;

	struct {
		int phase = -1;
		uf::Serializer payload;
	} sceneTransition;
}

void UF_API uf::load() {
	uf::config.readFromFile(uf::io::root+"config.json");
}
void UF_API uf::load( ext::json::Value& json ) {
	auto& configWindowJson = json["window"];
	auto& configEngineJson = json["engine"];

	auto& configEngineExtJson = configEngineJson["ext"];
	auto& configEngineSceneJson = configEngineJson["scenes"];
	auto& configEngineGraphJson = configEngineJson["graph"];
	auto& configEngineDebugJson = configEngineJson["debug"];
	auto& configEngineAudioJson = configEngineJson["audio"];
	auto& configEngineLimitersJson = configEngineJson["limiters"];
	auto& configEngineThreadJson = configEngineJson["threads"];
	auto& configEnginePhysicsJson = configEngineJson["physics"];
	auto& configEnginePhysicsSolverJson = configEnginePhysicsJson["solvers"];
	auto& configEnginePhysicsCorrectionJson = configEnginePhysicsJson["correction"];
	auto& configEnginePhysicsDebugDrawJson = configEnginePhysicsJson["debug draw"];
#if UF_USE_FFX_FSR || UF_USE_FFX_SDK
	auto& configEngineExtFfxJson = configEngineExtJson["fsr"];
#endif
#if UF_USE_VALL_E
	auto& configEngineExtValleJson = configEngineExtJson["vall_e"];
#endif
#if UF_USE_ULTRALIGHT
	auto& configEngineExtUltralightJson = configEngineExtJson["ultralight"];
#endif
#if UF_USE_VULKAN
	auto& configRenderJson = configEngineExtJson["vulkan"];
#elif UF_USE_OPENGL
	auto& configRenderJson = configEngineExtJson["opengl"];
#else
	auto& configRenderJson = configEngineExtJson["software"];
#endif
	auto& configRenderInvariantJson = configRenderJson["invariant"];
	auto& configRenderExperimentalJson = configRenderJson["experimental"];
	auto& configRenderPipelinesJson = configRenderJson["pipelines"];

	// Scene settings
	{
		uf::matrix::reverseInfiniteProjection = configEngineSceneJson["matrix"]["reverseInfinite"].as( uf::matrix::reverseInfiniteProjection );
	}

	// Graph settings
	{
		uf::graph::initialBufferElements = configEngineGraphJson["initial buffer elements"].as(uf::graph::initialBufferElements);
		if ( configEngineGraphJson["storage mode"].is<uf::stl::string>()  ) {
			auto mode = uf::string::lowercase( configEngineGraphJson["storage mode"].as<uf::stl::string>() );
			if ( mode == "object" ) uf::graph::storageMode = pod::Graph::Storage::OBJECT;
			else if ( mode == "graph" ) uf::graph::storageMode = pod::Graph::Storage::GRAPH;
			else if ( mode == "scene" ) uf::graph::storageMode = pod::Graph::Storage::SCENE;
			else if ( mode == "global" ) uf::graph::storageMode = pod::Graph::Storage::GLOBAL;
		} else if ( configEngineGraphJson["storage mode"].is<uint32_t>()  ) {
			uf::graph::storageMode = configEngineGraphJson["storage mode"].as(uf::graph::storageMode);
		}
	}

	// Various debug settings
	{
		::config.engine.gc.enabled = configEngineDebugJson["garbage collection"]["enabled"].as(::config.engine.gc.enabled);
		::config.engine.gc.every = configEngineDebugJson["garbage collection"]["every"].as(::config.engine.gc.every);
		::config.engine.gc.mode = configEngineDebugJson["garbage collection"]["mode"].as(::config.engine.gc.mode);
		::config.engine.gc.announce = configEngineDebugJson["garbage collection"]["announce"].as(::config.engine.gc.announce);

		::config.engine.limiter.print = configEngineDebugJson["framerate"]["print"].as(::config.engine.limiter.print);
		::config.engine.fps.print = configEngineDebugJson["framerate"]["print"].as(::config.engine.fps.print);
		::config.engine.fps.every = configEngineDebugJson["framerate"]["every"].as(::config.engine.fps.every);
		
		uf::Entity::deleteChildrenOnDestroy = configEngineDebugJson["entity"]["delete children on destroy"].as( uf::Entity::deleteChildrenOnDestroy );
		uf::Entity::deleteComponentsOnDestroy = configEngineDebugJson["entity"]["delete components on destroy"].as( uf::Entity::deleteComponentsOnDestroy );

		uf::Object::assertionLoad = configEngineDebugJson["loader"]["assert"].as( uf::Object::assertionLoad );
		uf::asset::assertionLoad = configEngineDebugJson["loader"]["assert"].as( uf::asset::assertionLoad );
		uf::asset::asyncQueue = configEngineDebugJson["loader"]["async"].as( uf::asset::asyncQueue );
		
		uf::userdata::autoDestruct = configEngineDebugJson["userdata"]["auto destruct"].as( uf::userdata::autoDestruct );
		uf::userdata::autoValidate = configEngineDebugJson["userdata"]["auto validate"].as( uf::userdata::autoValidate );
		
		uf::Object::deferLazyCalls = configEngineDebugJson["hooks"]["defer lazy calls"].as( uf::Object::deferLazyCalls );
		uf::scene::printTaskCalls = configEngineDebugJson["scene"]["print task calls"].as( uf::scene::printTaskCalls );
	}
	// Limiter settings
	{
		if ( configEngineLimitersJson["framerate"].as<uf::stl::string>() == "auto" && configWindowJson["refresh rate"].is<size_t>() ) {
			float scale = 1.0;
			size_t refreshRate = configWindowJson["refresh rate"].as<size_t>();
			configEngineLimitersJson["framerate"] = refreshRate * scale;
			UF_MSG_DEBUG("Setting framerate cap to {}", (int) refreshRate * scale);
		}

		/* Frame limiter */ {
			size_t limit = configEngineLimitersJson["framerate"].as<size_t>();
			::times.limiter = limit != 0 ? 1.0 / limit : 0;
			UF_MSG_DEBUG("Limiter set to {} ms", ::times.limiter);
		}
		/* Max delta time */{
			size_t limit = configEngineLimitersJson["deltaTime"].as<size_t>();
			uf::physics::time::clamp = limit != 0 ? 1.0 / limit : 0;
		}
	}

	// Thread settings
	{
		if ( configEngineThreadJson["frame limiter"].as<uf::stl::string>() == "auto" && configWindowJson["refresh rate"].is<size_t>() ) {
			float scale = 2.0;
			size_t refreshRate = configWindowJson["refresh rate"].as<size_t>();
			configEngineThreadJson["frame limiter"] = refreshRate * scale;
			UF_MSG_DEBUG("Setting thread frame limiter to {}", (int) refreshRate * scale);
		}

		/* Thread frame limiter */ {
			size_t limit = configEngineThreadJson["frame limiter"].as<size_t>();
			uf::thread::limiter = limit != 0 ? 1.0 / limit : 0;
		}

		// Set worker threads
		if ( configEngineThreadJson["workers"].as<uf::stl::string>() == "auto" ) {
			auto threads = std::max( 1, (int) std::thread::hardware_concurrency() - 1 ) / 2;
			configEngineThreadJson["workers"] = threads;
			uf::thread::workers = configEngineThreadJson["workers"].as<size_t>();
			UF_MSG_DEBUG("Using {} worker threads", threads);
		} else if ( configEngineThreadJson["workers"].is<size_t>() ) {
			auto threads = configEngineThreadJson["workers"].as<size_t>();
			uf::thread::workers = threads;
			UF_MSG_DEBUG("Using {} worker threads", threads);
		}
	}

	// Physics settings
	{
		uf::physics::settings.async = configEnginePhysicsJson["async"].as(uf::physics::settings.async);
		uf::physics::settings.timestep = configEnginePhysicsJson["timestep"].as(uf::physics::settings.timestep);
		uf::physics::settings.fixedStep = configEnginePhysicsJson["fixed step"].as(uf::physics::settings.fixedStep);
		uf::physics::settings.substeps = configEnginePhysicsJson["substeps"].as(uf::physics::settings.substeps);
		uf::physics::settings.flattenTransforms = configEnginePhysicsJson["flatten transforms"].as(uf::physics::settings.flattenTransforms);

		if ( ext::json::isObject( configEnginePhysicsSolverJson ) ) {
			uf::physics::settings.useGjk = configEnginePhysicsSolverJson["gjk"].as(uf::physics::settings.useGjk);
			uf::physics::settings.blockContactSolver = configEnginePhysicsSolverJson["block"].as(uf::physics::settings.blockContactSolver);
			uf::physics::settings.warmupSolver = configEnginePhysicsSolverJson["warmup"].as(uf::physics::settings.warmupSolver);
			uf::physics::settings.resolveBlockContact = configEnginePhysicsSolverJson["resolve invalid"].as(uf::physics::settings.resolveBlockContact);
			uf::physics::settings.solverIterations = configEnginePhysicsSolverJson["iterations"].as(uf::physics::settings.solverIterations);
		}
		if ( ext::json::isObject( configEnginePhysicsCorrectionJson ) ) {
			uf::physics::settings.ngsPositionSolver = configEnginePhysicsCorrectionJson["ngs"].as(uf::physics::settings.ngsPositionSolver);
			uf::physics::settings.baumgarteCorrectionPercent = configEnginePhysicsCorrectionJson["percent"].as(uf::physics::settings.baumgarteCorrectionPercent);
			uf::physics::settings.baumgarteCorrectionSlop = configEnginePhysicsCorrectionJson["slop"].as(uf::physics::settings.baumgarteCorrectionSlop);
			uf::physics::settings.maxLinearCorrection = configEnginePhysicsCorrectionJson["max"].as(uf::physics::settings.maxLinearCorrection);
		}
		if ( ext::json::isObject( configEnginePhysicsDebugDrawJson ) ) {
			if ( configEnginePhysicsDebugDrawJson["static"].as<bool>() ) uf::physics::settings.debugDraw.mask |= pod::Collider::CATEGORY_STATIC;
			if ( configEnginePhysicsDebugDrawJson["dynamic"].as<bool>() ) uf::physics::settings.debugDraw.mask |= pod::Collider::CATEGORY_DYNAMIC;
			if ( configEnginePhysicsDebugDrawJson["player"].as<bool>() ) uf::physics::settings.debugDraw.mask |= pod::Collider::CATEGORY_PLAYER;
			if ( configEnginePhysicsDebugDrawJson["npc"].as<bool>() ) uf::physics::settings.debugDraw.mask |= pod::Collider::CATEGORY_NPC;
			if ( configEnginePhysicsDebugDrawJson["trigger"].as<bool>() ) uf::physics::settings.debugDraw.mask |= pod::Collider::CATEGORY_TRIGGER;
			if ( configEnginePhysicsDebugDrawJson["projectile"].as<bool>() ) uf::physics::settings.debugDraw.mask |= pod::Collider::CATEGORY_PROJECTILE;
			if ( configEnginePhysicsDebugDrawJson["character"].as<bool>() ) uf::physics::settings.debugDraw.mask |= pod::Collider::CATEGORY_CHARACTER;
			if ( configEnginePhysicsDebugDrawJson["all"].as<bool>() ) uf::physics::settings.debugDraw.mask |= pod::Collider::CATEGORY_ALL;
			
			uf::physics::settings.debugDraw.contacts = configEnginePhysicsDebugDrawJson["contacts"].as( uf::physics::settings.debugDraw.contacts );
			uf::physics::settings.debugDraw.constraints = configEnginePhysicsDebugDrawJson["constraints"].as( uf::physics::settings.debugDraw.constraints );
			uf::physics::settings.debugDraw.rays = configEnginePhysicsDebugDrawJson["rays"].as( uf::physics::settings.debugDraw.rays );
			uf::physics::settings.debugDraw.depthTest = configEnginePhysicsDebugDrawJson["depthTest"].as( uf::physics::settings.debugDraw.depthTest );

		} else if ( configEnginePhysicsDebugDrawJson.is<bool>() && configEnginePhysicsDebugDrawJson.as<bool>() ) {
			uf::physics::settings.debugDraw.mask = pod::Collider::CATEGORY_ALL;
			uf::physics::settings.debugDraw.contacts = true;
			uf::physics::settings.debugDraw.constraints = true;
			uf::physics::settings.debugDraw.rays = true;
		}
	}

	// Audio settings
	{
		uf::audio::muted = configEngineAudioJson["mute"].as( uf::audio::muted );
		uf::audio::asyncUpdate = configEngineAudioJson["async update"].as( uf::audio::asyncUpdate );
		uf::audio::streamsByDefault = configEngineAudioJson["streams by default"].as( uf::audio::streamsByDefault );
		uf::audio::bufferSize = configEngineAudioJson["buffers"]["size"].as( uf::audio::bufferSize );
		uf::audio::buffers = configEngineAudioJson["buffers"]["count"].as( uf::audio::buffers );
	#if UF_AUDIO_MAPPED_VOLUMES
		ext::json::forEach( configEngineAudioJson["volumes"], []( const uf::stl::string& key, ext::json::Value& value ){
			float volume; volume = value.as(volume);
			uf::audio::volumes[key] = volume;
		});
	#else
		if ( ext::json::isObject( configEngineAudioJson["volumes"] ) ) {
			uf::audio::volumes::bgm = configEngineAudioJson["volumes"]["bgm"].as(uf::audio::volumes::bgm);
			uf::audio::volumes::sfx = configEngineAudioJson["volumes"]["sfx"].as(uf::audio::volumes::sfx);
			uf::audio::volumes::voice = configEngineAudioJson["volumes"]["voice"].as(uf::audio::volumes::voice);
		}
	#endif
	}

	// Various external settings
#if UF_USE_DISCORD
	{
		::config.engine.ext.discord.enabled = configEngineExtJson["discord"]["enabled"].as(::config.engine.ext.discord.enabled);
	}
#endif
#if UF_USE_IMGUI
	{
		::config.engine.ext.imgui.enabled = configEngineExtJson["imgui"]["enabled"].as(::config.engine.ext.imgui.enabled);
	}
#endif
#if UF_USE_VALL_E
	// VALL-E settings
	{
		::config.engine.ext.vall_e.enabled = configEngineExtValleJson["enabled"].as(::config.engine.ext.vall_e.enabled);
		::config.engine.ext.vall_e.model_path = configEngineExtValleJson["model_path"].as(::config.engine.ext.vall_e.model_path);
		::config.engine.ext.vall_e.encodec_path = configEngineExtValleJson["encodec_path"].as(::config.engine.ext.vall_e.encodec_path);
	}
#endif

#if UF_USE_ULTRALIGHT
	// Ultralight settings (painfully unused)
	{
		::config.engine.ext.ultralight.enabled = configEngineExtUltralightJson["enabled"].as(::config.engine.ext.ultralight.enabled);
		ext::ultralight::scale = configEngineExtUltralightJson["scale"].as( ext::ultralight::scale );
		ext::ultralight::log = configEngineExtUltralightJson["log"].as( ext::ultralight::log );
	}
#endif

	// Renderer settings
	{
		uf::renderer::settings::validation::messages = configRenderJson["validation"]["messages"].as( uf::renderer::settings::validation::messages );
		uf::renderer::settings::validation::checkpoints = configRenderJson["validation"]["checkpoints"].as( uf::renderer::settings::validation::checkpoints );
		
		uf::renderer::settings::experimental::batchQueueSubmissions = configRenderExperimentalJson["batch queue submissions"].as( uf::renderer::settings::experimental::batchQueueSubmissions );

	#if UF_USE_VULKAN
		uf::renderer::settings::defaultStageBuffers = configRenderInvariantJson["default stage buffers"].as( uf::renderer::settings::defaultStageBuffers );
		uf::renderer::settings::defaultDeferBufferDestroy = configRenderInvariantJson["default defer buffer destroy"].as( uf::renderer::settings::defaultDeferBufferDestroy );
	#if 0
		uf::renderer::settings::defaultCommandBufferImmediate = true;
		::requestDeferredCommandBufferSubmit = !configRenderInvariantJson["default command buffer immediate"].as( uf::renderer::settings::defaultCommandBufferImmediate );
	#else
		uf::renderer::settings::defaultCommandBufferImmediate = configRenderInvariantJson["default command buffer immediate"].as( uf::renderer::settings::defaultCommandBufferImmediate );
	#endif
		uf::renderer::settings::nBufferedUbos = configRenderInvariantJson["n-buffered uniform"].as( uf::renderer::settings::nBufferedUbos );
	#endif
	#if 1
		uf::renderer::settings::experimental::dedicatedThread = false;
		::requestDedicatedRenderThread = configRenderExperimentalJson["dedicated thread"].as( uf::renderer::settings::experimental::dedicatedThread );
	#else
		uf::renderer::settings::experimental::dedicatedThread = configRenderExperimentalJson["dedicated thread"].as( uf::renderer::settings::experimental::dedicatedThread );
	#endif
		uf::renderer::settings::invariant::multithreadedRecording = configRenderInvariantJson["multithreaded recording"].as( uf::renderer::settings::invariant::multithreadedRecording );
		uf::renderer::settings::invariant::waitOnRenderEnd = configRenderInvariantJson["wait on render end"].as( uf::renderer::settings::invariant::waitOnRenderEnd );
		uf::renderer::settings::invariant::individualPipelines = configRenderInvariantJson["individual pipelines"].as( uf::renderer::settings::invariant::individualPipelines );
	}
#if UF_USE_FFX_FSR || UF_USE_FFX_SDK
	ext::fsr::preset = configEngineExtFfxJson["preset"].as(ext::fsr::preset);
	ext::fsr::jitterScale = configEngineExtFfxJson["jitter scale"].as(ext::fsr::jitterScale);
	ext::fsr::sharpness = configEngineExtFfxJson["sharpness"].as(ext::fsr::sharpness);
	
	ext::fsr::frameUpscale = configEngineExtFfxJson["upscale"].as(ext::fsr::frameUpscale);
	ext::fsr::frameInterpolation = configEngineExtFfxJson["interpolation"].as(ext::fsr::frameInterpolation);
	
	if ( !configEngineExtFfxJson["enabled"].as(true) ) {
		ext::fsr::frameUpscale = false;
		ext::fsr::frameInterpolation = false;
	}
#endif

	// shouldn't ever fire because the deferred rendermode is owned by the scene now
	if ( uf::renderer::hasRenderMode("", true) ) {
		auto& renderMode = uf::renderer::getRenderMode("", true);
	#if UF_USE_FFX_FSR || UF_USE_FFX_SDK
		if ( uf::renderer::settings::pipelines::fsr ) {
			float factor = 1.0f;
			auto mode = uf::string::lowercase( ext::fsr::preset );
			if ( mode == "native" ) factor = 1.0f;
			else if ( mode == "quality" ) factor = 1.5f;
			else if ( mode == "balanced" ) factor = 1.7f;
			else if ( mode == "performance" ) factor = 2.0f;
			else if ( mode == "ultra" ) factor = 3.0f;
			else {
				UF_MSG_WARNING("Invalid FFX FSR preset enum string specified: {}", mode);
			}
			renderMode.scale = 1.0f / factor;
			UF_MSG_DEBUG("Using FFX FSR Preset: {} ({:.3f}% render scale)", mode, (100.0f / renderMode.scale));
		} else
	#endif
		renderMode.scale = configRenderJson["framebuffer"]["size"].as(1.0f);
		UF_MSG_DEBUG("Geometry render scale: {:.3f}", renderMode.scale);
	}
}

void UF_API uf::initialize() {
	/* Setup deferred Main thread */ {
		uf::thread::get(uf::thread::mainThreadName);
	}
	/* Setup non-blocking, asynchronous thread */ {
		uf::thread::get(uf::thread::asyncThreadName);
	}
	/* set JSON implicit preferences */ {
		ext::json::PREFERRED_ENCODING = uf::config["engine"]["ext"]["json"]["encoding"].as(ext::json::PREFERRED_ENCODING);
		ext::json::PREFERRED_COMPRESSION = uf::config["engine"]["ext"]["json"]["compression"].as(ext::json::PREFERRED_COMPRESSION);

		UF_MSG_DEBUG("Setting JSON implicit preference: {}.{}", ext::json::PREFERRED_ENCODING, ext::json::PREFERRED_COMPRESSION);
	}

	{
		uf::stl::string name = "window:Mouse.CursorVisibility";
		UF_MSG_DEBUG( "c_str={}, str={}", uf::algo::fnv1a("window:Mouse.CursorVisibility"), uf::algo::fnv1a(name) );
	}

	/* Arguments */ {
		bool modified = false;
		auto& arguments = uf::config["arguments"];
		for ( auto& arg : uf::arguments ) {
			// store raw argument
			int i = arguments.size();
			arguments[i] = arg;
			// parse --key=value
			auto match = uf::string::match( arg, "/^--(.+?)=(.+?)$/" );
			if ( !match.empty() ) {
				uf::stl::string keyString = match[1];
				uf::stl::string valueString = match[2];
				uf::Serializer value; value.deserialize(valueString);
				uf::config.path(keyString) = value;
				modified = true;
			}
		/*
			{
				std::regex regex("^--(.+?)=(.+?)$");
				std::smatch match;
				if ( std::regex_search( arg, match, regex ) ) {
					uf::stl::string keyString = match[1].str();
					uf::stl::string valueString = match[2].str();
					uf::Serializer value; value.deserialize(valueString);
					uf::config.path(keyString) = value;
					modified = true;
				}
			}
		*/
		}
	//	UF_MSG_DEBUG("Arguments: {}", uf::Serializer(arguments));
		if ( modified ) UF_MSG_DEBUG("New config: {}", uf::config.serialize());
	}
	/* Seed */ {
		std::srand(std::time(NULL));
	}
	/* Open output file */ {
		::io.filenames.output = uf::io::root+"/logs/output.txt";
		::io.output.open(::io.filenames.output);
	}
	/* Initialize timers */ {
		::times.sys.start();
	}
	/* Read persistent data */ {
		// #include "./inits/persistence.inl"
	}

	/* Set memory pool sizes */ {
		auto& configMemoryPoolJson = uf::config["engine"]["memory pool"];

		// check if we are even allowed to use memory pools
		bool enabled = configMemoryPoolJson["enabled"].as(true);
		auto deduceSize = [enabled]( const ext::json::Value& value )->size_t{
			if ( !enabled ) return 0;
			if ( value.is<size_t>() ) return value.as<size_t>();
			if ( value.is<uf::stl::string>() ) {
				uf::stl::string str = value.as<uf::stl::string>();
				std::regex regex("^(\\d+) ?((?:K|M|G)?(?:i?B)?)$");
				std::smatch match;
				if ( std::regex_search( str, match, regex ) ) {
					size_t requested = std::stoi( match[1].str() );
					uf::stl::string prefix = match[2].str();
					switch ( prefix.at(0) ) {
						case 'K': return requested * 1024;
						case 'M': return requested * 1024 * 1024;
						case 'G': return requested * 1024 * 1024 * 1024;
					}
					return requested;
				}
			}
			return 0;
		};
		// set memory pool alignment requirements
		uf::memoryPool::alignment = configMemoryPoolJson["alignment"].as( uf::memoryPool::alignment );
		// set memory pool sizes
		size_t size = deduceSize( configMemoryPoolJson["size"] );
		UF_MSG_DEBUG("Requesting {} bytes for global memory pool: {}", (size_t) size, (void*) &uf::memoryPool::global);
		uf::memoryPool::global.initialize( size );
		uf::memoryPool::subPool = configMemoryPoolJson["subPools"].as( uf::memoryPool::subPool );
		if ( size <= 0 || uf::memoryPool::subPool ) {
			{
				size_t size = deduceSize( configMemoryPoolJson["pools"]["component"] );
				uf::component::memoryPool.initialize( size );
				UF_MSG_DEBUG("Requested {} bytes for component memory pool: {}", (int) size, uf::component::memoryPool.data().memory);
			}
			{
				size_t size = deduceSize( configMemoryPoolJson["pools"]["userdata"] );
				uf::userdata::memoryPool.initialize( size );
				UF_MSG_DEBUG("Requested {} bytes for userdata memory pool: {}", (int) size, uf::userdata::memoryPool.data().memory);
			}
			{
				size_t size = deduceSize( configMemoryPoolJson["pools"]["entity"] );
				uf::Entity::memoryPool.initialize( size, pod::MemoryPool::Strategy::POOL, sizeof(uf::Entity) );
				UF_MSG_DEBUG("Requested {} bytes for entity memory pool: {}", (int) size, uf::Entity::memoryPool.data().memory);
			}
		}
		uf::allocator::override = configMemoryPoolJson["override"].as( uf::allocator::override );
	}

	/* Setup commands */ {
		uf::console::initialize();
	}

#if UF_USE_VALVE
	/* Load VPKs */ {
		auto& vpks = uf::config["engine"]["ext"]["valve"]["vpks"];
		ext::json::forEach( vpks, []( const uf::stl::string& uri ) {
			ext::valve::mountVpk( uri );
		});
	}
#endif

	/* Create initial scene (kludge) */ {
		uf::Scene& scene = uf::instantiator::instantiate<uf::Scene>();
		uf::scene::scenes.emplace_back(&scene);
		auto& metadata = scene.getComponent<uf::Serializer>();
	}

	uf::load( uf::config );

	// renderer settings
	{
	#if UF_USE_VULKAN
		auto& configRenderJson = uf::config["engine"]["ext"]["vulkan"];
	#elif UF_USE_OPENGL
		auto& configRenderJson = uf::config["engine"]["ext"]["opengl"];
	#else
		auto& configRenderJson = uf::config["engine"]["ext"]["software"];
	#endif
		auto& configRenderInvariantJson = configRenderJson["invariant"];
		auto& configRenderExperimentalJson = configRenderJson["experimental"];
		auto& configRenderPipelinesJson = configRenderJson["pipelines"];

		uf::renderer::settings::msaa = configRenderJson["framebuffer"]["msaa"].as( uf::renderer::settings::msaa );
		
		uf::renderer::settings::validation::enabled = configRenderJson["validation"]["enabled"].as( uf::renderer::settings::validation::enabled );

		if ( configRenderJson["framebuffer"]["size"].is<float>() ) {
		//	float scale = configRenderJson["framebuffer"]["size"].as(1.0f);
		//	uf::renderer::settings::width *= scale;
		//	uf::renderer::settings::height *= scale;
		} else if ( ext::json::isArray( configRenderJson["framebuffer"]["size"] ) ) {
			uf::renderer::settings::width = configRenderJson["framebuffer"]["size"][0].as(uf::renderer::settings::width);
			uf::renderer::settings::height = configRenderJson["framebuffer"]["size"][1].as(uf::renderer::settings::height);
			uf::stl::string filter = uf::string::lowercase( configRenderJson["framebuffer"]["size"][2].as<uf::stl::string>() );

			if ( filter == "nearest" ) uf::renderer::settings::swapchainUpscaleFilter = uf::renderer::enums::Filter::NEAREST;
			else if ( filter == "linear" ) uf::renderer::settings::swapchainUpscaleFilter = uf::renderer::enums::Filter::LINEAR;
		}

	#if UF_USE_VULKAN
		uf::renderer::settings::version = configRenderJson["version"].as<float>(1.3);

		if ( configRenderJson["gpu"].as<uf::stl::string>() == "auto" ) {
			uf::renderer::settings::gpuID = -1;
		} else {
			uf::renderer::settings::gpuID = configRenderJson["gpu"].as(uf::renderer::settings::gpuID);
		}
		for ( int i = 0; i < configRenderJson["validation"]["filters"].size(); ++i ) {
			uf::renderer::settings::validation::filters.emplace_back( configRenderJson["validation"]["filters"][i].as<uf::stl::string>() );
		}

		#define VK_LOAD_VERSION_LEVEL(VERSION) if ( VERSION <= uf::renderer::settings::version ) {\
			auto& configVersionLevel = configRenderJson["versions"][#VERSION];\
			for ( int i = 0; i < configVersionLevel["extensions"]["device"].size(); ++i ) {\
				uf::renderer::settings::requested::deviceExtensions.emplace_back( configVersionLevel["extensions"]["device"][i].as<uf::stl::string>() );\
			}\
			for ( int i = 0; i < configVersionLevel["extensions"]["instance"].size(); ++i ) {\
				uf::renderer::settings::requested::instanceExtensions.emplace_back( configVersionLevel["extensions"]["instance"][i].as<uf::stl::string>() );\
			}\
			for ( int i = 0; i < configVersionLevel["features"].size(); ++i ) {\
				uf::renderer::settings::requested::deviceFeatures.emplace_back( configVersionLevel["features"][i].as<uf::stl::string>() );\
			}\
			for ( int i = 0; i < configVersionLevel["featureChain"].size(); ++i ) {\
				uf::stl::string key = configVersionLevel["featureChain"][i].as<uf::stl::string>();\
				uf::renderer::settings::requested::featureChain[key] = true;\
			}\
		}

		VK_LOAD_VERSION_LEVEL(1.0);
		VK_LOAD_VERSION_LEVEL(1.1);
		VK_LOAD_VERSION_LEVEL(1.2);
		VK_LOAD_VERSION_LEVEL(1.3);

		uf::renderer::settings::invariant::deviceAddressing = uf::renderer::settings::requested::featureChain["physicalDeviceVulkan12"].as<bool>(false) || uf::renderer::settings::requested::featureChain["bufferDeviceAddress"].as<bool>(false);
		uf::renderer::settings::experimental::memoryBudgetBit = configRenderExperimentalJson["memory budget"].as( uf::renderer::settings::experimental::memoryBudgetBit );
		uf::renderer::settings::experimental::registerRenderMode = configRenderExperimentalJson["register render modes"].as( uf::renderer::settings::experimental::registerRenderMode );
	#endif

		uf::renderer::settings::experimental::rebuildOnTickBegin = configRenderExperimentalJson["rebuild on tick begin"].as( uf::renderer::settings::experimental::rebuildOnTickBegin );
		uf::renderer::settings::experimental::skipRenderOnRebuild = configRenderExperimentalJson["skip render on rebuild"].as( uf::renderer::settings::experimental::skipRenderOnRebuild );

		uf::renderer::settings::invariant::deferredMode = configRenderInvariantJson["deferred mode"].as( uf::renderer::settings::invariant::deferredMode );
	
		uf::renderer::settings::pipelines::vsync = configRenderPipelinesJson["vsync"].as( uf::renderer::settings::pipelines::vsync );
		uf::renderer::settings::pipelines::culling = configRenderPipelinesJson["culling"].as( uf::renderer::settings::pipelines::culling );

	#if UF_USE_VULKAN
		uf::renderer::settings::pipelines::deferred = configRenderPipelinesJson["deferred"].as( uf::renderer::settings::pipelines::deferred );
		uf::renderer::settings::pipelines::hdr = configRenderPipelinesJson["hdr"].as( uf::renderer::settings::pipelines::hdr );
		uf::renderer::settings::pipelines::vxgi = configRenderPipelinesJson["vxgi"].as( uf::renderer::settings::pipelines::vxgi );
		uf::renderer::settings::pipelines::bloom = configRenderPipelinesJson["bloom"].as( uf::renderer::settings::pipelines::bloom );
		uf::renderer::settings::pipelines::dof = configRenderPipelinesJson["dof"].as( uf::renderer::settings::pipelines::dof );
		uf::renderer::settings::pipelines::rt = configRenderPipelinesJson["rt"].as( uf::renderer::settings::pipelines::rt );
		uf::renderer::settings::pipelines::postProcess = configRenderPipelinesJson["postProcess"].as( uf::renderer::settings::pipelines::postProcess );
		
		if ( configRenderPipelinesJson["postProcess"].is<uf::stl::string>() ) {
			uf::renderer::settings::pipelines::postProcess = true;
		}

	#if UF_USE_FFX_FSR || UF_USE_FFX_SDK
		uf::renderer::settings::pipelines::fsr = configRenderPipelinesJson["fsr"].as( uf::renderer::settings::pipelines::fsr );
	#endif
	
		if ( uf::renderer::settings::pipelines::rt ) {
			uf::config["engine"]["scenes"]["lights"]["shadows"]["enabled"] = false;
		}
	#define JSON_TO_FORMAT( key ) if ( configRenderJson["formats"][#key].is<uf::stl::string>() ) {\
			uf::stl::string format = configRenderJson["formats"][#key].as<uf::stl::string>();\
			format = uf::string::replace( uf::string::uppercase(format), " ", "_" );\
			uf::renderer::settings::formats::key = uf::renderer::formatFromString( format );\
		}

		JSON_TO_FORMAT(color);
		JSON_TO_FORMAT(depth);
	#endif
	}

	/* Init controllers */ {
		spec::controller::initialize();
	}

#if UF_USE_LUA
	/* Lua */ {
		auto& configLuaJson = uf::config["engine"]["ext"]["lua"];

		ext::lua::main = uf::io::root + "/scripts/main.lua";

		ext::lua::enabled = configLuaJson["enabled"].as(ext::lua::enabled);
		ext::lua::main = configLuaJson["main"].as(ext::lua::main);
		ext::json::forEach( configLuaJson["modules"], []( const uf::stl::string& key, ext::json::Value& value ){
			ext::lua::modules[key] = value.as<uf::stl::string>();
		});
		ext::lua::initialize();
	}
#endif

#if UF_USE_OPENVR && UF_USE_VULKAN
	{	
		auto& configVrJson = uf::config["engine"]["ext"]["vr"];
		ext::openvr::enabled = configVrJson["enable"].as( ext::openvr::enabled );
		if ( ext::openvr::enabled && (ext::openvr::enabled = ext::openvr::initialize())) {
			ext::openvr::recommendedResolution( uf::renderer::settings::width, uf::renderer::settings::height );
			UF_MSG_DEBUG("VR Resolution: {}, {}", uf::renderer::settings::width, uf::renderer::settings::height); 
		}
			
		// could probably live alongside gui/deferred in the scene
		static auto* vrRenderMode = new uf::renderer::VrRenderMode;
		uf::renderer::addRenderMode(vrRenderMode, "VR");
	}
#endif

#if UF_USE_OPENAL
	/* Initialize OpenAL */ if ( !uf::audio::muted ) {
		ext::al::initialize();
	}
#endif

	/* Initialize Vulkan/OpenGL */ {
		uf::renderer::initialize();
	}

	pod::Thread& threadMain = uf::thread::get(uf::thread::mainThreadName);
#if UF_USE_FREETYPE
	{
		ext::freetype::initialize();
	}
#endif
#if UF_USE_DISCORD
	/* Discord */ if ( ::config.engine.ext.discord.enabled ) {
		ext::discord::initialize();
	}
#endif
#if UF_USE_ULTRALIGHT
	/* Ultralight-UX */ if ( ::config.engine.ext.ultralight.enabled ) {
		ext::ultralight::initialize();
	}
#endif
#if UF_USE_IMGUI
	if ( ::config.engine.ext.imgui.enabled ) {
	//	ext::imgui::initialize();
	}
#endif
#if 0 && UF_USE_VALL_E
	if ( ::config.engine.ext.vall_e.enabled ) {
		ext::vall_e::initialize( ::config.engine.ext.vall_e.model_path, ::config.engine.ext.vall_e.encodec_path );

		// bind the hook
		uf::hooks.addHook( "llm:VALL-E.synthesize", [&](ext::json::Value& json){
			auto text = json["text"].as<uf::stl::string>();
			auto prom = json["prom"].as<uf::stl::string>();
			auto play = json["play"].as<bool>();
			auto callback = json["callback"].as<uf::stl::string>("");

			uf::thread::queue( uf::thread::asyncThreadName, [=](){
				auto waveform = ext::vall_e::generate( text, prom );
				if ( callback != "" ) {
					uf::hooks.call( callback, waveform );
				}
				if ( play ) {
					uf::Audio audio;
					audio.load( waveform );
					audio.play();
				}
			});
		});
	}
#endif
	/* Add hooks */ {
		uf::hooks.addHook( "game:Scene.Load", [&](ext::json::Value& json){
			::sceneTransition.payload = json;
			::sceneTransition.phase = 0;
		});
		uf::hooks.addHook( "system:Quit", [&](ext::json::Value& json){
			if ( json["message"].is<uf::stl::string>() ) {
				UF_MSG_DEBUG( "{}", json["message"].as<uf::stl::string>() );
			}
			uf::ready = false;
		});
	}

	/* Initialize root scene*/ {
		ext::json::Value payload;
		payload["scene"] = uf::config["engine"]["scenes"]["start"];
		::sceneTransition.payload = payload;
		::sceneTransition.phase = 0;
	}
	
	uf::ready = true;
	UF_MSG_INFO("EXT took {} seconds to initialize", ::times.sys.elapsed().asDouble());
}

void UF_API uf::tick() {
	//++uf::time::frame;

	static pod::Thread& threadMain = uf::thread::get(uf::thread::mainThreadName);
#if UF_THREAD_METRICS
	auto activeStart = std::chrono::high_resolution_clock::now();
#endif

#if 1
	// skip the next tick to load the next scene to ensure nothing's happening
	if ( ::sceneTransition.phase >= 0 ) {
		auto target = ::sceneTransition.payload["scene"].as<uf::stl::string>();
		auto& phase = ::sceneTransition.phase;

		++phase;

	// might be necessary since i bluescreened with a dedicated thread
	#if UF_USE_VULKAN
		uf::renderer::flushCommandBuffers();
	#endif
		uf::renderer::synchronize();

		if ( uf::renderer::settings::experimental::dedicatedThread ) {
			::requestDedicatedRenderThread = true;
			uf::renderer::settings::experimental::dedicatedThread = !uf::renderer::settings::experimental::dedicatedThread;
		}
	#if UF_USE_VULKAN
		if ( !uf::renderer::settings::defaultCommandBufferImmediate ) {
			::requestDeferredCommandBufferSubmit = true;
			uf::renderer::settings::defaultCommandBufferImmediate = !uf::renderer::settings::defaultCommandBufferImmediate;
		}
	#endif

		uf::scene::unloadScene();
		uf::scene::loadScene( target );

		::sceneTransition.phase = -1;
	
	#if UF_USE_VULKAN
		uf::renderer::flushCommandBuffers();
	#endif
		uf::renderer::synchronize();

		return;
	}
#endif

	/* Tick controllers */ {
		spec::controller::tick();
	}
#if UF_USE_OPENVR
	/* OpenVR */ if ( ext::openvr::enabled ) {
		ext::openvr::tick();
	}
#endif
	/* Print Memory Pool Information */  {
		TIMER(1, uf::inputs::kbm::states::P ) {
			UF_MSG_DEBUG("==== Memory Pool Information ====");
			if ( uf::memoryPool::global.size() > 0 ) UF_MSG_DEBUG("Global Memory Pool: {}", uf::memoryPool::global.stats());
			if ( uf::Entity::memoryPool.size() > 0 ) UF_MSG_DEBUG("Entity Memory Pool: {}", uf::Entity::memoryPool.stats());
			if ( uf::component::memoryPool.size() > 0 ) UF_MSG_DEBUG("Components Memory Pool: {}", uf::component::memoryPool.stats());
			if ( uf::userdata::memoryPool.size() > 0 ) UF_MSG_DEBUG("Userdata Memory Pool: {}", uf::userdata::memoryPool.stats());
		}
	}
#if 0
	/* Attempt to reset VR position */  {
		TIMER(1, uf::inputs::kbm::states::Z ) {
			uf::hooks.call("VR:Seat.Reset");
		}
	}
	/* Print controller position */ if ( false ) {
		TIMER(1, uf::inputs::kbm::states::Z ) {
			auto& scene = uf::scene::getCurrentScene();
			auto& controller = scene.getController();
			auto& camera = controller.getComponent<uf::Camera>();
			auto& t = camera.getTransform(); //controller->getComponent<pod::Transform<>>();
			uf::iostream << "Viewport position: (" << t.position.x << ", " << t.position.y << ", " << t.position.z << ") (" << t.orientation.x << ", " << t.orientation.y << ", " << t.orientation.z << ", " << t.orientation.w << ")";
			uf::iostream << "\n";
			if ( false ) {
			//	uf::Entity* light = scene.findByUid(scene.loadChildUid("/light.json", true));
				uf::Object& light = scene.loadChild("/light.json", true);
				auto& lTransform = light.getComponent<pod::Transform<>>();
				auto& lMetadata = light.getComponent<uf::Serializer>();
				lTransform.position = t.position;
				lTransform.orientation = t.orientation;
				if ( !ext::json::isArray( lMetadata["light"] ) ) {
					lMetadata["light"]["color"][0] = (rand() % 100) / 100.0;
					lMetadata["light"]["color"][1] = (rand() % 100) / 100.0;
					lMetadata["light"]["color"][2] = (rand() % 100) / 100.0;
				}
				auto& sMetadata = scene.getComponent<uf::Serializer>();
				sMetadata["light"]["enabled"] = true;
			}
		}
	}
#endif
	/* Update physics timer */ {
		// to-do: add setting to either run in main thread or defer to a background thread
		uf::physics::tick();
	}
	/* Update entities */ {
		uf::scene::tick();
	}
	/* Update graph */ {
	//	uf::graph::tick();
	}

	/* Tick Main Thread Queue */ {
		uf::thread::process( threadMain );
	}
#if UF_USE_ULTRALIGHT
	/* Ultralight-UX */ if ( ::config.engine.ext.ultralight.enabled ) {
		ext::ultralight::tick();
	}
#endif
	/* Update vulkan */ {
		uf::renderer::tick();
	}

#if UF_USE_DISCORD
	/* Discord */ if ( ::config.engine.ext.discord.enabled ) {
		ext::discord::tick();
	}
#endif
#if UF_USE_IMGUI
	if ( ::config.engine.ext.imgui.enabled ) {
		ext::imgui::tick();
	}
#endif
	
	// perform GC on entities
	if ( ::config.engine.gc.enabled ) {
		TIMER( ::config.engine.gc.every ) {
			size_t collected = uf::instantiator::collect( ::config.engine.gc.mode );
			if ( collected > 0 ) {
				if ( ::config.engine.gc.announce ) UF_MSG_DEBUG("GC collected {} unused entities", (int) collected);
			}
		}
	}
#if UF_THREAD_METRICS
	// Mark the end of active work and the start of idle/sleep time
	auto idleStart = std::chrono::high_resolution_clock::now();
#endif

#if !UF_ENV_DREAMCAST
	if ( ::times.limiter > 0 ) {
		static auto nextFrameTime = std::chrono::steady_clock::now();
		
		double limiterMs = ::times.limiter * 1000.0;
		auto limiterDuration = std::chrono::duration_cast<std::chrono::steady_clock::duration>(std::chrono::duration<double, std::milli>(limiterMs));
		nextFrameTime += limiterDuration;

		auto now = std::chrono::steady_clock::now();
		if ( now < nextFrameTime ) {
			auto timeRemaining = nextFrameTime - now;
			if ( timeRemaining > std::chrono::milliseconds(2) ) std::this_thread::sleep_for(timeRemaining - std::chrono::milliseconds(1));
			while ( std::chrono::steady_clock::now() < nextFrameTime ) std::this_thread::yield();
		} else {
			nextFrameTime = std::chrono::steady_clock::now();
		}
	}

	auto& controller = uf::scene::getCurrentScene().getController();
	if ( ::requestDedicatedRenderThread && controller.getName() == "Player" ) {
		::requestDedicatedRenderThread = false;
		uf::renderer::settings::experimental::dedicatedThread = true;
		UF_MSG_DEBUG("Dedicated render requested");
	}
#if UF_USE_VULKAN
	if ( ::requestDeferredCommandBufferSubmit && controller.getName() == "Player" ) {
		::requestDeferredCommandBufferSubmit = false;
		uf::renderer::settings::defaultCommandBufferImmediate = false;
		UF_MSG_DEBUG("Defer command buffer submit requested");
	}
#endif
#endif

#if UF_THREAD_METRICS
	auto tickEnd = std::chrono::high_resolution_clock::now();

	std::chrono::duration<float, std::milli> activeTime = idleStart - activeStart;
	std::chrono::duration<float, std::milli> idleTime = tickEnd - idleStart;
	std::chrono::duration<float, std::milli> frameTime = tickEnd - activeStart;

	threadMain.metrics.activeTimeMs.store(activeTime.count(), std::memory_order_relaxed);
	threadMain.metrics.idleTimeMs.store(idleTime.count(), std::memory_order_relaxed);
	threadMain.metrics.totalFrameTimeMs.store(frameTime.count(), std::memory_order_relaxed);
#endif

	/* FPS Print */ if ( ::config.engine.fps.print ) {
		++::times.frames;
		++::times.total.frames;
		TIMER( ::config.engine.fps.every ) {
			UF_MSG_DEBUG("System: {:.3f} ms/frame | Time: {:.3f} | Frames: {} | FPS: {:.3f}", (time * 1000.0 / ::times.frames), time, ::times.frames, ::times.frames / time);
		#if UF_ENV_DREAMCAST
			DC_STATS();
		#endif
		#if 0 && UF_THREAD_METRICS
			auto metrics = uf::thread::collectStats();
			for ( auto& [ name, stats ] : metrics ) UF_MSG_DEBUG("Thread {}: active={}, idle={}, total={}, tasks={}", name, std::get<0>(stats), std::get<1>(stats), std::get<2>(stats), std::get<3>(stats) );
		#endif

			::times.frames = 0;
		}
	}
}
void UF_API uf::render() {
	if ( uf::scene::scenes.empty() ) return;

	if ( ::sceneTransition.phase >= 0 ) {
		return;
	}

#if UF_USE_ULTRALIGHT
	/* Ultralight-UX */ if ( ::config.engine.ext.ultralight.enabled ) {
		ext::ultralight::render();
	}
#endif
#if UF_USE_OPENVR
	{
		ext::openvr::synchronize();
	}
#endif
	/* Render scene */ {
	//	uf::renderer::tick();
		uf::renderer::render();
	}
}
void UF_API uf::terminate() {
	/* Kill threads */ {
		uf::thread::terminate();
	}
	/* Terminate controllers */ {
		spec::controller::terminate();
	}
#if UF_USE_VALL_E
	if ( ::config.engine.ext.vall_e.enabled ) {
		ext::vall_e::terminate();
	}
#endif
#if UF_USE_IMGUI
	if ( ::config.engine.ext.imgui.enabled ) {
		ext::imgui::terminate();
	}
#endif
#if UF_USE_ULTRALIGHT
	/* Ultralight-UX */ if ( ::config.engine.ext.ultralight.enabled ) {
		ext::ultralight::terminate();
	}
#endif
#if UF_USE_OPENVR
	/* OpenVR */ if ( ext::openvr::enabled ) {
		ext::openvr::terminate();
	}
#endif
	{
		uf::hooks.removeHooks();
	}
#if UF_USE_LUA
	{
		ext::lua::terminate();
	}
#endif
#if UF_USE_FREETYPE
	{
		ext::freetype::terminate();
	}
#endif
	{
		uf::graph::destroy();
	}
	{
		uf::scene::destroy();
	}
	/* Garbage collection */ if ( ::config.engine.gc.enabled ) {
		size_t collected = uf::instantiator::collect( ::config.engine.gc.mode );
		if ( collected > 0 ) {
			if ( ::config.engine.gc.announce ) UF_MSG_DEBUG("GC collected {} unused entities", (int) collected);
		}
	}

	/* Close vulkan */ {
	#if UF_USE_VULKAN
		uf::renderer::flushCommandBuffers();
	#endif
		uf::renderer::synchronize();

		uf::renderer::destroy();
	}

	#if UF_USE_OPENAL
		/* Initialize OpenAL */ if ( !uf::audio::muted ) {
			ext::al::destroy();
		}
	#endif

	/* Destroy memory pools */ {
		uf::component::memoryPool.destroy();
		uf::userdata::memoryPool.destroy();
		uf::Entity::memoryPool.destroy();

		uf::memoryPool::global.destroy(); // should probably leave this to be statically destructed
	}

	/* Print system stats */ {
		::times.total.time = ::times.sys.elapsed().asDouble();
		UF_MSG_DEBUG("System: Total Time: {} | Total Frames: {} | Average FPS: {}", ::times.total.time, ::times.total.frames, ::times.total.frames / ::times.total.time);
	}

	/* Flush input buffer */ {
		::io.output << ::io.input << "\n";
		for ( const auto& str : uf::iostream.getHistory() ) ::io.output << str << "\n";
		::io.output << "\nTerminated after " << ::times.sys.elapsed().asDouble() << " seconds" << "\n";
		::io.output.close();
	}
}