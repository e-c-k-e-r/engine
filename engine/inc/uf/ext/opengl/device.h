#pragma once

#include <uf/ext/opengl/ogl.h>
#include <uf/ext/opengl/buffer.h>
#include <uf/spec/context/context.h>
#include <uf/utils/window/window.h>
#include <uf/utils/thread/thread.h>
#include <uf/utils/thread/perthread.h>

namespace ext {
	namespace opengl {
		struct UF_API Device {
		public:
			uf::Window* window;
			spec::Context* context;
			spec::Context::Settings contextSettings;

			void initialize();
			void destroy();
		};
	}
}