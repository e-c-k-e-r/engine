#include <uf/engine/scene/scene.h>
#include <uf/engine/scene/behavior.h>
#include <uf/engine/graph/graph.h>
#include <uf/utils/string/ext.h>
#include <uf/utils/camera/camera.h>
#include <uf/utils/math/physics.h>
#include <uf/utils/renderer/renderer.h>
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
#define UF_TICK_FROM_TASKS 1
#define UF_SCENE_INVALIDATE_IMMEDIATE 1

#if UF_SCENE_GLOBAL_GRAPH
namespace {
	uf::SceneBehavior::Metadata metadata;
}
#endif

uf::Entity& uf::Scene::getController() {
#if !UF_SCENE_GLOBAL_GRAPH
	auto& metadata = this->getComponent<uf::SceneBehavior::Metadata>();
#endif
	auto currentRenderMode = uf::renderer::getCurrentRenderMode();
	uf::Entity* controller = NULL;
	if ( currentRenderMode ) {
		auto& renderMode = *currentRenderMode;
		auto& name = renderMode.getName();
		if ( metadata.cache.controllers.count(name) > 0 ) {
			controller = metadata.cache.controllers[name];
			if ( controller->isValid() ) return *controller;
		}

		auto split = uf::string::split( name, ":" );
		if ( split.front() == "RT" ) {
			controller = this->findByUid( std::stoi( split.back() ) );
			if ( controller->isValid() ) return *(metadata.cache.controllers[name] = controller);
		}
	}
	if ( metadata.cache.controllers.count("") > 0 ) {
		controller = metadata.cache.controllers[""];
		if ( controller->isValid() ) return *controller;
	}
	controller = this->findByName("Player");
	return *(metadata.cache.controllers[""] = controller ? controller : this);
}
const uf::Entity& uf::Scene::getController() const {
	uf::Scene& scene = *const_cast<uf::Scene*>(this);
	return scene.getController();
}

uf::Camera& uf::Scene::getCamera( uf::Entity& controller ) {
	auto currentRenderMode = uf::renderer::getCurrentRenderMode();
	if ( currentRenderMode && currentRenderMode->getName() != "" ) return controller.getComponent<uf::Camera>();
#if !UF_SCENE_GLOBAL_GRAPH
	auto& metadata = this->getComponent<uf::SceneBehavior::Metadata>();
#endif
	auto& cachedCamera = metadata.cache.cameras[controller.getUid()];

	if ( metadata.cache.frameNumbers[controller.getUid()] != uf::time::frame ) {
		metadata.cache.frameNumbers[controller.getUid()] = uf::time::frame;

		auto& camera = controller.getComponent<uf::Camera>();
	//	cachedCamera = camera;
		cachedCamera.setTransform( uf::transform::flatten( camera.getTransform() ) );
		for ( auto i = 0; i < uf::camera::maxViews; ++i ) {
			cachedCamera.setView( camera.getView(i), i );
			cachedCamera.setProjection( camera.getProjection(i), i );
		}
		cachedCamera.update(true);

//		UF_MSG_DEBUG("New frame time: {}: {} ({})", uf::time::frame, controller.getName(), controller.getUid());
	}

	return cachedCamera;
}

const uf::Camera& uf::Scene::getCamera( const uf::Entity& controller ) const {
	auto currentRenderMode = uf::renderer::getCurrentRenderMode();
	if ( currentRenderMode && currentRenderMode->getName() != "" ) return controller.getComponent<uf::Camera>();
#if !UF_SCENE_GLOBAL_GRAPH
	auto& metadata = this->getComponent<uf::SceneBehavior::Metadata>();
#endif
	auto& cachedCamera = metadata.cache.cameras[controller.getUid()];

	if ( metadata.cache.frameNumbers[controller.getUid()] != uf::time::frame ) {
		metadata.cache.frameNumbers[controller.getUid()] = uf::time::frame;

		auto& camera = controller.getComponent<uf::Camera>();
	//	cachedCamera = camera;
		cachedCamera.setTransform( uf::transform::flatten( camera.getTransform() ) );
		for ( auto i = 0; i < uf::camera::maxViews; ++i ) {
			cachedCamera.setView( camera.getView(i), i );
			cachedCamera.setProjection( camera.getProjection(i), i );
		}
		cachedCamera.update(true);

//		UF_MSG_DEBUG("New frame time: {}: {} ({})", uf::time::frame, controller.getName(), controller.getUid());
	}

	return cachedCamera;
}



