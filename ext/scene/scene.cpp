#include "scene.h"

#include <uf/ext/vulkan/vulkan.h>

ext::Scene* ext::Scene::current = NULL;

void ext::Scene::initialize() {
	this->m_graphics = new std::vector<ext::vulkan::Graphic*>();
	ext::vulkan::graphics = (std::vector<ext::vulkan::Graphic*>*) this->m_graphics;

	ext::Object::initialize();
}
void ext::Scene::tick() {
	ext::vulkan::graphics = (std::vector<ext::vulkan::Graphic*>*) this->m_graphics;
	ext::Object::tick();
}
void ext::Scene::render() {
	ext::vulkan::graphics = (std::vector<ext::vulkan::Graphic*>*) this->m_graphics;
	ext::Object::render();
}
void ext::Scene::destroy() {
	ext::Object::destroy();

	std::vector<ext::vulkan::Graphic*>* graphics = (std::vector<ext::vulkan::Graphic*>*) this->m_graphics;
	for ( auto* graphic : *graphics ) {
		graphic->destroy();
	}
	delete graphics;
	ext::vulkan::graphics = NULL;
}

uf::Entity* ext::Scene::getController() {
	return this->findByName("Player");
}
const uf::Entity* ext::Scene::getController() const {
	return this->findByName("Player");
}
/*
uf::Camera& ext::Scene::getCamera() {
	if ( !::camera ) ::camera = this->getPlayer().getComponentPointer<uf::Camera>();
	return *::camera;
}
ext::Player& ext::Scene::getPlayer() {
	return *((ext::Player*) this->findByName("Player"));
}
const ext::Player& ext::Scene::getPlayer() const {
	return *((const ext::Player*) this->findByName("Player"));
}
*/