#include <uf/engine/scene/scene.h>
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

std::vector<uf::Scene*> uf::scene::scenes;
std::vector<uf::Entity*> uf::scene::graph;
bool uf::scene::queuedInvalidation = false;
bool uf::scene::useGraph = true;

uf::Scene& uf::scene::loadScene( const std::string& name, const std::string& filename ) {
	std::string target = name;
	uf::Scene* scene = uf::instantiator::objects->has( name ) ? (uf::Scene*) &uf::instantiator::instantiate( name ) : new uf::Scene;
	uf::scene::scenes.emplace_back( scene );
/*
	std::regex regex("^(TestScene_?)?(.+?)(_?Scene)?$");
	std::smatch match;
	if ( std::regex_search( target, match, regex ) ) target = match[2];
*/
	target = uf::string::lowercase( target );
	scene->load(filename != "" ? filename : "./scenes/" + target + "/scene.json");
	scene->initialize();
	return *scene;
}
uf::Scene& uf::scene::loadScene( const std::string& name, const uf::Serializer& data ) {
	uf::Scene* scene = uf::instantiator::objects->has( name ) ? (uf::Scene*) &uf::instantiator::instantiate( name ) : new uf::Scene;
	uf::scene::scenes.emplace_back( scene );
	if ( data != "" ) scene->load(data);
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
void uf::scene::invalidateGraph() {
	uf::scene::queuedInvalidation = true;
}
std::vector<uf::Entity*> uf::scene::generateGraph() {
	// invalidate it by clearing the graph
	if ( uf::scene::queuedInvalidation ) {
		uf::scene::graph.clear();
		uf::scene::queuedInvalidation = false;
	}

	if ( !uf::scene::graph.empty() ) return uf::scene::graph;

	for ( uf::Scene* scene : uf::scene::scenes ) {
		if ( !scene ) continue;
		scene->process([&]( uf::Entity* entity ) {
			auto& metadata = entity->getComponent<uf::ObjectBehavior::Metadata>();
			if ( !metadata.system.ignoreGraph ) uf::scene::graph.emplace_back(entity);
		});
	}
	uf::renderer::states::rebuild = true;
	return uf::scene::graph;
}

void uf::scene::tick() {
if ( uf::scene::useGraph ) {
	auto graph = uf::scene::generateGraph();
	uf::Timer<long long> TIMER_TRACE;
//	UF_DEBUG_MSG("==== START TICK ====");
	for ( auto it = graph.rbegin(); it != graph.rend(); ++it ) {
		(*it)->tick();
//		UF_DEBUG_MSG(TIMER_TRACE.elapsed().asMicroseconds() << " us\t" << (*it)->getName() << ": " << (*it)->getUid());
	}
//	UF_DEBUG_MSG("==== END TICK ====");
} else {
	for ( auto scene : scenes ) scene->tick();
}
}
void uf::scene::render() {
if ( uf::scene::useGraph ) {
	auto graph = uf::scene::generateGraph();
	for ( auto it = graph.rbegin(); it != graph.rend(); ++it ) {
		(*it)->render();
	}
} else {
	for ( auto scene : scenes ) scene->render();
}
}
void uf::scene::destroy() {
	while ( !scenes.empty() ) unloadScene();
}