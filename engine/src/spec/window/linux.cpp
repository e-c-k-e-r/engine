#include <uf/spec/window/window.h>

#if UF_ENV_LINUX
#include <uf/utils/io/inputs.h>
#include <uf/utils/io/iostream.h>
#include <uf/utils/serialize/serializer.h>
#include <uf/utils/string/ext.h>
#include <uf/utils/string/utf.h>
#include <uf/utils/window/payloads.h>

#define UF_OPENGL_CONTEXT_IN_WINDOW 0
#include <uf/spec/context/context.h>

#define UF_HOOK_USE_USERDATA 1
#define UF_HOOK_USE_JSON 0

#if UF_USE_VULKAN
	#include <vulkan/vulkan.h>
	#include <vulkan/vulkan_xlib.h>
#elif UF_USE_OPENGL
	#include <GL/glx.h>
#endif

#include <X11/Xlib.h>
#include <X11/Xutil.h>
#include <X11/extensions/Xrandr.h>
#include <X11/keysym.h>
#include <unistd.h>

#include <cstring>
#include <iostream>

#define None 0L
#define Success 0

namespace {
	Display *globalDisplay = nullptr;
	int windowCount = 0;

	float lastMouseWheel;

	void initX11() {
		if (!globalDisplay) {
			globalDisplay = XOpenDisplay(nullptr);
			if (!globalDisplay) {
				UF_MSG_ERROR("Failed to open X11 display.");
				exit(1);
			}
		}
		++windowCount;
	}
	void shutdownX11() {
		if (--windowCount == 0 && globalDisplay) {
			XCloseDisplay(globalDisplay);
			globalDisplay = nullptr;
		}
	}
	struct MonitorInfo {
		int x, y;  // top-left offset
		int width;
		int height;
		bool primary;
	};

	uf::stl::vector<MonitorInfo> getMonitors(Display *display, int screen) {
		uf::stl::vector<MonitorInfo> monitors;

		Window root = RootWindow(display, screen);

		XRRScreenResources *res = XRRGetScreenResourcesCurrent(display, root);
		if (!res) return monitors;

		RROutput primary = XRRGetOutputPrimary(display, root);

		for (int i = 0; i < res->noutput; i++) {
			XRROutputInfo *output_info =
			    XRRGetOutputInfo(display, res, res->outputs[i]);
			if (!output_info || output_info->connection == RR_Disconnected) {
				if (output_info) XRRFreeOutputInfo(output_info);
				continue;
			}

			XRRCrtcInfo *crtc_info =
			    XRRGetCrtcInfo(display, res, output_info->crtc);
			if (!crtc_info) {
				XRRFreeOutputInfo(output_info);
				continue;
			}

			MonitorInfo info;
			info.x = crtc_info->x;
			info.y = crtc_info->y;
			info.width = crtc_info->width;
			info.height = crtc_info->height;
			info.primary = (res->outputs[i] == primary);

			monitors.push_back(info);

			XRRFreeCrtcInfo(crtc_info);
			XRRFreeOutputInfo(output_info);
		}

		XRRFreeScreenResources(res);
		return monitors;
	}

	#if 0
			float getXftDPI(Display* dpy) {
				char* rms = XResourceManagerString(dpy);
				if (!rms) return 0.0f;

				XrmInitialize();
				XrmDatabase db = XrmGetStringDatabase(rms);
				if (!db) return 0.0f;

				XrmValue value;
				char* type = nullptr;
				if (XrmGetResource(db, "Xft.dpi", "Xft.dpi", &type, &value)) {
					if (value.addr) {
						return std::atof(value.addr);
					}
				}
				return 0.0f;
			}

			float getRandRDPI(Display* dpy, int screen) {
				Window root = RootWindow(dpy, screen);
				XRRScreenResources* res = XRRGetScreenResourcesCurrent(dpy, root);
				if (!res) return 96.0f;

				XRROutputInfo* out_info = XRRGetOutputInfo(dpy, res, res->outputs[0]);
				XRRCrtcInfo* crtc_info  = XRRGetCrtcInfo(dpy, res, out_info->crtc);

				double mmWidth = out_info->mm_width;
				double mmHeight = out_info->mm_height;
				double width = crtc_info->width;
				double height = crtc_info->height;

				double dpiX = (width  * 25.4) / mmWidth;   // pixels per inch
				double dpiY = (height * 25.4) / mmHeight;

				XRRFreeCrtcInfo(crtc_info);
				XRRFreeOutputInfo(out_info);
				XRRFreeScreenResources(res);

				return (dpiX + dpiY) * 0.5; // average
			}
	#endif

	uf::stl::string KeySymToString(KeySym sym) {
		char *name = XKeysymToString(sym);
		return name ? uf::stl::string(name) : "?";
	}

	static uf::stl::string NormalizeKeyName(const uf::stl::string &raw) {
		if (raw == "Shift_L") return "LShift";
		if (raw == "Shift_R") return "RShift";
		if (raw == "Control_L") return "LControl";
		if (raw == "Control_R") return "RControl";
		if (raw == "Alt_L") return "LAlt";
		if (raw == "Alt_R") return "RAlt";
		if (raw == "Super_L") return "LSystem";
		if (raw == "Super_R") return "RSystem";
		// translate "Return" -> "Enter"
		if (raw == "Return") return "Enter";
		return raw;
	}
}  // namespace

spec::x11::Window::Window()
    : m_display(nullptr),
      m_screen(0),
      m_handle(0),
      m_gc(0),
      m_xim(nullptr),
      m_xic(nullptr),
      m_context(nullptr),
      m_lastSize({}),
      m_keyRepeatEnabled(true),
      m_resizing(false),
      m_mouseGrabbed(false),
      m_syncParse(true),
      m_asyncParse(false) {}

spec::x11::Window::Window(const vector_t &size, const title_t &title)
    : Window() {
	create(size, title);
}

