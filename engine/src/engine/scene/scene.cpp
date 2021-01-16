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
uf::Scene& uf::scene::loadScene( const std::string& name, const std::string& filename ) {
	uf::Scene* scene = uf::instantiator::objects->has( name ) ? (uf::Scene*) &uf::instantiator::instantiate( name ) : new uf::Scene;

//	uf::scene::scenes.insert(uf::scene::scenes.begin(), scene);
	uf::scene::scenes.emplace_back( scene );
	
	std::string target = name;
	
	std::regex regex("^(TestScene_?)?(.+?)(_?Scene)?$");
	std::smatch match;
	
	if ( std::regex_search( target, match, regex ) ) {
		target = match[2];
	}
	target = uf::string::lowercase( target );
	scene->load(filename != "" ? filename : "./scenes/" + target + "/scene.json");
	scene->initialize();
	return *scene;
}
uf::Scene& uf::scene::loadScene( const std::string& name, const uf::Serializer& data ) {
	uf::Scene* scene = uf::instantiator::objects->has( name ) ? (uf::Scene*) &uf::instantiator::instantiate( name ) : new uf::Scene;

//	uf::scene::scenes.insert(uf::scene::scenes.begin(), scene);
	uf::scene::scenes.emplace_back( scene );

	if ( data != "" ) scene->load(data);
	scene->initialize();
	return *scene;
}
void uf::scene::unloadScene() {
	uf::Scene* current = uf::scene::scenes.back();
	current->destroy();
	uf::scene::scenes.pop_back();
	delete current;
}
uf::Scene& uf::scene::getCurrentScene() {
	return *uf::scene::scenes.back();
}
void uf::scene::tick() {
	// uf::scene::getCurrentScene().tick();
	for ( auto scene : scenes ) scene->tick();
}
void uf::scene::render() {
	// uf::scene::getCurrentScene().render();
	for ( auto scene : scenes ) scene->render();
}
void uf::scene::destroy() {
	while ( !scenes.empty() ) {
		unloadScene();
	}
}
/*
uf::Camera& uf::Scene::getCamera() {
	if ( !::camera ) ::camera = this->getPlayer().getComponentPointer<uf::Camera>();
	return *::camera;
}
uf::Player& uf::Scene::getPlayer() {
	return *((uf::Player*) this->findByName("Player"));
}
const uf::Player& uf::Scene::getPlayer() const {
	return *((const uf::Player*) this->findByName("Player"));
}
*/