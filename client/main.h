#pragma once
#include <uf/ext/ext.h>

#include <uf/utils/window/window.h>
#include <uf/utils/serialize/serializer.h>
#include <uf/utils/thread/thread.h>

namespace client {
	extern bool ready;
	extern bool terminated;
	extern uf::Window window;
	extern uf::Serializer config;

	void initialize();
	void tick();
	void render();
	void terminate();
}