void spec::x11::Window::create(const vector_t &size, const title_t &title) {
	initX11();

	m_display = globalDisplay;
	m_screen = DefaultScreen(m_display);

	int dimW = size.x > 0 ? size.x : DisplayWidth(m_display, m_screen);
	int dimH = size.y > 0 ? size.y : DisplayHeight(m_display, m_screen);

	m_handle = XCreateSimpleWindow(
	    m_display, RootWindow(m_display, m_screen), 0, 0, dimW, dimH, 1,
	    BlackPixel(m_display, m_screen), WhitePixel(m_display, m_screen));

	// Input events mask
	XSelectInput(m_display, m_handle,
	             ExposureMask | KeyPressMask | KeyReleaseMask |
	                 ButtonPressMask | ButtonReleaseMask | PointerMotionMask |
	                 StructureNotifyMask | FocusChangeMask);

	// Set Title
	setTitle(title);

	m_gc = XCreateGC(m_display, m_handle, 0, nullptr);

	m_xim = XOpenIM(m_display, nullptr, nullptr, nullptr);
	if (m_xim == nullptr) {
		XSetLocaleModifiers("@im=none");
		m_xim = XOpenIM(m_display, nullptr, nullptr, nullptr);
	}
	m_xic = XCreateIC(m_xim, XNInputStyle, XIMPreeditNothing | XIMStatusNothing,
	                  XNClientWindow, m_handle, XNFocusWindow, m_handle, NULL);

	Atom WM_DELETE_WINDOW = XInternAtom(m_display, "WM_DELETE_WINDOW", False);
	XSetWMProtocols(m_display, m_handle, &WM_DELETE_WINDOW, 1);

	XMapWindow(m_display, m_handle);
	XFlush(m_display);

#if UF_USE_OPENGL && UF_OPENGL_CONTEXT_IN_WINDOW
	m_context = (void *)spec::uni::Context::create(settings, *this);
#endif
}

spec::x11::Window::~Window() { terminate(); }

void spec::x11::Window::terminate() {
	if (m_handle) {
		XDestroyWindow(m_display, m_handle);
		m_handle = 0;
	}
	shutdownX11();

#if UF_OPENGL_CONTEXT_IN_WINDOW
	if (m_context) {
		spec::Context *context = (spec::Context *)m_context;
		context->terminate();
		delete context;
		m_context = NULL;
	}
#endif
}

Display* spec::x11::Window::getDisplay() const {
	return m_display;
}
spec::x11::Window::handle_t spec::x11::Window::getHandle() const {
	return m_handle;
}

spec::x11::Window::vector_t spec::x11::Window::getPosition() const {
	int x, y;
	::Window child;
	XTranslateCoordinates(m_display, m_handle, RootWindow(m_display, m_screen),
	                      0, 0, &x, &y, &child);
	return {(unsigned)x, (unsigned)y};
}

spec::x11::Window::vector_t spec::x11::Window::getSize() const {
	XWindowAttributes attr;
	XGetWindowAttributes(m_display, m_handle, &attr);
	return {(unsigned)attr.width, (unsigned)attr.height};
}

/*

float spec::x11::Window::getDPIScale() const {
    if (!m_display) return 1.0f;

    float dpi = getXftDPI(m_display);
    if (dpi <= 0.0f) dpi = getRandRDPI(m_display, m_screen);
    if (dpi <= 0.0f) dpi = 96.0f;

    return dpi / 96.0f; // scale relative to Win32 baseline
}

*/

size_t spec::x11::Window::getRefreshRate() const {
	return 60;
	/*
	    if (!m_display) return 60; // safe default

	    Window root = RootWindow(m_display, m_screen);

	    XRRScreenResources* res = XRRGetScreenResourcesCurrent(m_display, root);
	    if (!res) return 60;

	    RROutput primary = XRRGetOutputPrimary(m_display, root);
	    if (primary == None && res->noutput > 0)
	        primary = res->outputs[0]; // fallback

	    XRROutputInfo* output_info = XRRGetOutputInfo(m_display, res, primary);
	    if (!output_info) {
	        XRRFreeScreenResources(res);
	        return 60;
	    }

	    XRRCrtcInfo* crtc_info = XRRGetCrtcInfo(m_display, res,
	   output_info->crtc);

	    size_t refresh = 60; // default fallback
	    if (crtc_info) {
	        for (int i = 0; i < res->nmode; i++) {
	            XRRModeInfo& mode = res->modes[i];
	            if (mode.id == crtc_info->mode) {
	                if (mode.hTotal > 0 && mode.vTotal > 0) {
	                    double rate = (double)mode.dotClock /
	                                 ((double)mode.hTotal *
	   (double)mode.vTotal); refresh = static_cast<size_t>(std::round(rate));
	                }
	                break;
	            }
	        }
	        XRRFreeCrtcInfo(crtc_info);
	    }

	    XRRFreeOutputInfo(output_info);
	    XRRFreeScreenResources(res);

	    return refresh;
	*/
}

void spec::x11::Window::setPosition(const vector_t &pos) {
	XMoveWindow(m_display, m_handle, pos.x, pos.y);
}

bool spec::x11::Window::isKeyPressed(const uf::stl::string &key) {
	if (!uf::Window::focused) return false;

	auto m_display = ::globalDisplay;

	// Query raw keyboard state
	char keys[32];
	XQueryKeymap(m_display, keys);

	// Convert friendly name (string) into KeySym
	// KeySym sym = XStringToKeysym(key.c_str());
	KeySym sym = XStringToKeysym(NormalizeKeyName(key).c_str());
	if (sym == NoSymbol) return false;

	// Translate KeySym → KeyCode
	KeyCode kc = XKeysymToKeycode(m_display, sym);
	if (kc == 0) return false;

	// Each bit represents a key state
	return (keys[kc >> 3] & (1 << (kc & 7))) != 0;
}

