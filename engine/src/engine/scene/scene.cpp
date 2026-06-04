#include <uf/engine/scene/scene.h>
#include <uf/engine/scene/behavior.h>
#include <uf/engine/graph/graph.h>
#include <uf/utils/string/ext.h>
#include <uf/utils/camera/camera.h>
#include <uf/utils/math/physics.h>
#include <uf/utils/renderer/renderer.h>
#include <uf/utils/debug/draw.h>
#include <uf/utils/io/fmt.h>
#include <uf/engine/ext.h>
#include <regex>

UF_OBJECT_REGISTER_BEGIN(uf::Scene)
//	UF_OBJECT_REGISTER_BEHAVIOR(uf::EntityBehavior)
	UF_OBJECT_REGISTER_BEHAVIOR(uf::ObjectBehavior)
	UF_OBJECT_REGISTER_BEHAVIOR(uf::SceneBehavior)
UF_OBJECT_REGISTER_END()
uf::Scene::Scene() UF_BEHAVIOR_ENTITY_CPP_ATTACH(uf::Scene)

#define UF_SCENE_GLOBAL_GRAPH 1
#define UF_TICK_SINGLETHREAD_OVERRIDE 0
#define UF_TICK_MULTITHREAD_OVERRIDE 0
#define UF_SCENE_INVALIDATE_IMMEDIATE 1

#if UF_ENV_DREAMCAST
	#define UF_TICK_FROM_TASKS 0
#else
	#define UF_TICK_FROM_TASKS 1
#endif

#if UF_SCENE_GLOBAL_GRAPH
namespace {
	uf::SceneBehavior::Metadata metadata;
}
#endif

uf::Entity& uf::Scene::getController() {
#if !UF_SCENE_GLOBAL_GRAPH
	auto& metadata = this->getComponent<uf::SceneBehavior::Metadata>();
#endif
	auto& cache = metadata.cache.controllers;

	auto currentRenderMode = uf::renderer::getCurrentRenderMode();
	if ( currentRenderMode ) {
		auto& name = currentRenderMode->getName();
		if ( auto it = cache.find(name); it != cache.end() && it->second->isValid() ) {
			return *(it->second);
		}
		if ( name.rfind("RT:", 0) == 0 ) {
			auto uid = std::stoi(name.substr(3));
			if ( auto controller = this->findByUid(uid); controller && controller->isValid() ) {
				return *(cache[name] = controller);
			}
		}
	}
	if (auto it = cache.find(""); it != cache.end() && it->second->isValid()) {
		return *(it->second);
	}

	auto controller = this->findByName("Player");
	return *(cache[""] = (controller ? controller : this));
}
const uf::Entity& uf::Scene::getController() const {
	uf::Scene& scene = *const_cast<uf::Scene*>(this);
	return scene.getController();
}

uf::Camera& uf::Scene::getCamera( uf::Entity& controller ) {
	// ???
/*
	if ( auto currentRenderMode = uf::renderer::getCurrentRenderMode(); currentRenderMode && !currentRenderMode->getName().empty() ) {
		return controller.getComponent<uf::Camera>();
	}
*/
#if !UF_SCENE_GLOBAL_GRAPH
	auto& metadata = this->getComponent<uf::SceneBehavior::Metadata>();
#endif
	auto uid = controller.getUid();
	auto& cache = metadata.cache.cameras[uid];
	auto& cachedCamera = cache.camera;
	auto& lastFrame = cache.lastFrame;

	if ( lastFrame != uf::time::frame ) {
		auto& sourceCamera = controller.getComponent<uf::Camera>();
		if ( controller.getName() == "Player" ) sourceCamera.update();
		// copy all matrices
		for ( auto i = 0; i < uf::camera::maxViews; ++i ) {
			cachedCamera.setView(sourceCamera.getView(i), i);
			cachedCamera.setProjection(sourceCamera.getProjection(i), i);
		}
		// flatten the transform in the event the parent transform updates later
		cachedCamera.setTransform(uf::transform::flatten(sourceCamera.getTransform()));
		lastFrame = uf::time::frame;
	}
	return cachedCamera;
}

// ick
const uf::Camera& uf::Scene::getCamera( const uf::Entity& controller ) const {
	uf::Scene& scene = const_cast<uf::Scene&>(*this);
	uf::Entity& entity = const_cast<uf::Entity&>(controller);
	return scene.getCamera( entity );
}

