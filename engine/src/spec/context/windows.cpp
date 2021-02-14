#include <uf/spec/context/context.h>

#include <uf/utils/io/iostream.h>

#if UF_ENV_WINDOWS && !UF_USE_SFML
UF_API_CALL spec::win32::Context::Context( uni::Context* shared, const Context::Settings& settings ) : 
	uni::Context( NULL, true, settings ),
	m_deviceContext 	(NULL),
	m_context 			(NULL)
{
	// Creating a dummy window is mandatory: we could create a memory DC but then
	// its pixel format wouldn't match the regular contexts' format, and thus
	// wglShareLists would always fail. Too bad...
#if UF_USE_OPENGL
	// Create a dummy window (disabled and hidden)
	this->m_window = CreateWindowA("STATIC", "", WS_POPUP | WS_DISABLED, 0, 0, 1, 1, NULL, NULL, GetModuleHandle(NULL), NULL);
	ShowWindow(this->m_window, SW_HIDE);
	this->m_deviceContext = GetDC(this->m_window);

	if ( this->m_deviceContext ) this->create(shared);
#endif
}
UF_API_CALL spec::win32::Context::Context( uni::Context* shared, const Context::Settings& settings, const Context::window_t& window ) :
	uni::Context( NULL, false, settings ),
	m_deviceContext 	(NULL),
	m_context 			(NULL)
{
#if UF_USE_OPENGL
	// Get the owner window and its device context
	this->m_window = window.getHandle();
	this->m_deviceContext = GetDC(this->m_window);

	// Create the context
	if ( this->m_deviceContext )
		this->create(shared);
#endif
}
UF_API_CALL spec::win32::Context::Context( uni::Context* shared, const Context::Settings& settings, unsigned int width, unsigned int height ) : Context( shared, settings ) {
	
}
spec::win32::Context::~Context() {
	this->terminate();
}

