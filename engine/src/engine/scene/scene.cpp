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
uf::Entity& uf::Scene::getController() {
	static uf::Entity* cachedController = NULL;
	if ( uf::renderer::currentRenderMode ) {
		static uf::renderer::RenderMode* cachedRenderMode = NULL;
		auto& renderMode = *uf::renderer::currentRenderMode;

		if ( cachedRenderMode == &renderMode && cachedController && cachedController->getUid() > 0 ) {
			return *cachedController;
		}
		cachedController = NULL;
		cachedRenderMode = &renderMode;
		auto split = uf::string::split( renderMode.getName(), ":" );
		if ( split.front() == "RT" ) {
			uint64_t uid = std::stoi( split.back() );
			uf::Entity* ent = this->findByUid( uid );
			if ( ent ) return *(cachedController = ent);
		}
	}
	if ( cachedController && cachedController->getUid() > 0 ) return *cachedController;
	cachedController = this->findByName("Player");
	return cachedController ? *cachedController : *this;
	// return this;
}
const uf::Entity& uf::Scene::getController() const {
	uf::Scene& scene = *const_cast<uf::Scene*>(this);
	return scene.getController();
}
void uf::Scene::invalidateGraph() {
	auto& metadata = this->getComponent<uf::SceneBehavior::Metadata>();
	metadata.invalidationQueued = true;
}
const std::vector<uf::Entity*>& uf::Scene::getGraph() {
	auto& metadata = this->getComponent<uf::SceneBehavior::Metadata>();
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
std::vector<uf::Entity*> uf::Scene::getGraph( bool reverse ) {
	auto graph = this->getGraph();
	if ( reverse ) std::reverse( graph.begin(), graph.end() );
	return graph;
}

std::vector<uf::Scene*> uf::scene::scenes;
uf::Scene& uf::scene::loadScene( const std::string& name, const std::string& _filename ) {
	uf::Scene* scene = uf::instantiator::objects->has( name ) ? (uf::Scene*) &uf::instantiator::instantiate( name ) : new uf::Scene;
	uf::scene::scenes.emplace_back( scene );

	const std::string filename = _filename != "" ? _filename : ("/" + uf::string::lowercase(name) + "/scene.json");
	scene->load(filename);
	if ( uf::renderer::settings::experimental::deferredMode == "vxgi" ) {
		uf::instantiator::bind( "VoxelizerBehavior", *scene );
	}
	scene->initialize();
	return *scene;
}
uf::Scene& uf::scene::loadScene( const std::string& name, const uf::Serializer& data ) {
	uf::Scene* scene = uf::instantiator::objects->has( name ) ? (uf::Scene*) &uf::instantiator::instantiate( name ) : new uf::Scene;
	uf::scene::scenes.emplace_back( scene );
	if ( data != "" ) scene->load(data);
	if ( uf::renderer::settings::experimental::deferredMode == "vxgi" ) {
		uf::instantiator::bind( "VoxelizerBehavior", *scene );
	}
	scene->initialize();
	return *scene;
}
void uf::scene::unloadScene() {
	uf::Scene* current = uf::scene::scenes.back();
	current->destroy();
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