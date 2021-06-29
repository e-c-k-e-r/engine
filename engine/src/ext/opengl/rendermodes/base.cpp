#if UF_USE_OPENGL

#include <uf/ext/opengl/opengl.h>
#include <uf/ext/opengl/rendermodes/base.h>
#include <uf/ext/opengl/initializers.h>
#include <uf/utils/window/window.h>
#include <uf/ext/opengl/graphic.h>
#include <uf/ext/opengl/rendertarget.h>
#include <uf/utils/graphic/graphic.h>
#include <uf/utils/camera/camera.h>

const uf::stl::string ext::opengl::BaseRenderMode::getType() const {
	return "Swapchain";
}
void ext::opengl::BaseRenderMode::createCommandBuffers( const uf::stl::vector<ext::opengl::Graphic*>& graphics ) {
	float width = this->width > 0 ? this->width : ext::opengl::settings::width;
	float height = this->height > 0 ? this->height : ext::opengl::settings::height;

	auto& scene = uf::scene::getCurrentScene();
	auto& sceneMetadata = scene.getComponent<uf::Serializer>();

	size_t currentSubpass = 0;
	size_t currentPass = 0;
	size_t currentDraw = 0;
	auto& commands = getCommands();
	commands.start(); {
	#if 0
		CommandBuffer::InfoClear clearCommandInfo = {};
		clearCommandInfo.type = enums::Command::CLEAR;
		clearCommandInfo.color = {0.0f, 0.0f, 0.0f, 0.0f};
		clearCommandInfo.depth = uf::matrix::reverseInfiniteProjection ? 0.0f : 1.0f;
		clearCommandInfo.bits = GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT;
		if ( !ext::json::isNull( sceneMetadata["system"]["renderer"]["clear values"][0] ) ) {
			clearCommandInfo.color = uf::vector::decode( sceneMetadata["system"]["renderer"]["clear values"][0], pod::Vector4f{0,0,0,0} );
		}
	
		if ( !ext::json::isNull( sceneMetadata["light"]["ambient"] ) ) {
		//	clearCommandInfo.color = uf::vector::decode( sceneMetadata["light"]["ambient"], pod::Vector3f{} );
		//	auto ambient = uf::vector::decode( sceneMetadata["light"]["ambient"], pod::Vector4f{1,1,1,1} );
		//	GL_ERROR_CHECK(glLightfv(GL_LIGHT0, GL_AMBIENT, &ambient[0]));
		}
		commands.record(clearCommandInfo);
	#endif

		CommandBuffer::InfoViewport viewportCommandInfo = {};
		viewportCommandInfo.type = enums::Command::VIEWPORT;
		viewportCommandInfo.corner = pod::Vector2ui{0, 0};
		viewportCommandInfo.size = pod::Vector2ui{width, height};
		commands.record(viewportCommandInfo);
		for ( auto graphic : graphics ) {
			if ( graphic->descriptor.renderMode != "" ) continue;
			GraphicDescriptor descriptor = bindGraphicDescriptor(graphic->descriptor, currentSubpass );
			graphic->record( commands, descriptor, currentPass, currentDraw++ );
		}
	} commands.end();
}

void ext::opengl::BaseRenderMode::initialize( Device& device ) {
	this->metadata.name = "Swapchain";
	auto windowSize = device.window->getSize();
	this->width = windowSize.x;
	this->height = windowSize.y;

	if ( uf::matrix::reverseInfiniteProjection ) {
		GL_ERROR_CHECK(glDepthFunc(GL_GEQUAL));
		#if !UF_ENV_DREAMCAST || (UF_ENV_DREAMCAST && UF_USE_OPENGL_GLDC)
			glDepthRange(0, 1);
		#endif
	} else {
		GL_ERROR_CHECK(glDepthFunc(GL_LESS));
	}
	GL_ERROR_CHECK(glEnable(GL_DEPTH_TEST));
	GL_ERROR_CHECK(glEnable(GL_TEXTURE_2D));
	GL_ERROR_CHECK(glEnable(GL_BLEND));
	GL_ERROR_CHECK(glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA));

//	GL_ERROR_CHECK(glEnable(GL_LIGHTING));
//	GL_ERROR_CHECK(glEnable(GL_NORMALIZE));
//	GL_ERROR_CHECK(glEnable(GL_COLOR_MATERIAL));
//	GL_ERROR_CHECK(glColorMaterial(GL_FRONT_AND_BACK, GL_AMBIENT_AND_DIFFUSE));
//	GL_ERROR_CHECK(glLightModeli(GL_LIGHT_MODEL_COLOR_CONTROL, GL_SEPARATE_SPECULAR_COLOR));
#if UF_USE_DREAMCAST
	//profiler_enable();
	GL_ERROR_CHECK(glEnable(GL_NEARZ_CLIPPING_KOS));
//	GL_ERROR_CHECK(glLightModeli(GL_LIGHT_MODEL_LOCAL_VIEWER, 0));
//	GL_ERROR_CHECK(glShadeModel(GL_SMOOTH));
#else
//	GL_ERROR_CHECK(glLightModeli(GL_LIGHT_MODEL_LOCAL_VIEWER, 1));
//	GL_ERROR_CHECK(glShadeModel(GL_SMOOTH));
#endif

	ext::opengl::RenderMode::initialize( device );
}

void ext::opengl::BaseRenderMode::tick() {
	if ( ext::opengl::states::resized ) {
		auto windowSize = device->window->getSize();
		this->width = windowSize.x;
		this->height = windowSize.y;
		this->rebuild = true;
	}
	ext::opengl::RenderMode::tick();
}
void ext::opengl::BaseRenderMode::render() {
//	if ( ext::opengl::renderModes.size() > 1 ) return;
//	if ( ext::opengl::renderModes.back() != this ) return;
	ext::opengl::RenderMode::render();
}

void ext::opengl::BaseRenderMode::destroy() {
	ext::opengl::RenderMode::destroy();
}

#endif