void UF_API_CALL spec::win32::Context::create( uni::Context* shared ) {
#if UF_USE_OPENGL
//	this->m_settings = settings;
	Context::Settings l_settings{24, 4, 8, 0, 3, 3};

	int bestFormat = 0;
	if ( l_settings.antialiasingLevel > 0 ) {
		// Get the wglChoosePixelFormatARB function (it is an extension)
		PFNWGLCHOOSEPIXELFORMATARBPROC wglChoosePixelFormatARB = reinterpret_cast<PFNWGLCHOOSEPIXELFORMATARBPROC>(wglGetProcAddress("wglChoosePixelFormatARB"));
		if ( wglChoosePixelFormatARB ) {
			// Define the basic attributes we want for our window
			int attributes_ints[] = {
					WGL_DRAW_TO_WINDOW_ARB, 	GL_TRUE,
					WGL_SUPPORT_OPENGL_ARB, 	GL_TRUE,
					WGL_ACCELERATION_ARB, 		WGL_FULL_ACCELERATION_ARB,
					WGL_DOUBLE_BUFFER_ARB, 		GL_TRUE,
					WGL_SAMPLE_BUFFERS_ARB, 	(l_settings.antialiasingLevel ? GL_TRUE : GL_FALSE),
					WGL_SAMPLES_ARB, 			(int) l_settings.antialiasingLevel,
					0, 							0
				};
			float attributes_floats[] = {
				0, 							0
			};
			// Let's check how many formats are supporting our requirements
			int formats[128];
			UINT nbFormats;
			bool isValid = false;
			// 	Decrease AA level until a valid one is found
			do {
				attributes_ints[11] = l_settings.antialiasingLevel;
				isValid = wglChoosePixelFormatARB(
					this->m_deviceContext,
					attributes_ints,
					attributes_floats,
					sizeof(formats) / sizeof(*formats),
					formats,
					&nbFormats
				);
			} while ( (!isValid || nbFormats == 0) && l_settings.antialiasingLevel-- > 0 );
			// Get the best format among the returned ones
			if ( isValid && nbFormats > 0 ) {
				int bestScore = 0xFFFF;
				for ( UINT i = 0; i < nbFormats; ++i ) {
					// Get the current format's attributes
					PIXELFORMATDESCRIPTOR attributes;
					attributes.nSize	= sizeof(attributes);
					attributes.nVersion = 1;
					DescribePixelFormat(this->m_deviceContext, formats[i], sizeof(attributes), &attributes);

					// Evaluate the current configuration
					int score = this->evaluateFormat(
						l_settings,
						attributes.cRedBits + attributes.cGreenBits + attributes.cBlueBits + attributes.cAlphaBits,
						attributes.cDepthBits,
						attributes.cStencilBits,
						l_settings.antialiasingLevel
					);

					// Keep it if it's better than the current best
					if (score < bestScore) {
						bestScore  = score;
						bestFormat = formats[i];
					}
				}
			}
		} else {
		// wglChoosePixelFormatARB not supported ; disabling antialiasing
			uf::iostream << "[" << uf::IoStream::Color()("Red") << "ERROR" << "]" << "Antialiasing is not supported ; it will be disabled" << "\n";
			uf::iostream << "[" << uf::IoStream::Color()("Red") << "ERROR" << "]" << "Antialiasing is not supported ; it will be disabled" << "\n";
			l_settings.antialiasingLevel = 0;
		}
	}

	// Find a pixel format with no antialiasing, if not needed or not supported
	if (bestFormat == 0) {
		// Setup a pixel format descriptor from the rendering settings
		PIXELFORMATDESCRIPTOR descriptor;
		ZeroMemory(&descriptor,   sizeof(descriptor));
		descriptor.nSize		= sizeof(descriptor);
		descriptor.nVersion	 	= 1;
		descriptor.iLayerType   = PFD_MAIN_PLANE;
		descriptor.dwFlags	  	= PFD_DRAW_TO_WINDOW | PFD_SUPPORT_OPENGL | PFD_DOUBLEBUFFER;
		descriptor.iPixelType   = PFD_TYPE_RGBA;
		descriptor.cColorBits   = (BYTE) l_settings.bitsPerPixel;
		descriptor.cDepthBits   = (BYTE) l_settings.depthBits;
		descriptor.cStencilBits = (BYTE) l_settings.stencilBits;
		descriptor.cAlphaBits   = l_settings.bitsPerPixel == 32 ? 8 : 0;

		// Get the pixel format that best matches our requirements
		bestFormat = ChoosePixelFormat(this->m_deviceContext, &descriptor);
		if (bestFormat == 0) {
			uf::iostream << "[" << uf::IoStream::Color()("Red") << "ERROR" << "]" << "Failed to find a suitable pixel format for device context -- cannot create OpenGL context" << "\n";
			return;
		}
	}

	// Extract the depth and stencil bits from the chosen format
	PIXELFORMATDESCRIPTOR actualFormat;
	actualFormat.nSize		= sizeof(actualFormat);
	actualFormat.nVersion 	= 1;
	DescribePixelFormat(this->m_deviceContext, bestFormat, sizeof(actualFormat), &actualFormat);
	l_settings.depthBits   	= actualFormat.cDepthBits;
	l_settings.stencilBits 	= actualFormat.cStencilBits;

	// Set the chosen pixel format
	if (!SetPixelFormat(this->m_deviceContext, bestFormat, &actualFormat)) {
		uf::iostream << "[" << uf::IoStream::Color()("Red") << "ERROR" << "]" << "Failed to set pixel format for device context -- cannot create OpenGL context" << "\n";
		return;
	}

	// Get the context to share display lists with
	HGLRC sharedContext = shared ? ((win32::Context*) shared)->m_context : NULL;

	// Create the OpenGL context -- first try context versions >= 3.0 if it is requested (they require special code)
	while (!this->m_context && (l_settings.majorVersion >= 3)) {
		PFNWGLCREATECONTEXTATTRIBSARBPROC wglCreateContextAttribsARB = reinterpret_cast<PFNWGLCREATECONTEXTATTRIBSARBPROC>(wglGetProcAddress("wglCreateContextAttribsARB"));
		if (wglCreateContextAttribsARB) {
			int attributes[] = {
				WGL_CONTEXT_MAJOR_VERSION_ARB,  (int) l_settings.majorVersion,
				WGL_CONTEXT_MINOR_VERSION_ARB,  (int) l_settings.minorVersion,
				WGL_CONTEXT_PROFILE_MASK_ARB, 	WGL_CONTEXT_COMPATIBILITY_PROFILE_BIT_ARB,
				0, 								0
			};
			this->m_context = wglCreateContextAttribsARB(this->m_deviceContext, sharedContext, attributes);
		}

		// If we couldn't create the context, lower the version number and try again -- stop at 3.0
		// Invalid version numbers will be generated by this algorithm (like 3.9), but we really don't care
		if (!this->m_context) {
			if (l_settings.minorVersion > 0) {
				// If the minor version is not 0, we decrease it and try again
				l_settings.minorVersion--;
			}
			else {
				// If the minor version is 0, we decrease the major version
				l_settings.majorVersion--;
				l_settings.minorVersion = 9;
			}
		}
	}

	// If the OpenGL >= 3.0 context failed or if we don't want one, create a regular OpenGL 1.x/2.x context
	if (!this->m_context) {
		// set the context version to 2.0 (arbitrary)
		l_settings.majorVersion = 2;
		l_settings.minorVersion = 0;

		this->m_context = wglCreateContext(this->m_deviceContext);
		if (!this->m_context) {
			uf::iostream << "[" << uf::IoStream::Color()("Red") << "ERROR" << "]" << "Failed to create an OpenGL context for this window" << "\n";
			return;
		}

		// Share this context with others
		if (sharedContext) {
		// 	wglShareLists doesn't seem to be thread-safe
		//	static Mutex mutex;
		//	Lock lock(mutex);

			if (!wglShareLists(sharedContext, this->m_context)) {
				uf::iostream << "[" << uf::IoStream::Color()("Red") << "ERROR" << "]" << "Failed to share the OpenGL context" << "\n";
			}
		}
	}
#endif
}
void UF_API_CALL spec::win32::Context::terminate() {
#if UF_USE_OPENGL
	// Destroy the OpenGL context
	if ( this->m_context ) {
		if ( wglGetCurrentContext() == this->m_context) wglMakeCurrent(NULL, NULL);
		wglDeleteContext(this->m_context);
	}

	// Destroy the device context
	if ( this->m_deviceContext ) ReleaseDC(this->m_window, this->m_deviceContext);

	// Destroy the window if we own it
	if ( this->m_window && this->m_ownsWindow ) DestroyWindow(this->m_window);
#endif
}

bool spec::win32::Context::makeCurrent() {
#if UF_USE_OPENGL
	return this->m_deviceContext && this->m_context && wglMakeCurrent(this->m_deviceContext, this->m_context);
#else
	return true;
#endif
}

void spec::win32::Context::display() {
#if UF_USE_OPENGL
	if (this->m_deviceContext && this->m_context)
		SwapBuffers(this->m_deviceContext);
#endif
}

void spec::win32::Context::setVerticalSyncEnabled(bool enabled) {
#if UF_USE_OPENGL
	PFNWGLSWAPINTERVALEXTPROC wglSwapIntervalEXT = reinterpret_cast<PFNWGLSWAPINTERVALEXTPROC>(wglGetProcAddress("wglSwapIntervalEXT"));
	if (wglSwapIntervalEXT) wglSwapIntervalEXT(enabled ? 1 : 0);
#endif
}
#endif