void uf::Scene::invalidateGraph() {
#if !UF_SCENE_GLOBAL_GRAPH
	auto& metadata = this->getComponent<uf::SceneBehavior::Metadata>();
#endif
	metadata.invalidationQueued = true;
/*
	metadata.cache.controllers.clear();
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
		if ( uf::scene::printTaskCalls ) {
			for ( auto& behavior : self->getBehaviors() ) {
				if ( !behavior.traits.ticks ) continue;

				auto name = behavior.type.name().str();
			#if UF_TICK_MULTITHREAD_OVERRIDE
				if ( true )
			#elif UF_TICK_SINGLETHREAD_OVERRIDE
				if ( false )
			#else
				if ( behavior.traits.multithread )
			#endif
				metadata.tasks.parallel.queue([=]{
					UF_MSG_DEBUG("Parallel {} task executing: {}: {}", name, self->getName(), self->getUid());
					behavior.tick(*self);
					UF_MSG_DEBUG("Parallel {} task exectued: {}: {}", name, self->getName(), self->getUid());
				}); else metadata.tasks.serial.queue([=]{
					UF_MSG_DEBUG("Serial {} task executing: {}: {}", name, self->getName(), self->getUid());
					behavior.tick(*self);
					UF_MSG_DEBUG("Serial {} task exectued: {}: {}", name, self->getName(), self->getUid());
				});
			}
		} else {
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

		}	
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

//	uf::physics::initialize();
//	uf::graph::initialize();
//	uf::renderer::states::renderSkip = false;
//	uf::renderer::states::frameSkip = 60;

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

//	uf::physics::initialize();
//	uf::graph::initialize();
//	uf::renderer::states::renderSkip = false;
//	uf::renderer::states::frameSkip = 60;

	return *scene;
}
void uf::scene::unloadScene() {
//	uf::renderer::states::frameSkip = 60;

	uf::Scene* current = uf::scene::scenes.back();
//	current->destroy();
	current->queueDeletion();
	
//	uf::physics::terminate();
//	uf::graph::destroy();
/*
	auto graph = current->getGraph(true);
	for ( auto entity : graph ) entity->destroy();
*/
	// destroy phyiscs state
	if ( current->hasComponent<pod::Graph::Storage>() ) {
		uf::graph::destroy( current->getComponent<pod::Graph::Storage>() );
	}
	if ( current->hasComponent<uf::physics::impl::WorldState>() ) {
		uf::physics::impl::destroy( *current );
	}

	// mark rendermodes as disabled immediately
	auto graph = current->getGraph(true);
	for ( auto entity : graph ) {
		if ( entity->hasComponent<uf::renderer::RenderTargetRenderMode>() ) {
			auto& renderMode = entity->getComponent<uf::renderer::RenderTargetRenderMode>();
			auto& blitter = renderMode.getBlitter();
			renderMode.execute = false;
			blitter.process = false;
		/*
			if ( uf::renderer::settings::experimental::registerRenderMode ) {
				uf::renderer::removeRenderMode( &renderMode, false );
			}
		*/
		}
		if ( entity->hasComponent<uf::renderer::DeferredRenderMode>() ) {
			auto& renderMode = entity->getComponent<uf::renderer::DeferredRenderMode>();
			auto& blitter = renderMode.getBlitter();
			renderMode.execute = false;
			blitter.process = false;
		/*
			if ( uf::renderer::settings::experimental::registerRenderMode ) {
				uf::renderer::removeRenderMode( &renderMode, false );
			}
		*/
		}
	}

	uf::renderer::states::rebuild = true;
	uf::renderer::states::resized = true;

	/*
	if ( rebuilded ) {
		// rebuild swapchain
		if ( uf::renderer::hasRenderMode("Swapchain", true) ) {
			auto& renderMode = uf::renderer::getRenderMode("Swapchain", true);
			renderMode.execute = false;
			renderMode.rerecord = true;
			renderMode.synchronize();

			renderMode.destroy();
			renderMode.initialize( uf::renderer::device );
			if ( uf::renderer::settings::invariant::individualPipelines ) renderMode.bindPipelines();
			renderMode.createCommandBuffers();
			renderMode.tick();
		}
	}
	*/
	
	uf::scene::scenes.pop_back();
}
uf::Scene& uf::scene::getCurrentScene() {
	//UF_ASSERT( !uf::scene::scenes.empty() );
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

	if ( uf::scene::printTaskCalls ) UF_MSG_DEBUG("Scene tick start");

	auto& scene = uf::scene::getCurrentScene();
	
	uf::physics::tick( scene );

#if !UF_SCENE_GLOBAL_GRAPH
	auto& metadata = scene.getComponent<uf::SceneBehavior::Metadata>();
#endif
	auto/*&*/ graph = scene.getGraph(true);
#if UF_TICK_FROM_TASKS
	// copy because executing from the tasks erases them all
	auto tasks = metadata.tasks;
	auto workers = uf::thread::execute( tasks.parallel );
	uf::thread::execute( tasks.serial );

	uf::thread::wait( workers );
#else
	for ( auto entity : graph ) entity->tick();
#endif
	
	uf::graph::tick( scene );
	if ( uf::scene::printTaskCalls ) UF_MSG_DEBUG("Scene tick end");
}
void uf::scene::render() {
	if ( scenes.empty() ) return;
	auto& scene = uf::scene::getCurrentScene();
	auto/*&*/ graph = scene.getGraph(true);
	for ( auto entity : graph ) entity->render();
	uf::graph::render( scene );
}
void uf::scene::destroy() {
	while ( !scenes.empty() ) unloadScene();
}