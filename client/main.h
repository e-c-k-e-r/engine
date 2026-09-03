#pragma once

#include <uf/engine/ext.h>
#include <uf/ext/ext.h>

#include <uf/utils/window/window.h>
#include <uf/utils/serialize/serializer.h>
#include <uf/utils/thread/thread.h>
#include <memory>

namespace client {
	extern bool ready;
	extern bool terminated;
	// time control (headless console: pause/resume/step) lives engine-side as uf::paused / uf::stepBudget
	extern std::unique_ptr<uf::Window> window;
	extern uf::Serializer config;

	void initialize();
	void tick();
	void render();
	void terminate();
}