void uf::Scene::invalidateGraph() {
#if !UF_SCENE_GLOBAL_GRAPH
	auto& metadata = this->getComponent<uf::SceneBehavior::Metadata>();
#endif
	metadata.invalidationQueued = true;
	metadata.cache.controllers.clear();
	metadata.cache.cameras.clear();
/*
	metadata.tasks.serial.clear();
	metadata.tasks.parallel.clear();

	metadata.graph.clear();
*/
}
const uf::stl::vector<uf::Entity*>& uf::Scene::getGraph() {
#if !UF_SCENE_GLOBAL_GRAPH
	auto& metadata = this->getComponent<uf::SceneBehavior::Metadata>();
#endif
#if UF_SCENE_INVALIDATE_IMMEDIATE
	if ( metadata.invalidationQueued ) {
		metadata.invalidationQueued = false;
		metadata.cache.controllers.clear();
		metadata.graph.clear();
		metadata.tasks.serial.clear();
		metadata.tasks.parallel.clear();
	}
#endif
	if ( !metadata.graph.empty() ) return metadata.graph;

	metadata.tasks.serial = uf::thread::schedule(false);
	metadata.tasks.parallel = uf::thread::schedule(true, false);

	this->process([&]( uf::Entity* entity ) {
		if ( !entity->hasComponent<uf::ObjectBehavior::Metadata>() ) return;
		auto& eMetadata = entity->getComponent<uf::ObjectBehavior::Metadata>();
		if ( eMetadata.system.ignoreGraph ) return;

		metadata.graph.emplace_back(entity);

	#if UF_TICK_FROM_TASKS
		auto* self = (uf::Object*) entity;
		auto/*&*/ behaviorGraph = entity->getGraph();

		#if UF_TICK_MULTITHREAD_OVERRIDE
			for ( auto fun : behaviorGraph.tick.serial ) metadata.tasks.parallel.queue([=]{ fun(*self); });
			for ( auto fun : behaviorGraph.tick.parallel ) metadata.tasks.parallel.queue([=]{ fun(*self); });
		#elif UF_TICK_SINGLETHREAD_OVERRIDE
			for ( auto fun : behaviorGraph.tick.serial ) metadata.tasks.serial.queue([=]{ fun(*self); });
			for ( auto fun : behaviorGraph.tick.parallel ) metadata.tasks.serial.queue([=]{ fun(*self); });
		#else
			for ( auto fun : behaviorGraph.tick.serial ) metadata.tasks.serial.queue([=]{ fun(*self); });
			for ( auto fun : behaviorGraph.tick.parallel ) metadata.tasks.parallel.queue([=]{ fun(*self); });
		#endif
	#endif
	});

	uf::renderer::states::rebuild = true;
	return metadata.graph;
}
uf::stl::vector<uf::Entity*> uf::Scene::getGraph( bool reverse ) {
	auto/*&*/ graph = this->getGraph();
	if ( reverse ) std::reverse( graph.begin(), graph.end() );
	return graph;
}

uf::stl::vector<uf::Scene*> uf::scene::scenes;
bool uf::scene::printTaskCalls = false;

