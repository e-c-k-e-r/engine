#include <uf/spec/context/context.h>

#if UF_ENV_LINUX && UF_USE_OPENGL && !UF_USE_OPENGL_GLDC
#include <uf/utils/io/iostream.h>

#include <X11/Xlib.h>
#include <X11/Xutil.h>
#include <GL/glx.h>
#include <GL/glxext.h>

#define None 0L
#define Success 0

spec::x11::Context::Context( uni::Context* shared, const Context::Settings& settings ) :
	uni::Context(NULL, true, settings),
	m_display(nullptr),
	m_context(nullptr),
	m_window(0)
{
	m_display = XOpenDisplay(NULL);
	if (!m_display) {
		std::cerr << "Failed to open X display\n";
		return;
	}

	// Create a hidden dummy window
	int scr = DefaultScreen(m_display);

	XSetWindowAttributes swa;
	swa.event_mask = StructureNotifyMask;
	m_window = XCreateSimpleWindow(m_display, RootWindow(m_display, scr), 0, 0, 1, 1, 0, 0, 0);
	
	//XMapWindow(m_display, m_window);

	this->create(shared);
}

spec::x11::Context::Context( uni::Context* shared, const Context::Settings& settings, const Context::window_t& window ) :
	uni::Context(NULL, false, settings),
	m_display(window.getDisplay()),
	m_context(nullptr),
	m_window(window.getHandle())
{
	if (m_display && m_window)
		this->create(shared);
}

spec::x11::Context::Context( uni::Context* shared, const Context::Settings& settings, unsigned int width, unsigned int height ) :
	Context(shared, settings) {
	// Nothing more; dummy window variant handles this
}

spec::x11::Context::~Context() {
	this->terminate();
}

void spec::x11::Context::create( uni::Context* shared ) {
	int attribs[] = {
		GLX_RGBA,
		GLX_DOUBLEBUFFER,
		GLX_DEPTH_SIZE,   (int)m_settings.depthBits,
		GLX_STENCIL_SIZE, (int)m_settings.stencilBits,
		None
	};

	int scr = DefaultScreen(m_display);
	XVisualInfo* vi = glXChooseVisual(m_display, scr, attribs);
	if (!vi) {
		std::cerr << "Failed to choose visual\n";
		return;
	}

	GLXContext sharedCtx = shared ? ((x11::Context*)shared)->m_context : nullptr;
	m_context = glXCreateContext(m_display, vi, sharedCtx, True);

	if (!m_context) {
		std::cerr << "glXCreateContext failed\n";
	}
}

void spec::x11::Context::terminate() {
	if (m_context) {
		if (glXGetCurrentContext() == m_context)
			glXMakeCurrent(m_display, None, NULL);
		glXDestroyContext(m_display, m_context);
		m_context = nullptr;
	}

	if (m_display && m_window && m_ownsWindow) {
		XDestroyWindow(m_display, m_window);
	}

	if (m_display) {
		XCloseDisplay(m_display);
		m_display = nullptr;
	}
}

bool spec::x11::Context::makeCurrent() {
	if (!m_display || !m_context || !m_window) return false;
	return glXMakeCurrent(m_display, m_window, m_context);
}

void spec::x11::Context::display() {
	if (m_display && m_window)
		glXSwapBuffers(m_display, m_window);
}

void spec::x11::Context::setVerticalSyncEnabled(bool enabled) {
	typedef int (*glXSwapIntervalEXTProc)(Display*, GLXDrawable, int);
	static glXSwapIntervalEXTProc glXSwapIntervalEXT =
		(glXSwapIntervalEXTProc) glXGetProcAddressARB((const GLubyte*)"glXSwapIntervalEXT");

	if (glXSwapIntervalEXT) {
		glXSwapIntervalEXT(m_display, glXGetCurrentDrawable(), enabled ? 1 : 0);
	}
}
#endif