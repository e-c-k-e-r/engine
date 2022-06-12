#include <uf/engine/scene/scene.h>
#include <uf/engine/scene/behavior.h>
#include <uf/utils/string/ext.h>
#include <uf/utils/camera/camera.h>
#include <uf/utils/renderer/renderer.h>
#include <regex>

UF_OBJECT_REGISTER_BEGIN(uf::Scene)
	UF_OBJECT_REGISTER_BEHAVIOR(uf::EntityBehavior)
	UF_OBJECT_REGISTER_BEHAVIOR(uf::ObjectBehavior)
	UF_OBJECT_REGISTER_BEHAVIOR(uf::SceneBehavior)
UF_OBJECT_REGISTER_END()
uf::Scene::Scene() UF_BEHAVIOR_ENTITY_CPP_ATTACH(uf::Scene)

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
		if ( metadata.cache.count(name) > 0 ) {
			controller = metadata.cache[name];
			if ( controller->isValid() ) return *controller;
		}

		auto split = uf::string::split( name, ":" );
		if ( split.front() == "RT" ) {
			controller = this->findByUid( std::stoi( split.back() ) );
			if ( controller->isValid() ) return *(metadata.cache[name] = controller);
		}
	}
	if ( metadata.cache.count("") > 0 ) {
		controller = metadata.cache[""];
		if ( controller->isValid() ) return *controller;
	}
	controller = this->findByName("Player");
	return *(metadata.cache[""] = controller ? controller : this);
}
const uf::Entity& uf::Scene::getController() const {
	uf::Scene& scene = *const_cast<uf::Scene*>(this);
	return scene.getController();
}



void uf::Scene::invalidateGraph() {
#if !UF_SCENE_GLOBAL_GRAPH
	auto& metadata = this->getComponent<uf::SceneBehavior::Metadata>();
#endif
	metadata.invalidationQueued = true;
#if 1
	metadata.cache.clear();
	metadata.tasks.clear();
#else
	metadata.cached.renderMode = NULL;
	metadata.cached.controller = NULL;
#endif
}
const uf::stl::vector<uf::Entity*>& uf::Scene::getGraph() {
#if !UF_SCENE_GLOBAL_GRAPH
	auto& metadata = this->getComponent<uf::SceneBehavior::Metadata>();
#endif
	if ( metadata.invalidationQueued ) {
		metadata.invalidationQueued = false;
		metadata.graph.clear();
		metadata.tasks.clear();
	}
	if ( !metadata.graph.empty() ) return metadata.graph;
	this->process([&]( uf::Entity* entity ) {
		if ( !entity->hasComponent<uf::ObjectBehavior::Metadata>() ) return;
		auto& eMetadata = entity->getComponent<uf::ObjectBehavior::Metadata>();
		if ( !eMetadata.system.ignoreGraph ) {
			metadata.graph.emplace_back(entity);

			auto& behaviorGraph = entity->getGraph();
			for ( auto& fun : behaviorGraph.tickMT ) metadata.tasks.queue(fun);
		}
	});
	uf::renderer::states::rebuild = true;
	return metadata.graph;
}
uf::stl::vector<uf::Entity*> uf::Scene::getGraph( bool reverse ) {
	auto graph = this->getGraph();
	if ( reverse ) std::reverse( graph.begin(), graph.end() );
	return graph;
}

uf::stl::vector<uf::Scene*> uf::scene::scenes;
uf::Scene& uf::scene::loadScene( const uf::stl::string& name, const uf::stl::string& _filename ) {
	uf::Scene* scene = uf::instantiator::objects->has( name ) ? (uf::Scene*) &uf::instantiator::instantiate( name ) : new uf::Scene;
	uf::scene::scenes.emplace_back( scene );

	const uf::stl::string filename = _filename != "" ? _filename : ("/" + uf::string::lowercase(name) + "/scene.json");
	scene->load(filename);
	if ( uf::renderer::settings::pipelines::vxgi ) {
		uf::instantiator::bind( "VoxelizerBehavior", *scene );
	}
	scene->initialize();
	return *scene;
}
uf::Scene& uf::scene::loadScene( const uf::stl::string& name, const uf::Serializer& data ) {
	uf::Scene* scene = uf::instantiator::objects->has( name ) ? (uf::Scene*) &uf::instantiator::instantiate( name ) : new uf::Scene;
	uf::scene::scenes.emplace_back( scene );
	if ( data != "" ) scene->load(data);
	if ( uf::renderer::settings::pipelines::vxgi ) {
		uf::instantiator::bind( "VoxelizerBehavior", *scene );
	}
	scene->initialize();
	return *scene;
}
void uf::scene::unloadScene() {
	uf::Scene* current = uf::scene::scenes.back();
//	current->destroy();
	auto graph = current->getGraph(true);
	for ( auto entity : graph ) entity->destroy();
	uf::scene::scenes.pop_back();
}
uf::Scene& uf::scene::getCurrentScene() {
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
	auto& scene = uf::scene::getCurrentScene();
#if !UF_SCENE_GLOBAL_GRAPH
	auto& metadata = scene.getComponent<uf::SceneBehavior::Metadata>();
#endif
	auto graph = scene.getGraph(true);
#if 1
	for ( auto entity : graph ) entity->tick();
	auto& tasks = metadata.tasks;
#else
	pod::Thread::Tasks tasks = metadata.tasks;
	tasks.queue([&]{
		for ( auto entity : graph ) entity->tick();
	});
#endif
	uf::thread::execute( tasks );
}
void uf::scene::render() {
	if ( scenes.empty() ) return;
	auto graph = uf::scene::getCurrentScene().getGraph(true);
	for ( auto entity : graph ) entity->render();
}
void uf::scene::destroy() {
	while ( !scenes.empty() ) unloadScene();
}