void spec::x11::Window::centerWindow() {
	auto monitors = getMonitors(m_display, m_screen);
	if (monitors.empty()) return;

	// Current window geometry
	XWindowAttributes attr;
	XGetWindowAttributes(m_display, m_handle, &attr);
	int winCenterX = attr.x + attr.width / 2;
	int winCenterY = attr.y + attr.height / 2;

	// Find monitor containing window center
	MonitorInfo target = monitors[0];
	for (auto &mon : monitors) {
		if (winCenterX >= mon.x && winCenterX < mon.x + mon.width &&
		    winCenterY >= mon.y && winCenterY < mon.y + mon.height) {
			target = mon;
			break;
		}
	}

	// Center window within this monitor
	auto size = getSize();
	int newX = target.x + (target.width - size.x) / 2;
	int newY = target.y + (target.height - size.y) / 2;

	setPosition({(unsigned)newX, (unsigned)newY});
}

void spec::x11::Window::setMousePosition(const vector_t &pos) {
	XWarpPointer(m_display, None, m_handle, 0, 0, 0, 0, pos.x, pos.y);
	XFlush(m_display);
}

spec::x11::Window::vector_t spec::x11::Window::getMousePosition() {
	int root_x, root_y, win_x, win_y;
	unsigned mask;
	::Window child, root;
	XQueryPointer(m_display, m_handle, &root, &child, &root_x, &root_y, &win_x,
	              &win_y, &mask);
	return {(unsigned)win_x, (unsigned)win_y};
}

void spec::x11::Window::setSize(const vector_t &size) {
	XResizeWindow(m_display, m_handle, size.x, size.y);
}

void spec::x11::Window::setTitle(const title_t &title) {
	XStoreName(m_display, m_handle, (const char *)title.c_str());
}
/*
void spec::x11::Window::setIcon(const vector_t&, uint8_t*) {
    // Convert RGBA8 -> ARGB32 (unsigned long)
    uf::stl::vector<unsigned long> icon;
    icon.resize(2 + size.x * size.y);
    icon[0] = size.x;
    icon[1] = size.y;

    for (int i = 0; i < size.x * size.y; i++) {
        unsigned char r = pixels[i*4+0];
        unsigned char g = pixels[i*4+1];
        unsigned char b = pixels[i*4+2];
        unsigned char a = pixels[i*4+3];
        unsigned long argb = ((unsigned long)a << 24) |
                             ((unsigned long)r << 16) |
                             ((unsigned long)g <<  8) |
                             ((unsigned long)b);
        icon[2+i] = argb;
    }

    Atom property = XInternAtom(m_display, "_NET_WM_ICON", False);
    XChangeProperty(m_display, m_handle, property, XA_CARDINAL, 32,
PropModeReplace, (unsigned char*)icon.data(), icon.size()); XFlush(m_display);
}
*/
void spec::x11::Window::setIcon(const vector_t &size, uint8_t *pixels) {
	// Convert RGBA8 (incoming) into ARGB32 (required by _NET_WM_ICON)
	uf::stl::vector<unsigned long> icon;
	icon.resize(2 + size.x * size.y);
	icon[0] = size.x;
	icon[1] = size.y;

	for (size_t i = 0; i < size.x * size.y; i++) {
		uint8_t r = pixels[i * 4 + 0];
		uint8_t g = pixels[i * 4 + 1];
		uint8_t b = pixels[i * 4 + 2];
		uint8_t a = pixels[i * 4 + 3];

		// Pack ARGB as required by X
		unsigned long argb = ((unsigned long)a << 24) |
		                     ((unsigned long)r << 16) |
		                     ((unsigned long)g << 8) | ((unsigned long)b);

		icon[2 + i] = argb;
	}

	Atom property = XInternAtom(m_display, "_NET_WM_ICON", False);
	XChangeProperty(
	    m_display, m_handle, property, XA_CARDINAL, 32, PropModeReplace,
	    reinterpret_cast<unsigned char *>(icon.data()), icon.size());
	XFlush(m_display);
}
void spec::x11::Window::setVisible(bool vis) {
	if (vis)
		XMapWindow(m_display, m_handle);
	else
		XUnmapWindow(m_display, m_handle);
	XFlush(m_display);
}

/*
void spec::x11::Window::setCursorVisible(bool vis) {
    if (!vis) {
        // hide cursor
        Pixmap bm_no;
        XColor black; memset(&black, 0, sizeof(black));
        char no_data[] = { 0 };
        bm_no = XCreateBitmapFromData(m_display, m_handle, no_data, 1, 1);
        Cursor invisible = XCreatePixmapCursor(m_display, bm_no, bm_no,
                                               &black, &black, 0, 0);
        XDefineCursor(m_display, m_handle, invisible);
        XFreeCursor(m_display, invisible);
    } else {
        XUndefineCursor(m_display, m_handle);
    }
}
*/

void spec::x11::Window::setCursorVisible(bool visibility) {
	if (visibility) {
		// Restore default cursor
		XUndefineCursor(m_display, m_handle);
		XFlush(m_display);
	} else {
		// Create an invisible cursor (1x1 transparent)
		Pixmap bm_no;
		XColor dummy;
		static char no_data[] = {0};
		bm_no = XCreateBitmapFromData(m_display, m_handle, no_data, 1, 1);
		dummy.red = dummy.green = dummy.blue = 0;

		Cursor invisible =
		    XCreatePixmapCursor(m_display, bm_no, bm_no, &dummy, &dummy, 0, 0);
		XDefineCursor(m_display, m_handle, invisible);
		XFreeCursor(m_display, invisible);
		XFreePixmap(m_display, bm_no);
		XFlush(m_display);
	}
}

