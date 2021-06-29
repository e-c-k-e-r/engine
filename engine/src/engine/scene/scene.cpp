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

namespace {
	uf::SceneBehavior::Metadata metadata;
}

uf::Entity& uf::Scene::getController() {
//	auto& metadata = this->getComponent<uf::SceneBehavior::Metadata>();
	if ( uf::renderer::currentRenderMode ) {
		auto& renderMode = *uf::renderer::currentRenderMode;

		if ( metadata.cached.renderMode == &renderMode && metadata.cached.controller && metadata.cached.controller->isValid() ) {
			return *metadata.cached.controller;
		}
		metadata.cached.controller = NULL;
		metadata.cached.renderMode = &renderMode;
		auto split = uf::string::split( renderMode.getName(), ":" );
		if ( split.front() == "RT" ) {
			uint64_t uid = std::stoi( split.back() );
			uf::Entity* ent = this->findByUid( uid );
			if ( ent ) return *(metadata.cached.controller = ent);
		}
	}
	if ( metadata.cached.controller && metadata.cached.controller->isValid() ) return *metadata.cached.controller;
	metadata.cached.controller = this->findByName("Player");
	return metadata.cached.controller ? *metadata.cached.controller : *this;
	// return this;
}
const uf::Entity& uf::Scene::getController() const {
	uf::Scene& scene = *const_cast<uf::Scene*>(this);
	return scene.getController();
}



void uf::Scene::invalidateGraph() {
//	auto& metadata = this->getComponent<uf::SceneBehavior::Metadata>();
	metadata.invalidationQueued = true;
	metadata.cached.renderMode = NULL;
	metadata.cached.controller = NULL;
}
const uf::stl::vector<uf::Entity*>& uf::Scene::getGraph() {
//	auto& metadata = this->getComponent<uf::SceneBehavior::Metadata>();
	if ( metadata.invalidationQueued ) {
		metadata.invalidationQueued = false;
		metadata.graph.clear();
	}
	if ( !metadata.graph.empty() ) return metadata.graph;
	this->process([&]( uf::Entity* entity ) {
		if ( !entity->getComponent<uf::ObjectBehavior::Metadata>().system.ignoreGraph ) metadata.graph.emplace_back(entity);
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
	if ( uf::renderer::settings::experimental::vxgi ) {
		uf::instantiator::bind( "VoxelizerBehavior", *scene );
	}
	scene->initialize();
	return *scene;
}
uf::Scene& uf::scene::loadScene( const uf::stl::string& name, const uf::Serializer& data ) {
	uf::Scene* scene = uf::instantiator::objects->has( name ) ? (uf::Scene*) &uf::instantiator::instantiate( name ) : new uf::Scene;
	uf::scene::scenes.emplace_back( scene );
	if ( data != "" ) scene->load(data);
	if ( uf::renderer::settings::experimental::vxgi ) {
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
	auto graph = uf::scene::getCurrentScene().getGraph(true);
	for ( auto entity : graph ) entity->tick();
}
void uf::scene::render() {
	if ( scenes.empty() ) return;
	auto graph = uf::scene::getCurrentScene().getGraph(true);
	for ( auto entity : graph ) entity->render();
}
void uf::scene::destroy() {
	while ( !scenes.empty() ) unloadScene();
}