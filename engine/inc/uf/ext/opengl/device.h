#pragma once

#include <uf/ext/opengl/ogl.h>
#include <uf/ext/opengl/buffer.h>
#include <uf/utils/window/window.h>
#include <uf/utils/thread/thread.h>
#include <uf/utils/thread/perthread.h>

namespace ext {
	namespace opengl {
		class UF_API Device {
		public:
			uf::Window* window;
		};
	}
}