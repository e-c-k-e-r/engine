#include <uf/engine/scene/scene.h>
#include <uf/ext/vulkan/vulkan.h>

UF_OBJECT_REGISTER_CPP(Scene)
void uf::Scene::initialize() {
//	this->m_graphics = new std::vector<ext::vulkan::Graphic*>();
//	ext::vulkan::graphics = (std::vector<ext::vulkan::Graphic*>*) this->m_graphics;
	ext::vulkan::scenes.push_back(this);
	ext::vulkan::rebuild = true;
	uf::Object::initialize();
}
void uf::Scene::tick() {
//	ext::vulkan::graphics = (std::vector<ext::vulkan::Graphic*>*) this->m_graphics;
	uf::Object::tick();
}
void uf::Scene::render() {
//	ext::vulkan::graphics = (std::vector<ext::vulkan::Graphic*>*) this->m_graphics;
	uf::Object::render();
}
void uf::Scene::destroy() {
	uf::Object::destroy();

	{
		auto it = std::find(ext::vulkan::scenes.begin(), ext::vulkan::scenes.end(), this);
		if ( it != ext::vulkan::scenes.end() ) {
			ext::vulkan::scenes.erase(it);
		}
	}
/*	
	ext::vulkan::scenes.erase(
		std::remove(ext::vulkan::scenes.begin(), ext::vulkan::scenes.end(), this),
		ext::vulkan::scenes.end()
	);
*/
	ext::vulkan::rebuild = true;
/*
	std::vector<ext::vulkan::Graphic*>* graphics = (std::vector<ext::vulkan::Graphic*>*) this->m_graphics;
	for ( auto* graphic : *graphics ) {
		graphic->destroy();
	}
	delete graphics;
	ext::vulkan::graphics = NULL;
*/
}

uf::Entity* uf::Scene::getController() {
	return this->findByName("Player");
}
const uf::Entity* uf::Scene::getController() const {
	return this->findByName("Player");
}

std::vector<uf::Scene*> uf::scene::scenes;
uf::Scene& uf::scene::loadScene( const std::string& name, const std::string data ) {
	uf::Scene* scene = (uf::Scene*) uf::instantiator::instantiate( name );
	uf::scene::scenes.push_back(scene);
	if ( data != "" ) scene->load(data);
	else scene->initialize();
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