uf::Scene& uf::scene::loadScene( const uf::stl::string& name, const uf::stl::string& _filename ) {
	uf::Scene* scene = uf::instantiator::objects->has( name ) ? (uf::Scene*) &uf::instantiator::instantiate( name ) : new uf::Scene;
	uf::scene::scenes.emplace_back( scene );

#if UF_USE_FMT
	uf::stl::string filename = _filename;
	if ( _filename == "" ) {
		filename = ::fmt::format("/{}/scene.json", uf::string::lowercase(name));
	}
#else
	const uf::stl::string filename = _filename != "" ? _filename : (uf::stl::string("/") + uf::string::lowercase(name) + "/scene.json");
#endif
	scene->load(filename);

	auto& metadataJson = scene->getComponent<uf::Serializer>();
	metadataJson["system"]["scene"] = name;
#if UF_USE_VULKAN
	if ( uf::renderer::settings::pipelines::rt ) uf::instantiator::bind( "RayTraceSceneBehavior", *scene );
	if ( uf::renderer::settings::pipelines::vxgi ) uf::instantiator::bind( "VoxelizerSceneBehavior", *scene );
#endif
	scene->initialize();

	return *scene;
}
uf::Scene& uf::scene::loadScene( const uf::stl::string& name, const uf::Serializer& data ) {
	uf::Scene* scene = uf::instantiator::objects->has( name ) ? (uf::Scene*) &uf::instantiator::instantiate( name ) : new uf::Scene;
	uf::scene::scenes.emplace_back( scene );
	if ( data != "" ) scene->load(data);

	auto& metadataJson = scene->getComponent<uf::Serializer>();
	metadataJson["system"]["scene"] = name;
#if UF_USE_VULKAN
	if ( uf::renderer::settings::pipelines::rt ) uf::instantiator::bind( "RayTraceSceneBehavior", *scene );
	if ( uf::renderer::settings::pipelines::vxgi ) uf::instantiator::bind( "VoxelizerSceneBehavior", *scene );
#endif
	scene->initialize();

	return *scene;
}
void uf::scene::unloadScene() {
	uf::Scene* current = uf::scene::scenes.back();
	current->queueDeletion();
	
	// destroy graph
	if ( current->hasComponent<pod::Graph::Storage>() ) {
		uf::graph::destroy( current->getComponent<pod::Graph::Storage>() );
	}
	// destroy physics state
	if ( current->hasComponent<pod::World>() ) {
		uf::physics::destroy( *current );
	}

	// mark rendermodes as disabled immediately
	auto graph = current->getGraph(true);
	for ( auto entity : graph ) {
		if ( entity->hasComponent<uf::renderer::RenderTargetRenderMode>() ) {
			auto& renderMode = entity->getComponent<uf::renderer::RenderTargetRenderMode>();
			auto& blitter = renderMode.getBlitter();
			renderMode.execute = false;
			blitter.process = false;
		}
		if ( entity->hasComponent<uf::renderer::DeferredRenderMode>() ) {
			auto& renderMode = entity->getComponent<uf::renderer::DeferredRenderMode>();
			auto& blitter = renderMode.getBlitter();
			renderMode.execute = false;
			blitter.process = false;
		}
	}

	uf::renderer::states::rebuild = true;
	uf::renderer::states::resized = true;
	
	uf::scene::scenes.pop_back();
}
uf::Scene& uf::scene::getCurrentScene() {
	if ( uf::scene::scenes.empty() ) {
		return uf::Entity::null.as<uf::Scene>();
	}

	return *uf::scene::scenes.back();
}
void uf::scene::invalidateGraphs() {
	for ( auto scene : uf::scene::scenes ) {
		if ( !scene ) continue;
		scene->invalidateGraph();
	}
}

void uf::scene::tick() {
	if ( scenes.empty() ) return;

#if !UF_SCENE_INVALIDATE_IMMEDIATE
	if ( metadata.invalidationQueued ) {
		metadata.invalidationQueued = false;
		metadata.graph.clear();
		metadata.tasks.serial.clear();
		metadata.tasks.parallel.clear();
	}
#endif

	auto& scene = uf::scene::getCurrentScene();
	auto/*&*/ graph = scene.getGraph(true);
	
//	uf::physics::tick( scene );

#if !UF_SCENE_GLOBAL_GRAPH
	auto& metadata = scene.getComponent<uf::SceneBehavior::Metadata>();
#endif

#if UF_TICK_FROM_TASKS
	// only dispatch if we have tasks that requested parallelization
	if ( !metadata.tasks.parallel.empty() ) {
		// copy because executing from the tasks erases them all
		auto tasks = metadata.tasks;
		auto workers = uf::thread::execute( tasks.parallel );
		uf::thread::execute( tasks.serial );
		uf::thread::wait( workers );
	} else
#endif
	for ( auto entity : graph ) entity->tick();
	uf::graph::tick();

	uf::debug::draw( uf::time::delta );
}
void uf::scene::render() {
	if ( scenes.empty() ) return;
	auto& scene = uf::scene::getCurrentScene();
	auto/*&*/ graph = scene.getGraph(true);
	for ( auto entity : graph ) entity->render();
	uf::graph::render();
}
void uf::scene::destroy() {
	while ( !scenes.empty() ) unloadScene();
}