/*
#include <X11/Xcursor/Xcursor.h>

void spec::x11::Window::setCustomCursor(const vector_t& size, uint8_t* pixels) {
    XcursorImage* img = XcursorImageCreate(size.x, size.y);
    if (!img) return;

    img->xhot = size.x / 2; // center hot spot
    img->yhot = size.y / 2;

    // Fill ARGB pixels
    for (size_t i = 0; i < size.x * size.y; i++) {
        uint8_t r = pixels[i * 4 + 0];
        uint8_t g = pixels[i * 4 + 1];
        uint8_t b = pixels[i * 4 + 2];
        uint8_t a = pixels[i * 4 + 3];

        img->pixels[i] = ((unsigned long)a << 24) |
                         ((unsigned long)r << 16) |
                         ((unsigned long)g << 8)  |
                         ((unsigned long)b);
    }

    Cursor cursor = XcursorImageLoadCursor(m_display, img);
    XcursorImageDestroy(img);

    XDefineCursor(m_display, m_handle, cursor);
    XFreeCursor(m_display, cursor);
    XFlush(m_display);
}
*/

void spec::x11::Window::setKeyRepeatEnabled(bool state) {
	m_keyRepeatEnabled = state;
}

void spec::x11::Window::setMouseGrabbed(bool state) {
	this->m_mouseGrabbed = state;
	this->grabMouse(state);
}

void spec::x11::Window::grabMouse(bool state) {
	if (state) {
		// Grab pointer: locks input focus to this window
		int result = XGrabPointer(
		    m_display, m_handle,
		    True,  // owner_events
		    ButtonPressMask | ButtonReleaseMask | PointerMotionMask,
		    GrabModeAsync, GrabModeAsync,
		    m_handle,  // confine to this window
		    None, CurrentTime);

		if (result != GrabSuccess) {
			UF_MSG_ERROR("Failed to grab mouse pointer via XGrabPointer");
		}

		// Optionally grab keyboard too (rare, but possible)
		// XGrabKeyboard(m_display, m_handle, True, GrabModeAsync,
		// GrabModeAsync, CurrentTime);

	} else {
		// Release
		XUngrabPointer(m_display, CurrentTime);
		// XUngrabKeyboard(m_display, CurrentTime);
	}

	XFlush(m_display);
}

void spec::x11::Window::requestFocus() {
	XSetInputFocus(m_display, m_handle, RevertToParent, CurrentTime);
}

bool spec::x11::Window::hasFocus() const {
	::Window focused;
	int revert;
	XGetInputFocus(m_display, &focused, &revert);
	return focused == m_handle;
}

void spec::x11::Window::bufferInputs() {
	uf::Window::focused = this->hasFocus();

	char keys[32];
	XQueryKeymap(m_display, keys);

#define GET_KEYSTATE(SYMBOL)                           \
	(uf::Window::focused &&                            \
	 (keys[XKeysymToKeycode(m_display, SYMBOL) >> 3] & \
	  (1 << (XKeysymToKeycode(m_display, SYMBOL) & 7))))

#define STORE_KEYSTATE(K, SYM) uf::inputs::kbm::states::K = GET_KEYSTATE(SYM);

	STORE_KEYSTATE(LShift, XK_Shift_L);
	STORE_KEYSTATE(RShift, XK_Shift_R);
	STORE_KEYSTATE(LAlt, XK_Alt_L);
	STORE_KEYSTATE(RAlt, XK_Alt_R);
	STORE_KEYSTATE(LControl, XK_Control_L);
	STORE_KEYSTATE(RControl, XK_Control_R);
	STORE_KEYSTATE(LSystem, XK_Super_L);
	STORE_KEYSTATE(RSystem, XK_Super_R);

	STORE_KEYSTATE(Menu, XK_Menu);
	STORE_KEYSTATE(SemiColon, XK_semicolon);
	STORE_KEYSTATE(Slash, XK_slash);
	STORE_KEYSTATE(Equal, XK_equal);
	STORE_KEYSTATE(Dash, XK_minus);
	STORE_KEYSTATE(LBracket, XK_bracketleft);
	STORE_KEYSTATE(RBracket, XK_bracketright);
	STORE_KEYSTATE(Comma, XK_comma);
	STORE_KEYSTATE(Period, XK_period);
	STORE_KEYSTATE(Quote, XK_apostrophe);
	STORE_KEYSTATE(BackSlash, XK_backslash);
	STORE_KEYSTATE(Tilde, XK_grave);

	STORE_KEYSTATE(Escape, XK_Escape);
	STORE_KEYSTATE(Space, XK_space);
	STORE_KEYSTATE(Enter, XK_Return);
	STORE_KEYSTATE(BackSpace, XK_BackSpace);
	STORE_KEYSTATE(Tab, XK_Tab);
	STORE_KEYSTATE(PageUp, XK_Page_Up);
	STORE_KEYSTATE(PageDown, XK_Page_Down);
	STORE_KEYSTATE(End, XK_End);
	STORE_KEYSTATE(Home, XK_Home);
	STORE_KEYSTATE(Insert, XK_Insert);
	STORE_KEYSTATE(Delete, XK_Delete);

	STORE_KEYSTATE(Add, XK_KP_Add);
	STORE_KEYSTATE(Subtract, XK_KP_Subtract);
	STORE_KEYSTATE(Multiply, XK_KP_Multiply);
	STORE_KEYSTATE(Divide, XK_KP_Divide);
	STORE_KEYSTATE(Pause, XK_Pause);

	// Function keys
	STORE_KEYSTATE(F1, XK_F1);
	STORE_KEYSTATE(F2, XK_F2);
	STORE_KEYSTATE(F3, XK_F3);
	STORE_KEYSTATE(F4, XK_F4);
	STORE_KEYSTATE(F5, XK_F5);
	STORE_KEYSTATE(F6, XK_F6);
	STORE_KEYSTATE(F7, XK_F7);
	STORE_KEYSTATE(F8, XK_F8);
	STORE_KEYSTATE(F9, XK_F9);
	STORE_KEYSTATE(F10, XK_F10);
	STORE_KEYSTATE(F11, XK_F11);
	STORE_KEYSTATE(F12, XK_F12);
	STORE_KEYSTATE(F13, XK_F13);
	STORE_KEYSTATE(F14, XK_F14);
	STORE_KEYSTATE(F15, XK_F15);

	// Arrow keys
	STORE_KEYSTATE(Left, XK_Left);
	STORE_KEYSTATE(Right, XK_Right);
	STORE_KEYSTATE(Up, XK_Up);
	STORE_KEYSTATE(Down, XK_Down);

	// Numpad
	STORE_KEYSTATE(Numpad0, XK_KP_0);
	STORE_KEYSTATE(Numpad1, XK_KP_1);
	STORE_KEYSTATE(Numpad2, XK_KP_2);
	STORE_KEYSTATE(Numpad3, XK_KP_3);
	STORE_KEYSTATE(Numpad4, XK_KP_4);
	STORE_KEYSTATE(Numpad5, XK_KP_5);
	STORE_KEYSTATE(Numpad6, XK_KP_6);
	STORE_KEYSTATE(Numpad7, XK_KP_7);
	STORE_KEYSTATE(Numpad8, XK_KP_8);
	STORE_KEYSTATE(Numpad9, XK_KP_9);

	// Letters
	STORE_KEYSTATE(Q, XK_Q);
	STORE_KEYSTATE(W, XK_W);
	STORE_KEYSTATE(E, XK_E);
	STORE_KEYSTATE(R, XK_R);
	STORE_KEYSTATE(T, XK_T);
	STORE_KEYSTATE(Y, XK_Y);
	STORE_KEYSTATE(U, XK_U);
	STORE_KEYSTATE(I, XK_I);
	STORE_KEYSTATE(O, XK_O);
	STORE_KEYSTATE(P, XK_P);

	STORE_KEYSTATE(A, XK_A);
	STORE_KEYSTATE(S, XK_S);
	STORE_KEYSTATE(D, XK_D);
	STORE_KEYSTATE(F, XK_F);
	STORE_KEYSTATE(G, XK_G);
	STORE_KEYSTATE(H, XK_H);
	STORE_KEYSTATE(J, XK_J);
	STORE_KEYSTATE(K, XK_K);
	STORE_KEYSTATE(L, XK_L);

	STORE_KEYSTATE(Z, XK_Z);
	STORE_KEYSTATE(X, XK_X);
	STORE_KEYSTATE(C, XK_C);
	STORE_KEYSTATE(V, XK_V);
	STORE_KEYSTATE(B, XK_B);
	STORE_KEYSTATE(N, XK_N);
	STORE_KEYSTATE(M, XK_M);

	STORE_KEYSTATE(Num1, XK_1);
	STORE_KEYSTATE(Num2, XK_2);
	STORE_KEYSTATE(Num3, XK_3);
	STORE_KEYSTATE(Num4, XK_4);
	STORE_KEYSTATE(Num5, XK_5);
	STORE_KEYSTATE(Num6, XK_6);
	STORE_KEYSTATE(Num7, XK_7);
	STORE_KEYSTATE(Num8, XK_8);
	STORE_KEYSTATE(Num9, XK_9);
	STORE_KEYSTATE(Num0, XK_0);

	// Query mouse button states
	::Window root, child;
	int rootX, rootY, winX, winY;
	unsigned int mask;
	XQueryPointer(m_display, m_handle, &root, &child, &rootX, &rootY, &winX,
	              &winY, &mask);

	uf::inputs::kbm::states::Mouse1 = (mask & Button1Mask);  // Left
	uf::inputs::kbm::states::Mouse2 = (mask & Button3Mask);  // Right
	uf::inputs::kbm::states::Mouse3 = (mask & Button2Mask);  // Middle

	uf::inputs::kbm::states::MouseWheel =
	    uf::Window::focused ? ::lastMouseWheel : 0;
	::lastMouseWheel = 0;
}
/*
void spec::x11::Window::processEvents() {
    uf::stl::string prettyName = NormalizeKeyName( KeySymToString( sym ) );
    while (XPending(m_display) > 0) {
        XEvent ev;
        XNextEvent(m_display, &ev);

        switch (ev.type) {
            case ConfigureNotify: {
                vector_t newSize = { (unsigned)ev.xconfigure.width,
(unsigned)ev.xconfigure.height }; if (newSize != m_lastSize) { m_lastSize =
newSize; pod::payloads::windowResized event { { "window:Resized", "os" }, {
m_lastSize }
                    };
                    this->pushEvent(event.type, event);
                }
            } break;

            case DestroyNotify: {
                pod::payloads::windowEvent event { "window:Closed", "os" };
                this->pushEvent(event.type, event);
                this->terminate();
            } break;

            case FocusIn: {
                pod::payloads::windowFocusedChanged event {
                    { "window:Focus.Changed", "os" },
                    { 1 } // gained
                };
                this->pushEvent(event.type, event);
            } break;
            case FocusOut: {
                pod::payloads::windowFocusedChanged event {
                    { "window:Focus.Changed", "os" },
                    { -1 } // lost
                };
                this->pushEvent(event.type, event);
            } break;

            case KeyPress: {
                KeySym sym;
                char buffer[32];
                Status status;
                int len = Xutf8LookupString(m_xic, &ev.xkey, buffer,
sizeof(buffer)-1, &sym, &status); buffer[len] = '\0';

                // Key info
                uf::stl::string keycode = KeySymToString(sym);

                pod::payloads::windowKey event{
                    { "window:Key", "os" },
                    {
                        keycode,
                        (unsigned long)sym,
                        -1, // pressed
                        false,
                        {
                            .alt  = (ev.xkey.state & Mod1Mask),
                            .ctrl = (ev.xkey.state & ControlMask),
                            .shift= (ev.xkey.state & ShiftMask),
                            .sys  = (ev.xkey.state & Mod4Mask)
                        }
                    }
                };
                this->pushEvent(event.type, event);
                this->pushEvent(event.type + "." + keycode, event);

                // Text input (if UTF-8 char is present)
                if (len > 0) {
                    uf::stl::string utf8(buffer);
                    pod::payloads::windowTextEntered textEvent{
                        { "window:Text.Entered", "os" },
                        { (uint32_t)utf8[0], utf8 } // simplistic, full UTF-32
conversion recommended
                    };
                    this->pushEvent(textEvent.type, textEvent);
                }
            } break;

            case KeyRelease: {
                KeySym sym = XLookupKeysym(&ev.xkey, 0);
                uf::stl::string keycode = KeySymToString(sym);

                pod::payloads::windowKey event{
                    { "window:Key", "os" },
                    {
                        keycode,
                        (unsigned long)sym,
                        1, // released
                        false,
                        {
                            .alt  = (ev.xkey.state & Mod1Mask),
                            .ctrl = (ev.xkey.state & ControlMask),
                            .shift= (ev.xkey.state & ShiftMask),
                            .sys  = (ev.xkey.state & Mod4Mask)
                        }
                    }
                };
                this->pushEvent(event.type, event);
                this->pushEvent(event.type + "." + keycode, event);
            } break;

            case ButtonPress: {
                pod::Vector2i pos = { ev.xbutton.x, ev.xbutton.y };
                int state = -1;
                uf::stl::string button;

                switch (ev.xbutton.button) {
                    case 1: button = "Left"; break;
                    case 2: button = "Middle"; break;
                    case 3: button = "Right"; break;
                    case 8: // aux typical
                    case 9: button = "Aux"; break;
                    case 4: { // wheel up
                        pod::payloads::windowMouseWheel wheelEvent {
                            { "window:Mouse.Wheel", "os" },
                            { { (unsigned)pos.x, (unsigned)pos.y }, +120 }
                        };
                        this->pushEvent(wheelEvent.type, wheelEvent);
                        continue;
                    }
                    case 5: { // wheel down
                        pod::payloads::windowMouseWheel wheelEvent {
                            { "window:Mouse.Wheel", "os" },
                            { { (unsigned)pos.x, (unsigned)pos.y }, -120 }
                        };
                        this->pushEvent(wheelEvent.type, wheelEvent);
                        continue;
                    }
                }

                pod::payloads::windowMouseClick event{
                    { "window:Mouse.Click", "os" },
                    { pos, {0,0}, button, state }
                };
                this->pushEvent(event.type, event);
            } break;

            case ButtonRelease: {
                pod::Vector2i pos = { ev.xbutton.x, ev.xbutton.y };
                int state = 1;
                uf::stl::string button;

                switch (ev.xbutton.button) {
                    case 1: button = "Left"; break;
                    case 2: button = "Middle"; break;
                    case 3: button = "Right"; break;
                    case 8:
                    case 9: button = "Aux"; break;
                }

                pod::payloads::windowMouseClick event{
                    { "window:Mouse.Click", "os" },
                    { pos, {0,0}, button, state }
                };
                this->pushEvent(event.type, event);
            } break;
            case MotionNotify: {
                static pod::Vector2i lastPosition{};
                pod::Vector2i current = { ev.xmotion.x, ev.xmotion.y };

                // Hard clamp inside window bounds
                auto winSize = getSize();
                bool clamped = false;
                if (current.x < 0) { current.x = 0; clamped = true; }
                if (current.y < 0) { current.y = 0; clamped = true; }
                if (current.x >= (int)winSize.x) { current.x = winSize.x-1;
clamped = true; } if (current.y >= (int)winSize.y) { current.y = winSize.y-1;
clamped = true; }

                if (clamped) {
                    // Warp pointer back into client area
                    XWarpPointer(m_display, None, m_handle, 0,0,0,0, current.x,
current.y); XFlush(m_display);
                }

                // Fire payload as usual
                pod::payloads::windowMouseMoved event {
                    { { "window:Mouse.Moved", "client" }, { winSize } },
                    { current, current - lastPosition, 0 }
                };
                this->pushEvent(event.type, event);

                lastPosition = current;
            } break;
            case ClientMessage: {
                if ((Atom)ev.xclient.data.l[0] == XInternAtom(m_display,
"WM_DELETE_WINDOW", False)) { pod::payloads::windowEvent event {
"window:Closed", "os" }; this->pushEvent(event.type, event); this->terminate();
                }
            } break;
        }
    }
}
*/
void spec::x11::Window::processEvents() {
	while (XPending(m_display) > 0) {
		XEvent ev;
		XNextEvent(m_display, &ev);

		switch (ev.type) {
			case ConfigureNotify: {
				vector_t newSize = {(unsigned)ev.xconfigure.width,
				                    (unsigned)ev.xconfigure.height};
				if (newSize != m_lastSize) {
					m_lastSize = newSize;
					pod::payloads::windowResized event{{"window:Resized", "os"},
					                                   {m_lastSize}};
					this->pushEvent(event.type, event);
				} else {
					pod::payloads::windowEvent moved{"window:Moved", "os"};
					this->pushEvent(moved.type, moved);
				}
			} break;

			case ClientMessage: {
				Atom wmDelete =
				    XInternAtom(m_display, "WM_DELETE_WINDOW", False);
				if ((Atom)ev.xclient.data.l[0] == wmDelete) {
					pod::payloads::windowEvent event{"window:Closed", "os"};
					this->pushEvent(event.type, event);
					this->terminate();
				}
			} break;

			case FocusIn: {
				pod::payloads::windowFocusedChanged focusEvent{
				    {"window:Focus.Changed", "os"}, {1}};
				this->pushEvent(focusEvent.type, focusEvent);
			} break;

			case FocusOut: {
				pod::payloads::windowFocusedChanged focusEvent{
				    {"window:Focus.Changed", "os"}, {-1}};
				this->pushEvent(focusEvent.type, focusEvent);
			} break;

			case KeyPress: {
				KeySym sym;
				char buffer[32];
				Status status;
				int len = Xutf8LookupString(m_xic, &ev.xkey, buffer,
				                            sizeof(buffer) - 1, &sym, &status);
				buffer[len] = '\0';

				uf::stl::string keyName = NormalizeKeyName(KeySymToString(sym));

				pod::payloads::windowKey keyEvent{
				    {"window:Key", "os"},
				    {keyName,
				     (unsigned long)sym,
				     -1,  // pressed
				     false,
				     {.alt = (ev.xkey.state & Mod1Mask),
				      .ctrl = (ev.xkey.state & ControlMask),
				      .shift = (ev.xkey.state & ShiftMask),
				      .sys = (ev.xkey.state & Mod4Mask)}}};
				this->pushEvent(keyEvent.type, keyEvent);
				this->pushEvent(keyEvent.type + "." + keyName, keyEvent);

				if (len > 0) {
					uf::stl::string utf8(buffer);
					uint32_t codepoint =
					    (uint8_t)utf8[0];  // simple, but can be upgraded
					pod::payloads::windowTextEntered textEvent{
					    {"window:Text.Entered", "os"}, {codepoint, utf8}};
					this->pushEvent(textEvent.type, textEvent);
				}
			} break;

			case KeyRelease: {
				KeySym sym = XLookupKeysym(&ev.xkey, 0);
				uf::stl::string keyName = NormalizeKeyName(KeySymToString(sym));

				pod::payloads::windowKey keyEvent{
				    {"window:Key", "os"},
				    {keyName,
				     (unsigned long)sym,
				     1,  // released
				     false,
				     {.alt = (ev.xkey.state & Mod1Mask),
				      .ctrl = (ev.xkey.state & ControlMask),
				      .shift = (ev.xkey.state & ShiftMask),
				      .sys = (ev.xkey.state & Mod4Mask)}}};
				this->pushEvent(keyEvent.type, keyEvent);
				this->pushEvent(keyEvent.type + "." + keyName, keyEvent);
			} break;

			case ButtonPress: {
				pod::Vector2i pos = {ev.xbutton.x, ev.xbutton.y};

				if (ev.xbutton.button == 4) {  // wheel up
					pod::payloads::windowMouseWheel w{
					    {"window:Mouse.Wheel", "os"},
					    {{(unsigned)pos.x, (unsigned)pos.y},
					     ::lastMouseWheel = +120}};
					this->pushEvent(w.type, w);
					break;
				}
				if (ev.xbutton.button == 5) {  // wheel down
					pod::payloads::windowMouseWheel w{
					    {"window:Mouse.Wheel", "os"},
					    {{(unsigned)pos.x, (unsigned)pos.y},
					     ::lastMouseWheel = -120}};
					this->pushEvent(w.type, w);
					break;
				}

				uf::stl::string button = "?";
				if (ev.xbutton.button == 1) button = "Left";
				if (ev.xbutton.button == 2) button = "Middle";
				if (ev.xbutton.button == 3) button = "Right";

				pod::payloads::windowMouseClick click{
				    {"window:Mouse.Click", "os"}, {pos, {0, 0}, button, -1}};
				this->pushEvent(click.type, click);
			} break;

			case ButtonRelease: {
				pod::Vector2i pos = {ev.xbutton.x, ev.xbutton.y};
				uf::stl::string button = "?";
				if (ev.xbutton.button == 1) button = "Left";
				if (ev.xbutton.button == 2) button = "Middle";
				if (ev.xbutton.button == 3) button = "Right";

				pod::payloads::windowMouseClick click{
				    {"window:Mouse.Click", "os"}, {pos, {0, 0}, button, 1}};
				this->pushEvent(click.type, click);
			} break;

			case MotionNotify: {
				static pod::Vector2i last{};
				pod::Vector2i current = {ev.xmotion.x, ev.xmotion.y};

				pod::payloads::windowMouseMoved move{
				    {{"window:Mouse.Moved", "client"}, {getSize()}},
				    {current, current - last, 0}};

				if ( current == last ) break;

				this->pushEvent(move.type, move);

				last = current;
			} break;
		}
	}
}

bool spec::x11::Window::pollEvents(bool block) {
	/*
	if (block) {
	    // Block until one event is received
	    XEvent ev;
	    XNextEvent(m_display, &ev);  // this blocks
	    XPutBackEvent(m_display, &ev); // push back into event queue
	}
	*/
	if (block && XPending(m_display) == 0) {
		int fd = ConnectionNumber(m_display);
		fd_set in_fds;
		FD_ZERO(&in_fds);
		FD_SET(fd, &in_fds);
		select(fd + 1, &in_fds, NULL, NULL,
		       NULL);  // block until input available
	}

	// Process whatever is pending now
	this->processEvents();

	// Dispatch hook events from m_events (matching Win32 style)
	while (!this->m_events.empty()) {
		auto &event = this->m_events.front();
		uf::hooks.call("window:Event", event.payload);
		uf::hooks.call(event.name, event.payload);
		this->m_events.pop();
	}
	return true;
}

pod::Vector2ui spec::x11::Window::getResolution() {
	auto m_display = ::globalDisplay;
	auto m_screen = DefaultScreen(m_display);

	auto monitors = getMonitors(m_display, m_screen);
	if (monitors.empty())
		return {(unsigned)DisplayWidth(m_display, m_screen),
		        (unsigned)DisplayHeight(m_display, m_screen)};

	// Use primary monitor
	for (auto &mon : monitors) {
		if (mon.primary) return {(unsigned)mon.width, (unsigned)mon.height};
	}

	// Fallback: first monitor
	return {(unsigned)monitors[0].width, (unsigned)monitors[0].height};
}

/*
void spec::x11::Window::toggleFullscreen(bool borderless) {
    Atom wm_state	  = XInternAtom(m_display, "_NET_WM_STATE", False);
    Atom fullscreen	= XInternAtom(m_display, "_NET_WM_STATE_FULLSCREEN", False);

    XEvent xev{};
    xev.type = ClientMessage;
    xev.xclient.window = m_handle;
    xev.xclient.message_type = wm_state;
    xev.xclient.format = 32;
    xev.xclient.data.l[0] = 2; // _NET_WM_STATE_TOGGLE
    xev.xclient.data.l[1] = fullscreen;
    xev.xclient.data.l[2] = 0; // No second property
    xev.xclient.data.l[3] = 1; // Source indication: application
    xev.xclient.data.l[4] = 0;

    XSendEvent(m_display, DefaultRootWindow(m_display), False,
               SubstructureRedirectMask | SubstructureNotifyMask, &xev);
    XFlush(m_display);
}
*/

void spec::x11::Window::toggleFullscreen(bool borderless) {
	Atom wmState = XInternAtom(m_display, "_NET_WM_STATE", False);
	Atom wmFullscreen =
	    XInternAtom(m_display, "_NET_WM_STATE_FULLSCREEN", False);
	Atom wmMonitors =
	    XInternAtom(m_display, "_NET_WM_FULLSCREEN_MONITORS", False);

	// Find the monitor where the window currently resides
	auto monitors = getMonitors(m_display, m_screen);
	if (monitors.empty()) return;

	XWindowAttributes attr;
	XGetWindowAttributes(m_display, m_handle, &attr);
	int winCenterX = attr.x + attr.width / 2;
	int winCenterY = attr.y + attr.height / 2;

	int top = 0, bottom = 0, left = 0, right = 0;
	for (size_t i = 0; i < monitors.size(); i++) {
		auto &mon = monitors[i];
		if (winCenterX >= mon.x && winCenterX < mon.x + mon.width &&
		    winCenterY >= mon.y && winCenterY < mon.y + mon.height) {
			// Select this monitor index
			top = bottom = left = right = i;
			break;
		}
	}

	// Set which monitor to fullscreen into
	XEvent monEvent{};
	monEvent.type = ClientMessage;
	monEvent.xclient.window = m_handle;
	monEvent.xclient.message_type = wmMonitors;
	monEvent.xclient.format = 32;
	monEvent.xclient.data.l[0] = top;     // top monitor index
	monEvent.xclient.data.l[1] = bottom;  // bottom monitor index
	monEvent.xclient.data.l[2] = left;    // left monitor index
	monEvent.xclient.data.l[3] = right;   // right monitor index
	monEvent.xclient.data.l[4] = 0;

	XSendEvent(m_display, DefaultRootWindow(m_display), False,
	           SubstructureRedirectMask | SubstructureNotifyMask, &monEvent);

	// Toggle fullscreen
	XEvent ev{};
	ev.type = ClientMessage;
	ev.xclient.window = m_handle;
	ev.xclient.message_type = wmState;
	ev.xclient.format = 32;
	ev.xclient.data.l[0] = 2;  // _NET_WM_STATE_TOGGLE
	ev.xclient.data.l[1] = wmFullscreen;
	ev.xclient.data.l[2] = 0;
	ev.xclient.data.l[3] = 1;  // application source
	ev.xclient.data.l[4] = 0;

	XSendEvent(m_display, DefaultRootWindow(m_display), False,
	           SubstructureRedirectMask | SubstructureNotifyMask, &ev);

	if (borderless) {
		Atom wmHints = XInternAtom(m_display, "_MOTIF_WM_HINTS", True);
		if (wmHints) {
			struct MotifHints {
				unsigned long flags;
				unsigned long functions;
				unsigned long decorations;
				long inputMode;
				unsigned long status;
			};

			MotifHints hints;
			hints.flags = 2;        // Decorations
			hints.decorations = 0;  // No borders/title
			XChangeProperty(m_display, m_handle, wmHints, wmHints, 32,
			                PropModeReplace, (unsigned char *)&hints, 5);
		}
	}

	XFlush(m_display);
}

void spec::x11::Window::display() {
#if UF_USE_OPENGL
#if UF_OPENGL_CONTEXT_IN_WINDOW
	if (m_context) {
		spec::Context *context = (spec::Context *)this->m_context;
		if (context->setActive(true)) context->display();
	}
#else
	glXSwapBuffers(m_display, m_handle);
#endif
#endif
}

/*

#if UF_USE_OPENGL
void spec::x11::Window::createGLContext() {
    static int visualAttribs[] = {
        GLX_RGBA,
        GLX_DOUBLEBUFFER,
        GLX_DEPTH_SIZE, 24,
        GLX_STENCIL_SIZE, 8,
        None
    };

    int screen = DefaultScreen(m_display);
    XVisualInfo* vi = glXChooseVisual(m_display, screen, visualAttribs);
    if (!vi) {
        UF_EXCEPTION("Failed to choose X11 GLX visual");
    }

    GLXContext ctx = glXCreateContext(m_display, vi, NULL, GL_TRUE);
    if (!ctx) {
        UF_EXCEPTION("Failed to create GLX context");
    }

    glXMakeCurrent(m_display, m_handle, ctx);
    this->m_context = ctx;
}

void spec::x11::Window::display() {
    if (m_context) {
        glXSwapBuffers(m_display, m_handle);
    }
}
#endif

*/

#if UF_USE_VULKAN
uf::stl::vector<uf::stl::string> spec::x11::Window::getExtensions(
    bool validationEnabled) {
	uf::stl::vector<uf::stl::string> exts = {
	    VK_KHR_SURFACE_EXTENSION_NAME, VK_KHR_XLIB_SURFACE_EXTENSION_NAME};
	if (validationEnabled) exts.push_back(VK_EXT_DEBUG_UTILS_EXTENSION_NAME);
	return exts;
}

void spec::x11::Window::createSurface(VkInstance instance,
                                      VkSurfaceKHR &surface) {
	VkXlibSurfaceCreateInfoKHR info{};
	info.sType = VK_STRUCTURE_TYPE_XLIB_SURFACE_CREATE_INFO_KHR;
	info.dpy = m_display;
	info.window = m_handle;

	VkResult result =
	    vkCreateXlibSurfaceKHR(instance, &info, nullptr, &surface);
	if (result != VK_SUCCESS) {
		UF_EXCEPTION("Failed to create Xlib Vulkan surface");
	}
}
#endif

#endif