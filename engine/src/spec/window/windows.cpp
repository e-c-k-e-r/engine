#include <uf/spec/window/window.h>
#include <uf/utils/io/iostream.h>
#include <uf/utils/string/utf.h>

#if defined(UF_ENV_WINDOWS) && (!defined(UF_USE_SFML) || (defined(UF_USE_SFML) && UF_USE_SFML == 0))
namespace {
	int windowCount 		= 0;
	std::wstring className 	= L"uf::Window::Class";
	void* fullscreenWindow 	= NULL;
	void setProcessDpiAware() {
		HINSTANCE shCoreDll = LoadLibrary("Shcore.dll");
		if (shCoreDll) {
			enum ProcessDpiAwareness {
				ProcessDpiUnaware		 = 0,
				ProcessSystemDpiAware	 = 1,
				ProcessPerMonitorDpiAware = 2
			};

			typedef HRESULT (WINAPI* SetProcessDpiAwarenessFuncType)(ProcessDpiAwareness);
			SetProcessDpiAwarenessFuncType SetProcessDpiAwarenessFunc = reinterpret_cast<SetProcessDpiAwarenessFuncType>(GetProcAddress(shCoreDll, "SetProcessDpiAwareness"));
			if (SetProcessDpiAwarenessFunc) {
				if (SetProcessDpiAwarenessFunc(ProcessSystemDpiAware) == E_INVALIDARG) {
					uf::iostream << "Failed to set process DPI awareness" << "\n";
				} else {
					FreeLibrary(shCoreDll);
					return;
				}
			}
			FreeLibrary(shCoreDll);
		}
		HINSTANCE user32Dll = LoadLibrary("user32.dll");
		if (user32Dll) {
			typedef BOOL (WINAPI* SetProcessDPIAwareFuncType)(void);
			SetProcessDPIAwareFuncType SetProcessDPIAwareFunc = reinterpret_cast<SetProcessDPIAwareFuncType>(GetProcAddress(user32Dll, "SetProcessDPIAware"));
			if (SetProcessDPIAwareFunc) {
				if (!SetProcessDPIAwareFunc())
					uf::iostream << "Failed to set process DPI awareness" << "\n";
			}
			FreeLibrary(user32Dll);
		}
	}
	LRESULT CALLBACK globalOnEvent(spec::win32::Window::handle_t handle, UINT message, WPARAM wParam, LPARAM lParam) {
		if (message == WM_CREATE) {
			LONG_PTR window = (LONG_PTR)reinterpret_cast<CREATESTRUCT*>(lParam)->lpCreateParams;
			SetWindowLongPtrW(handle, GWLP_USERDATA, window);
		}
		spec::win32::Window* window = handle ? reinterpret_cast<spec::win32::Window*>(GetWindowLongPtr(handle, GWLP_USERDATA)) : NULL;
		if (window) {
			window->processEvent(message, wParam, lParam);
			if (window->m_callback) return CallWindowProcW(reinterpret_cast<WNDPROC>(window->m_callback), handle, message, wParam, lParam);
		}
		if (message == WM_CLOSE) return 0;
		if ((message == WM_SYSCOMMAND) && (wParam == SC_KEYMENU)) return 0;
		return DefWindowProcW(handle, message, wParam, lParam);
	}
}

UF_API_CALL spec::win32::Window::Window() : 
	m_handle 			(NULL),
	m_callback			(0),
	m_cursor			(NULL),
	m_icon				(NULL),
	m_lastSize			({}),
	m_keyRepeatEnabled	(true),
	m_resizing			(false),
	m_mouseInside		(false),
	m_mouseGrabbed		(false),
	m_syncParse			(false),
	m_asyncParse		(false)
{
}
UF_API_CALL spec::win32::Window::Window( spec::win32::Window::handle_t handle ) :
	m_handle 			(handle),
	m_callback			(0),
	m_cursor			(NULL),
	m_icon				(NULL),
	m_lastSize			({}),
	m_keyRepeatEnabled	(true),
	m_resizing			(false),
	m_mouseInside		(false),
	m_mouseGrabbed		(false),
	m_syncParse			(false),
	m_asyncParse		(false)
{
	if ( handle ) {
		SetWindowLongPtrW(this->m_handle, GWLP_USERDATA, reinterpret_cast<LONG_PTR>(this));
		m_callback = SetWindowLongPtrW(this->m_handle, GWLP_WNDPROC, reinterpret_cast<LONG_PTR>(&::globalOnEvent));
	}
}
UF_API_CALL spec::win32::Window::Window( const spec::win32::Window::vector_t& size, const spec::win32::Window::title_t& title ) : 
	m_handle 			(NULL),
	m_callback			(0),
	m_cursor			(NULL),
	m_icon				(NULL),
	m_keyRepeatEnabled	(true),
	m_resizing			(false),
	m_mouseInside		(false),
	m_mouseGrabbed		(false),
	m_syncParse			(false),
	m_asyncParse		(false)
{
	this->create(size, title);
}
void UF_API_CALL spec::win32::Window::create( const spec::win32::Window::vector_t& size, const spec::win32::Window::title_t& title ) {
	setProcessDpiAware();
	if ( windowCount == 0 ) this->registerWindowClass();

	HDC screenDC = GetDC(NULL);
	spec::win32::Window::vector_t position;
	position.x = (uint) ((GetDeviceCaps(screenDC, HORZRES) - size.x ) / 2);
	position.y = (uint) ((GetDeviceCaps(screenDC, VERTRES) - size.y ) / 2);
	ReleaseDC(NULL, screenDC);

	DWORD winStyle = WS_VISIBLE | WS_CAPTION | WS_MINIMIZEBOX | WS_MAXIMIZEBOX | WS_THICKFRAME | WS_SYSMENU;

	RECT rectangle = { 0, 0, size.x, size.y };
	AdjustWindowRect( &rectangle, winStyle, false );

	int width = rectangle.right - rectangle.left;
	int height = rectangle.bottom - rectangle.top;

	this->m_handle = CreateWindowW(
		className.c_str(),
		std::wstring(title).c_str(),
		winStyle,
		position.x,
		position.y,
		width,
		height,
		NULL,
		NULL,
		GetModuleHandle(NULL),
		this
	);
	this->setSize( size );

	// m_callback = SetWindowLongPtrW(this->m_handle, GWLP_WNDPROC, reinterpret_cast<LONG_PTR>(&::globalOnEvent));

	++windowCount;
}

spec::win32::Window::~Window() {
	if ( this->m_icon ) DestroyIcon(this->m_icon);
	if ( !this->m_callback ) {
		if ( this->m_handle ) DestroyWindow(this->m_handle);
		if ( --windowCount == 0 ) UnregisterClassW( className.c_str(), GetModuleHandleW(NULL) );
	} else {
		SetWindowLongPtrW( this->m_handle, GWLP_WNDPROC, this->m_callback );
	}
}
void UF_API_CALL spec::win32::Window::terminate() {
	if ( this == (spec::win32::Window*) fullscreenWindow ) {
		ChangeDisplaySettingsW(NULL, 0);
		fullscreenWindow = NULL;
	}
	this->setCursorVisible(true);
	this->setTracking(false);
	ReleaseCapture();
}

spec::win32::Window::handle_t UF_API_CALL spec::win32::Window::getHandle() const {
	return this->m_handle;
}
spec::win32::Window::vector_t UF_API_CALL spec::win32::Window::getPosition() const {
	RECT rectangle;
	GetWindowRect( this->m_handle, &rectangle );
	spec::win32::Window::vector_t vec;
	vec.x = rectangle.left;
	vec.y = rectangle.top;
	return vec;
}
spec::win32::Window::vector_t UF_API_CALL spec::win32::Window::getSize() const {
	RECT rectangle;
	GetClientRect( this->m_handle, &rectangle );
	spec::win32::Window::vector_t vec;
	vec.x = rectangle.right - rectangle.left;
	vec.y = rectangle.bottom - rectangle.top;
	return vec;
}

void UF_API_CALL spec::win32::Window::centerWindow() {
	if ( fullscreenWindow == (void*) this ) return;
	RECT rect;
	GetWindowRect ( this->m_handle, &rect ) ;
	
	pod::Vector2i offset = {
		(GetSystemMetrics(SM_CXSCREEN)-rect.right),
		(GetSystemMetrics(SM_CYSCREEN)-rect.bottom)
	};
	offset /= 2;
	SetWindowPos( this->m_handle, 0, offset.x, offset.y, 0, 0, SWP_NOZORDER | SWP_NOSIZE );
}
void UF_API_CALL spec::win32::Window::setPosition( const spec::win32::Window::vector_t& position ) {
	if ( fullscreenWindow == (void*) this ) return;
	SetWindowPos(this->m_handle, NULL, position.x, position.y, 0, 0, SWP_NOSIZE | SWP_NOZORDER);
}
void UF_API_CALL spec::win32::Window::setMousePosition( const spec::win32::Window::vector_t& position ) {
	POINT pt = { position.x, position.y };
	ClientToScreen(this->m_handle, &pt);
	ReleaseCapture();
	SetCursorPos(pt.x,pt.y);
	SetCapture(this->m_handle);
}
spec::win32::Window::vector_t UF_API_CALL spec::win32::Window::getMousePosition( ) {
	POINT pt;
	GetCursorPos( &pt );
	return { pt.x, pt.y };
}
void UF_API_CALL spec::win32::Window::setSize( const spec::win32::Window::vector_t& size ) {
	if ( fullscreenWindow == (void*) this ) return;
	RECT rectangle = { 0, 0, size.x, size.y };
	AdjustWindowRect( &rectangle, GetWindowLong(this->m_handle, GWL_STYLE), false );
	SetWindowPos(this->m_handle, NULL, 0, 0, rectangle.right - rectangle.left, rectangle.bottom - rectangle.top, SWP_NOMOVE | SWP_NOZORDER);
}

void UF_API_CALL spec::win32::Window::setTitle( const spec::win32::Window::title_t& title ) {
	SetWindowTextW(this->m_handle, std::wstring(title).c_str());
//	SetWindowTextW(this->m_handle, (wchar_t*) std::wstring(title).c_str());
}
void UF_API_CALL spec::win32::Window::setIcon( const spec::win32::Window::vector_t& size, uint8_t* pixels ) {
	// First destroy the previous one

	std::vector<uint8_t> iconPixels(size.x * size.y * 4);
	for (std::size_t i = 0; i < iconPixels.size() / 4; ++i) {
		iconPixels[i * 4 + 0] = pixels[i * 4 + 2];
		iconPixels[i * 4 + 1] = pixels[i * 4 + 1];
		iconPixels[i * 4 + 2] = pixels[i * 4 + 0];
		iconPixels[i * 4 + 3] = pixels[i * 4 + 3];
	}
	this->m_icon = CreateIcon(GetModuleHandleW(NULL), size.x, size.y, 1, 32, NULL, &iconPixels[0]);
	if (this->m_icon) {
		SendMessageW(m_handle, WM_SETICON, ICON_BIG,   (LPARAM)this->m_icon);
		SendMessageW(m_handle, WM_SETICON, ICON_SMALL, (LPARAM)this->m_icon);
	}
/*
	if ( this->m_icon ) DestroyIcon(this->m_icon);
	
	// RGBA -> BGRA
	std::vector<uint8_t> icon( uf::vector::product(size) * 4 );
	for ( std::size_t i = 0; i < icon.size() / 4; ++i ) {
		icon[i * 4 + 0] = pixels[i * 4 + 2];
		icon[i * 4 + 1] = pixels[i * 4 + 1];
		icon[i * 4 + 2] = pixels[i * 4 + 0];
		icon[i * 4 + 3] = pixels[i * 4 + 3];
	}
	this->m_icon = CreateIcon( GetModuleHandleW(NULL), size.x, size.y, 1, 32, NULL, &icon[0] );
	if ( this->m_icon ) {
		SendMessageW(this->m_handle, WM_SETICON, ICON_BIG,	(LPARAM) m_icon);
		SendMessageW(this->m_handle, WM_SETICON, ICON_SMALL, (LPARAM) m_icon);
	}
*/
}
void UF_API_CALL spec::win32::Window::setVisible( bool visibility ) {
	ShowWindow(this->m_handle, visibility ? SW_SHOW : SW_HIDE);
}
void UF_API_CALL spec::win32::Window::setCursorVisible( bool visibility ) {
	this->m_cursor = ( visibility ? LoadCursor(NULL, IDC_ARROW) : NULL );
	SetCursor(this->m_cursor);
}
void UF_API_CALL spec::win32::Window::setKeyRepeatEnabled( bool state ) {
	this->m_keyRepeatEnabled = state;
}

void UF_API_CALL spec::win32::Window::requestFocus() {
	DWORD thisPid = GetWindowThreadProcessId(this->m_handle, NULL);
	DWORD forePid = GetWindowThreadProcessId(GetForegroundWindow(), NULL);

	if ( thisPid == forePid ) {
		SetForegroundWindow(this->m_handle);
	} else {
		FLASHWINFO info;
		info.cbSize 	= sizeof(info);
		info.hwnd 		= this->m_handle;
		info.dwFlags 	= FLASHW_TRAY;
		info.dwTimeout 	= 0;
		info.uCount 	= 3;

		FlashWindowEx(&info);
	}
}
bool UF_API_CALL spec::win32::Window::hasFocus() const {
	return this->m_handle == GetForegroundWindow();
}


#include <uf/utils/serialize/serializer.h>
void UF_API_CALL spec::win32::Window::processEvents() {
	if ( !this->m_callback ) {
		MSG message;
		while ( PeekMessageW( &message, NULL, 0, 0, PM_REMOVE ) ) {
			TranslateMessage(&message);
			DispatchMessage(&message);
		}
	}
	/* Key inputs */ if ( this->m_asyncParse ) {
		std::vector<WPARAM> keys;

		if ( GetAsyncKeyState('A') & 0x8000 ) keys.push_back('A'); // keys.push_back(this->getKey('A', 0));
		if ( GetAsyncKeyState('B') & 0x8000 ) keys.push_back('B'); // keys.push_back(this->getKey('B', 0));
		if ( GetAsyncKeyState('C') & 0x8000 ) keys.push_back('C'); // keys.push_back(this->getKey('C', 0));
		if ( GetAsyncKeyState('D') & 0x8000 ) keys.push_back('D'); // keys.push_back(this->getKey('D', 0));
		if ( GetAsyncKeyState('E') & 0x8000 ) keys.push_back('E'); // keys.push_back(this->getKey('E', 0));
		if ( GetAsyncKeyState('F') & 0x8000 ) keys.push_back('F'); // keys.push_back(this->getKey('F', 0));
		if ( GetAsyncKeyState('G') & 0x8000 ) keys.push_back('G'); // keys.push_back(this->getKey('G', 0));
		if ( GetAsyncKeyState('H') & 0x8000 ) keys.push_back('H'); // keys.push_back(this->getKey('H', 0));
		if ( GetAsyncKeyState('I') & 0x8000 ) keys.push_back('I'); // keys.push_back(this->getKey('I', 0));
		if ( GetAsyncKeyState('J') & 0x8000 ) keys.push_back('J'); // keys.push_back(this->getKey('J', 0));
		if ( GetAsyncKeyState('K') & 0x8000 ) keys.push_back('K'); // keys.push_back(this->getKey('K', 0));
		if ( GetAsyncKeyState('L') & 0x8000 ) keys.push_back('L'); // keys.push_back(this->getKey('L', 0));
		if ( GetAsyncKeyState('M') & 0x8000 ) keys.push_back('M'); // keys.push_back(this->getKey('M', 0));
		if ( GetAsyncKeyState('N') & 0x8000 ) keys.push_back('N'); // keys.push_back(this->getKey('N', 0));
		if ( GetAsyncKeyState('O') & 0x8000 ) keys.push_back('O'); // keys.push_back(this->getKey('O', 0));
		if ( GetAsyncKeyState('P') & 0x8000 ) keys.push_back('P'); // keys.push_back(this->getKey('P', 0));
		if ( GetAsyncKeyState('Q') & 0x8000 ) keys.push_back('Q'); // keys.push_back(this->getKey('Q', 0));
		if ( GetAsyncKeyState('R') & 0x8000 ) keys.push_back('R'); // keys.push_back(this->getKey('R', 0));
		if ( GetAsyncKeyState('S') & 0x8000 ) keys.push_back('S'); // keys.push_back(this->getKey('S', 0));
		if ( GetAsyncKeyState('T') & 0x8000 ) keys.push_back('T'); // keys.push_back(this->getKey('T', 0));
		if ( GetAsyncKeyState('U') & 0x8000 ) keys.push_back('U'); // keys.push_back(this->getKey('U', 0));
		if ( GetAsyncKeyState('V') & 0x8000 ) keys.push_back('V'); // keys.push_back(this->getKey('V', 0));
		if ( GetAsyncKeyState('W') & 0x8000 ) keys.push_back('W'); // keys.push_back(this->getKey('W', 0));
		if ( GetAsyncKeyState('X') & 0x8000 ) keys.push_back('X'); // keys.push_back(this->getKey('X', 0));
		if ( GetAsyncKeyState('Y') & 0x8000 ) keys.push_back('Y'); // keys.push_back(this->getKey('Y', 0));
		if ( GetAsyncKeyState('Z') & 0x8000 ) keys.push_back('Z'); // keys.push_back(this->getKey('Z', 0));
		if ( GetAsyncKeyState('0') & 0x8000 ) keys.push_back('0'); // keys.push_back(this->getKey('0', 0));
		if ( GetAsyncKeyState('1') & 0x8000 ) keys.push_back('1'); // keys.push_back(this->getKey('1', 0));
		if ( GetAsyncKeyState('2') & 0x8000 ) keys.push_back('2'); // keys.push_back(this->getKey('2', 0));
		if ( GetAsyncKeyState('3') & 0x8000 ) keys.push_back('3'); // keys.push_back(this->getKey('3', 0));
		if ( GetAsyncKeyState('4') & 0x8000 ) keys.push_back('4'); // keys.push_back(this->getKey('4', 0));
		if ( GetAsyncKeyState('5') & 0x8000 ) keys.push_back('5'); // keys.push_back(this->getKey('5', 0));
		if ( GetAsyncKeyState('6') & 0x8000 ) keys.push_back('6'); // keys.push_back(this->getKey('6', 0));
		if ( GetAsyncKeyState('7') & 0x8000 ) keys.push_back('7'); // keys.push_back(this->getKey('7', 0));
		if ( GetAsyncKeyState('8') & 0x8000 ) keys.push_back('8'); // keys.push_back(this->getKey('8', 0));
		if ( GetAsyncKeyState('9') & 0x8000 ) keys.push_back('9'); // keys.push_back(this->getKey('9', 0));
		if ( GetAsyncKeyState(VK_ESCAPE) & 0x8000 ) keys.push_back(VK_ESCAPE); // keys.push_back(this->getKey(VK_ESCAPE, 0));
		if ( GetAsyncKeyState(VK_LCONTROL) & 0x8000 ) keys.push_back(VK_LCONTROL); // keys.push_back(this->getKey(VK_LCONTROL, 0));
		if ( GetAsyncKeyState(VK_LSHIFT) & 0x8000 ) keys.push_back(VK_LSHIFT); // keys.push_back(this->getKey(VK_LSHIFT, 0));
		if ( GetAsyncKeyState(VK_LMENU) & 0x8000 ) keys.push_back(VK_LMENU); // keys.push_back(this->getKey(VK_LMENU, 0));
		if ( GetAsyncKeyState(VK_LWIN) & 0x8000 ) keys.push_back(VK_LWIN); // keys.push_back(this->getKey(VK_LWIN, 0));
		if ( GetAsyncKeyState(VK_RCONTROL) & 0x8000 ) keys.push_back(VK_RCONTROL); // keys.push_back(this->getKey(VK_RCONTROL, 0));
		if ( GetAsyncKeyState(VK_RSHIFT) & 0x8000 ) keys.push_back(VK_RSHIFT); // keys.push_back(this->getKey(VK_RSHIFT, 0));
		if ( GetAsyncKeyState(VK_RMENU) & 0x8000 ) keys.push_back(VK_RMENU); // keys.push_back(this->getKey(VK_RMENU, 0));
		if ( GetAsyncKeyState(VK_RWIN) & 0x8000 ) keys.push_back(VK_RWIN); // keys.push_back(this->getKey(VK_RWIN, 0));
		if ( GetAsyncKeyState(VK_APPS) & 0x8000 ) keys.push_back(VK_APPS); // keys.push_back(this->getKey(VK_APPS, 0));
		if ( GetAsyncKeyState(VK_OEM_4) & 0x8000 ) keys.push_back(VK_OEM_4); // keys.push_back(this->getKey(VK_OEM_4, 0));
		if ( GetAsyncKeyState(VK_OEM_6) & 0x8000 ) keys.push_back(VK_OEM_6); // keys.push_back(this->getKey(VK_OEM_6, 0));
		if ( GetAsyncKeyState(VK_OEM_1) & 0x8000 ) keys.push_back(VK_OEM_1); // keys.push_back(this->getKey(VK_OEM_1, 0));
		if ( GetAsyncKeyState(VK_OEM_COMMA) & 0x8000 ) keys.push_back(VK_OEM_COMMA); // keys.push_back(this->getKey(VK_OEM_COMMA, 0));
		if ( GetAsyncKeyState(VK_OEM_PERIOD) & 0x8000 ) keys.push_back(VK_OEM_PERIOD); // keys.push_back(this->getKey(VK_OEM_PERIOD, 0));
		if ( GetAsyncKeyState(VK_OEM_7) & 0x8000 ) keys.push_back(VK_OEM_7); // keys.push_back(this->getKey(VK_OEM_7, 0));
		if ( GetAsyncKeyState(VK_OEM_2) & 0x8000 ) keys.push_back(VK_OEM_2); // keys.push_back(this->getKey(VK_OEM_2, 0));
		if ( GetAsyncKeyState(VK_OEM_5) & 0x8000 ) keys.push_back(VK_OEM_5); // keys.push_back(this->getKey(VK_OEM_5, 0));
		if ( GetAsyncKeyState(VK_OEM_3) & 0x8000 ) keys.push_back(VK_OEM_3); // keys.push_back(this->getKey(VK_OEM_3, 0));
		if ( GetAsyncKeyState(VK_OEM_PLUS) & 0x8000 ) keys.push_back(VK_OEM_PLUS); // keys.push_back(this->getKey(VK_OEM_PLUS, 0));
		if ( GetAsyncKeyState(VK_OEM_MINUS) & 0x8000 ) keys.push_back(VK_OEM_MINUS); // keys.push_back(this->getKey(VK_OEM_MINUS, 0));
		if ( GetAsyncKeyState(VK_SPACE) & 0x8000 ) keys.push_back(VK_SPACE); // keys.push_back(this->getKey(VK_SPACE, 0));
		if ( GetAsyncKeyState(VK_RETURN) & 0x8000 ) keys.push_back(VK_RETURN); // keys.push_back(this->getKey(VK_RETURN, 0));
		if ( GetAsyncKeyState(VK_BACK) & 0x8000 ) keys.push_back(VK_BACK); // keys.push_back(this->getKey(VK_BACK, 0));
		if ( GetAsyncKeyState(VK_TAB) & 0x8000 ) keys.push_back(VK_TAB); // keys.push_back(this->getKey(VK_TAB, 0));
		if ( GetAsyncKeyState(VK_PRIOR) & 0x8000 ) keys.push_back(VK_PRIOR); // keys.push_back(this->getKey(VK_PRIOR, 0));
		if ( GetAsyncKeyState(VK_NEXT) & 0x8000 ) keys.push_back(VK_NEXT); // keys.push_back(this->getKey(VK_NEXT, 0));
		if ( GetAsyncKeyState(VK_END) & 0x8000 ) keys.push_back(VK_END); // keys.push_back(this->getKey(VK_END, 0));
		if ( GetAsyncKeyState(VK_HOME) & 0x8000 ) keys.push_back(VK_HOME); // keys.push_back(this->getKey(VK_HOME, 0));
		if ( GetAsyncKeyState(VK_INSERT) & 0x8000 ) keys.push_back(VK_INSERT); // keys.push_back(this->getKey(VK_INSERT, 0));
		if ( GetAsyncKeyState(VK_DELETE) & 0x8000 ) keys.push_back(VK_DELETE); // keys.push_back(this->getKey(VK_DELETE, 0));
		if ( GetAsyncKeyState(VK_ADD) & 0x8000 ) keys.push_back(VK_ADD); // keys.push_back(this->getKey(VK_ADD, 0));
		if ( GetAsyncKeyState(VK_SUBTRACT) & 0x8000 ) keys.push_back(VK_SUBTRACT); // keys.push_back(this->getKey(VK_SUBTRACT, 0));
		if ( GetAsyncKeyState(VK_MULTIPLY) & 0x8000 ) keys.push_back(VK_MULTIPLY); // keys.push_back(this->getKey(VK_MULTIPLY, 0));
		if ( GetAsyncKeyState(VK_DIVIDE) & 0x8000 ) keys.push_back(VK_DIVIDE); // keys.push_back(this->getKey(VK_DIVIDE, 0));
		if ( GetAsyncKeyState(VK_LEFT) & 0x8000 ) keys.push_back(VK_LEFT); // keys.push_back(this->getKey(VK_LEFT, 0));
		if ( GetAsyncKeyState(VK_RIGHT) & 0x8000 ) keys.push_back(VK_RIGHT); // keys.push_back(this->getKey(VK_RIGHT, 0));
		if ( GetAsyncKeyState(VK_UP) & 0x8000 ) keys.push_back(VK_UP); // keys.push_back(this->getKey(VK_UP, 0));
		if ( GetAsyncKeyState(VK_DOWN) & 0x8000 ) keys.push_back(VK_DOWN); // keys.push_back(this->getKey(VK_DOWN, 0));
		if ( GetAsyncKeyState(VK_NUMPAD0) & 0x8000 ) keys.push_back(VK_NUMPAD0); // keys.push_back(this->getKey(VK_NUMPAD0, 0));
		if ( GetAsyncKeyState(VK_NUMPAD1) & 0x8000 ) keys.push_back(VK_NUMPAD1); // keys.push_back(this->getKey(VK_NUMPAD1, 0));
		if ( GetAsyncKeyState(VK_NUMPAD2) & 0x8000 ) keys.push_back(VK_NUMPAD2); // keys.push_back(this->getKey(VK_NUMPAD2, 0));
		if ( GetAsyncKeyState(VK_NUMPAD3) & 0x8000 ) keys.push_back(VK_NUMPAD3); // keys.push_back(this->getKey(VK_NUMPAD3, 0));
		if ( GetAsyncKeyState(VK_NUMPAD4) & 0x8000 ) keys.push_back(VK_NUMPAD4); // keys.push_back(this->getKey(VK_NUMPAD4, 0));
		if ( GetAsyncKeyState(VK_NUMPAD5) & 0x8000 ) keys.push_back(VK_NUMPAD5); // keys.push_back(this->getKey(VK_NUMPAD5, 0));
		if ( GetAsyncKeyState(VK_NUMPAD6) & 0x8000 ) keys.push_back(VK_NUMPAD6); // keys.push_back(this->getKey(VK_NUMPAD6, 0));
		if ( GetAsyncKeyState(VK_NUMPAD7) & 0x8000 ) keys.push_back(VK_NUMPAD7); // keys.push_back(this->getKey(VK_NUMPAD7, 0));
		if ( GetAsyncKeyState(VK_NUMPAD8) & 0x8000 ) keys.push_back(VK_NUMPAD8); // keys.push_back(this->getKey(VK_NUMPAD8, 0));
		if ( GetAsyncKeyState(VK_NUMPAD9) & 0x8000 ) keys.push_back(VK_NUMPAD9); // keys.push_back(this->getKey(VK_NUMPAD9, 0));
		if ( GetAsyncKeyState(VK_F1) & 0x8000 ) keys.push_back(VK_F1); // keys.push_back(this->getKey(VK_F1, 0));
		if ( GetAsyncKeyState(VK_F2) & 0x8000 ) keys.push_back(VK_F2); // keys.push_back(this->getKey(VK_F2, 0));
		if ( GetAsyncKeyState(VK_F3) & 0x8000 ) keys.push_back(VK_F3); // keys.push_back(this->getKey(VK_F3, 0));
		if ( GetAsyncKeyState(VK_F4) & 0x8000 ) keys.push_back(VK_F4); // keys.push_back(this->getKey(VK_F4, 0));
		if ( GetAsyncKeyState(VK_F5) & 0x8000 ) keys.push_back(VK_F5); // keys.push_back(this->getKey(VK_F5, 0));
		if ( GetAsyncKeyState(VK_F6) & 0x8000 ) keys.push_back(VK_F6); // keys.push_back(this->getKey(VK_F6, 0));
		if ( GetAsyncKeyState(VK_F7) & 0x8000 ) keys.push_back(VK_F7); // keys.push_back(this->getKey(VK_F7, 0));
		if ( GetAsyncKeyState(VK_F8) & 0x8000 ) keys.push_back(VK_F8); // keys.push_back(this->getKey(VK_F8, 0));
		if ( GetAsyncKeyState(VK_F9) & 0x8000 ) keys.push_back(VK_F9); // keys.push_back(this->getKey(VK_F9, 0));
		if ( GetAsyncKeyState(VK_F10) & 0x8000 ) keys.push_back(VK_F10); // keys.push_back(this->getKey(VK_F10, 0));
		if ( GetAsyncKeyState(VK_F11) & 0x8000 ) keys.push_back(VK_F11); // keys.push_back(this->getKey(VK_F11, 0));
		if ( GetAsyncKeyState(VK_F12) & 0x8000 ) keys.push_back(VK_F12); // keys.push_back(this->getKey(VK_F12, 0));
		if ( GetAsyncKeyState(VK_F13) & 0x8000 ) keys.push_back(VK_F13); // keys.push_back(this->getKey(VK_F13, 0));
		if ( GetAsyncKeyState(VK_F14) & 0x8000 ) keys.push_back(VK_F14); // keys.push_back(this->getKey(VK_F14, 0));
		if ( GetAsyncKeyState(VK_F15) & 0x8000 ) keys.push_back(VK_F15); // keys.push_back(this->getKey(VK_F15, 0));
		if ( GetAsyncKeyState(VK_PAUSE) & 0x8000 ) keys.push_back(VK_PAUSE); // keys.push_back(this->getKey(VK_PAUSE, 0));

   		if ( GetAsyncKeyState(VK_LBUTTON) & 0x8000 ) keys.push_back(VK_LBUTTON); // keys.push_back( this->getKey(VK_LBUTTON, 0) );
		if ( GetAsyncKeyState(VK_RBUTTON) & 0x8000 ) keys.push_back(VK_RBUTTON); // keys.push_back( this->getKey(VK_RBUTTON, 0) );
		if ( GetAsyncKeyState(VK_MBUTTON) & 0x8000 ) keys.push_back(VK_MBUTTON); // keys.push_back( this->getKey(VK_MBUTTON, 0) );
		if ( GetAsyncKeyState(VK_XBUTTON1) & 0x8000 ) keys.push_back(VK_XBUTTON1); // keys.push_back( this->getKey(VK_XBUTTON1, 0) );
		if ( GetAsyncKeyState(VK_XBUTTON2) & 0x8000 ) keys.push_back(VK_XBUTTON2); // keys.push_back( this->getKey(VK_XBUTTON2, 0) );

		struct Event {
			std::string type = "unknown";
			std::string invoker = "???";

			struct {
				int state;
				std::string code;
				uint32_t raw;
				bool async;

				struct {
					bool alt;
					bool ctrl;
					bool shift;
					bool sys;
				} modifier;
			} key;
		};
		Event event; {
			event.type 			= "window:Key";
			event.invoker 		= "window";
			event.key.state 	= -1;
			event.key.code 		= "NULL";
			event.key.raw 		= 0;
			event.key.async 	= true;
			event.key.modifier 	= {	
				.alt		= HIWORD(GetAsyncKeyState(VK_MENU))		!= 0,
				.ctrl 		= HIWORD(GetAsyncKeyState(VK_CONTROL)) 	!= 0,
				.shift		= HIWORD(GetAsyncKeyState(VK_SHIFT))	!= 0,
				.sys  		= HIWORD(GetAsyncKeyState(VK_LWIN)) 	|| HIWORD(GetAsyncKeyState(VK_RWIN)),
			};
		}
		/* Readable (JSON) + Optimal (Userdata) event */ {
			uf::Serializer json;	
			/* Set up JSON data*/ {
				json["type"] 							= event.type + "." + ((event.key.state == -1)?"Pressed":"Released");
				json["invoker"] 						= event.invoker;
				json["key"]["state"] 					= (event.key.state == -1) ? "Down" : "Up";
				json["key"]["async"] 					= event.key.async;
				json["key"]["modifier"]["alt"]			= event.key.modifier.alt;
				json["key"]["modifier"]["control"] 		= event.key.modifier.ctrl;
				json["key"]["modifier"]["shift"]		= event.key.modifier.shift;
				json["key"]["modifier"]["system"]  		= event.key.modifier.sys;
			}
			/* Loop through key inputs */ {	
				for ( auto& key : keys ) {
					auto code = this->getKey(key, 0);
					event.key.code 	= code;
					event.key.raw  	= key;
					this->pushEvent(event.type + "." + code, uf::userdata::create(event));

					json["key"]["code"] = code;
					json["key"]["raw"] = key;
					this->pushEvent(event.type + "." + code, json);
				}
			}
		}
	}
}
bool UF_API_CALL spec::win32::Window::pollEvents( bool block ) {
	/* Empty; look for something! */ if ( this->m_events.readable.empty() && this->m_events.optimal.empty() ) {
		do {
			this->processEvents();
		} while ( block && this->m_events.readable.empty() && this->m_events.optimal.empty() );
	}

	if ( uf::hooks.prefersReadable() ) // Should we handle events in a readable format...
	/* Process events parsed in a readable format */ {
		// process things
		while ( !this->m_events.readable.empty() ) {
			auto& event = this->m_events.readable.front();
				uf::hooks.call( event.name, event.argument );
		/*
			try {
				uf::hooks.call( "window:Event", event.argument );
				uf::hooks.call( event.name, event.argument );
			//	uf::iostream << event.name << "\t" << event.argument << "\n";
			} catch ( ... ) {
				// Let the hook handler handle the exceptions
			}
		*/
			this->m_events.readable.pop();
		}
	//	this->m_events.optimal.clear(); 	// flush queue in case
	} else // ... or in an optimal format?
	/* Process events parsed in an optimal format */ {
		// process things
		while ( !this->m_events.optimal.empty() ) {
			auto& event = this->m_events.optimal.front();
				uf::hooks.call( event.name, event.argument );
		/*
			try {
				uf::hooks.call( "window:Event", event.argument );
				uf::hooks.call( event.name, event.argument );
			//	uf::iostream << event.name << "\t" << event.argument << "\n";
			} catch ( ... ) {
				// Let the hook handler handle the exceptions
			}
		*/
			uf::userdata::destroy(event.argument);
			this->m_events.optimal.pop();
		}
	//	this->m_events.readable.clear(); 	// flush queue in case
	}
	return true;
}

void UF_API_CALL spec::win32::Window::registerWindowClass() {
	WNDCLASSW windowClass;
	windowClass.style 			= 0;
	windowClass.lpfnWndProc 	= &(::globalOnEvent);
	windowClass.cbClsExtra 		= 0;
	windowClass.cbWndExtra 		= 0;
	windowClass.hInstance 		= GetModuleHandleW(NULL);
	windowClass.hIcon 			= NULL;
	windowClass.hCursor 		= 0;
	windowClass.hbrBackground 	= 0;
	windowClass.lpszMenuName 	= NULL;
	windowClass.lpszClassName 	= className.c_str();
	RegisterClassW(&windowClass);
}
void UF_API_CALL spec::win32::Window::processEvent(UINT message, WPARAM wParam, LPARAM lParam) {
	if (!this->m_handle) return;
	
	uf::Serializer json;
	std::string hook = "window:Unknown";
	std::string serialized = "";
	std::stringstream serializer;
	bool labelAsDelta = true;

	switch (message) {
		case WM_DESTROY:
			this->terminate();
		break;
		case WM_SETCURSOR:
			if (LOWORD(lParam) == HTCLIENT) SetCursor(this->m_cursor);
		break;
		case WM_CLOSE: {
			struct Event {
				std::string type = "unknown";
				std::string invoker = "???";
			};
			Event event; {
				event.type = "window:Closed";
				event.invoker = "os";
			};
			this->pushEvent(event.type, uf::userdata::create(event)); {
				json["type"] = event.type;
				json["invoker"] = event.invoker;
				this->pushEvent(event.type, json);
			}
		} break;
		case WM_SIZE: {
			if (wParam != SIZE_MINIMIZED && !this->m_resizing && this->m_lastSize != getSize()) {
				this->m_lastSize = this->getSize();

				struct Event {
					std::string type = "unknown";
					std::string invoker = "???";

					struct {
						pod::Vector2i size;
					} window;
				};
				Event event; {
					event.type = "window:Resized";
					event.invoker = "os";
					event.window.size = this->m_lastSize;
				};
				this->pushEvent(event.type, uf::userdata::create(event)); {
					json["type"] = event.type;
					json["invoker"] = event.invoker;
					json["window"]["size"]["x"] = event.window.size.x;
					json["window"]["size"]["y"] = event.window.size.y;
					this->pushEvent(event.type, json);
				}
				
				this->grabMouse(this->m_mouseGrabbed);
			}
		} break;
		case WM_ENTERSIZEMOVE: {	
			this->m_resizing = true;
			this->grabMouse(false);
		} break;
		case WM_EXITSIZEMOVE:{
			this->m_resizing = false;
			if(this->m_lastSize != this->getSize()) {
				this->m_lastSize = this->getSize();

				struct Event {
					std::string type = "unknown";
					std::string invoker = "???";

					struct {
						pod::Vector2i size;
					} window;
				};
				Event event; {
					event.type = "window:Resized";
					event.invoker = "os";
					event.window.size = this->m_lastSize;
				};
				this->pushEvent(event.type, uf::userdata::create(event)); {
					json["type"] = event.type;
					json["invoker"] = event.invoker;
					json["window"]["size"]["x"] = event.window.size.x;
					json["window"]["size"]["y"] = event.window.size.y;
					this->pushEvent(event.type, json);
				}
			} else {
				struct Event {
					std::string type = "unknown";
					std::string invoker = "???";
				};
				Event event; {
					event.type = "window:Moved";
					event.invoker = "os";
				};
				this->pushEvent(event.type, uf::userdata::create(event)); {
					json["type"] = event.type;
					json["invoker"] = event.invoker;
					this->pushEvent(event.type, json);
				}
			}
			this->grabMouse(this->m_mouseGrabbed);
		} break;
		case WM_GETMINMAXINFO: {
			MINMAXINFO* info = reinterpret_cast<MINMAXINFO*>(lParam);
			info->ptMaxTrackSize.x = 50000;
			info->ptMaxTrackSize.y = 50000;
		} break;
		case WM_KILLFOCUS:
		case WM_SETFOCUS: {
			if ( message == WM_KILLFOCUS ) {
				this->grabMouse(false);
				SetCursor(NULL);
			}
			if ( message == WM_SETFOCUS ) {
				this->grabMouse(this->m_mouseGrabbed);
				SetCursor(this->m_cursor);
			}
			struct Event {
				std::string type = "unknown";
				std::string invoker = "???";

				struct {
					int state = 0;
				} window;
			};
			Event event; {
				event.type = "window:Focus.Changed";
				event.invoker = "os";

				switch ( message ) {
					case WM_SETFOCUS: event.window.state 	=  1; break;
					case WM_KILLFOCUS: event.window.state 	= -1; break;
				}
			};
			this->pushEvent(event.type, uf::userdata::create(event)); {
				json["type"] = event.type;
				json["invoker"] = event.invoker;
				switch ( message ) {
					case WM_SETFOCUS: json["window"]["state"] = "Gained"; break;
					case WM_KILLFOCUS: json["window"]["state"] =  "Lost"; break;
				}
				this->pushEvent(event.type, json);
			}
		} break;
		// Text event
		case WM_CHAR: if ( /*true ||*/ this->m_syncParse ) {
			if (this->m_keyRepeatEnabled || ((lParam & (1 << 30)) == 0)) {
				// Get the code of the typed character
				uint32_t character = static_cast<uint32_t>(wParam);
				// Check if it is the first part of a surrogate pair, or a regular character
				if ((character >= 0xD800) && (character <= 0xDBFF)) {
					// First part of a surrogate pair: store it and wait for the second one
					this->m_surrogate = static_cast<uint16_t>(character);
				}
				else {
					// Check if it is the second part of a surrogate pair, or a regular character
					if ((character >= 0xDC00) && (character <= 0xDFFF)) {
						// Convert the UTF-16 surrogate pair to a single UTF-32 value
						uint16_t utf16[] = {this->m_surrogate, static_cast<uint16_t>(character)};
						uf::Utf16::toUtf32(utf16, utf16 + 2, &character);
						this->m_surrogate = 0;
					}
					std::basic_string<uint32_t> 	utf32; 	utf32+=character;
					std::basic_string<uint8_t> 		utf8; 	utf8.reserve(utf32.length());
					uf::Utf32::toUtf8(utf32.begin(), utf32.end(), std::back_inserter(utf8));
					
					std::stringstream to_hex;
					to_hex << std::hex << character;

					struct Event {
						std::string type = "unknown";
						std::string invoker = "???";

						struct {
							uint32_t utf32;
							std::string unicode;
						} text;
					};
					Event event; {
						event.type = "window:Text.Entered";
						event.invoker = "os";

						event.text.utf32 = character;
						event.text.unicode = std::string(utf8.begin(), utf8.end());
					};
					if ( this->m_syncParse ) {
						this->pushEvent(event.type, uf::userdata::create(event)); 
						json["type"] = event.type;
						json["invoker"] = event.invoker;

						json["text"]["uint32_t"] = event.text.utf32;
						json["text"]["unicode"] 		= std::string(utf8.begin(), utf8.end());
						this->pushEvent(event.type, json);
					}
				}
			}
		} break;
		case WM_KEYDOWN:
		case WM_SYSKEYDOWN:
		case WM_KEYUP:
		case WM_SYSKEYUP: if ( /*true ||*/ this->m_syncParse ) {
			if (this->m_keyRepeatEnabled || ((HIWORD(lParam) & KF_REPEAT) == 0)) {
				struct Event {
					std::string type = "unknown";
					std::string invoker = "???";

					struct {
						int state;
						std::string code;
						uint32_t raw;
						bool async;

						struct {
							bool alt;
							bool ctrl;
							bool shift;
							bool sys;
						} modifier;
					} key;
				};
				Event event; {
					event.type 			= "window:Key",
					event.invoker 		= "window",
					event.key.code 		= this->getKey(wParam, lParam),
					event.key.raw 		= wParam,
					event.key.async 	= false,
					event.key.modifier 	= {	
						.alt		= HIWORD(GetAsyncKeyState(VK_MENU))		!= 0,
						.ctrl 		= HIWORD(GetAsyncKeyState(VK_CONTROL)) 	!= 0,
						.shift		= HIWORD(GetAsyncKeyState(VK_SHIFT))	!= 0,
						.sys  		= HIWORD(GetAsyncKeyState(VK_LWIN)) 	|| HIWORD(GetAsyncKeyState(VK_RWIN)),
					};
					switch ( message ) {
						case WM_KEYDOWN:
						case WM_SYSKEYDOWN: event.key.state 	= -1; break;

						case WM_KEYUP:
						case WM_SYSKEYUP: event.key.state 		=  1; break;
					}
				}
				if ( this->m_syncParse ) {
					this->pushEvent(event.type + "." + event.key.code, uf::userdata::create(event));
					json["type"] 							= event.type + "." + ((event.key.state == -1)?"Pressed":"Released");
					json["invoker"] 						= event.invoker;
					json["key"]["state"] 					= (event.key.state == -1) ? "Down" : "Up";
					json["key"]["async"] 					= event.key.async;
					json["key"]["modifier"]["alt"]			= event.key.modifier.alt;
					json["key"]["modifier"]["control"] 		= event.key.modifier.ctrl;
					json["key"]["modifier"]["shift"]		= event.key.modifier.shift;
					json["key"]["modifier"]["system"]  		= event.key.modifier.sys;
					
					json["key"]["code"] 					= event.key.code;
					json["key"]["raw"] 						= event.key.raw;
					this->pushEvent(event.type + "." + event.key.code, json);
				}
			}
		} break;
		case WM_MOUSEWHEEL: {
			POINT position;
			position.x = static_cast<int16_t>(LOWORD(lParam));
			position.y = static_cast<int16_t>(HIWORD(lParam));
			ScreenToClient(this->m_handle, &position);

			int16_t delta = static_cast<int16_t>(HIWORD(wParam));

			struct Event {
				std::string type = "unknown";
				std::string invoker = "???";

				struct {
					pod::Vector2ui 	position = {};
					int16_t 		delta = 0;
				} mouse;
			};
			Event event; {
				event.type = "window:Mouse.Wheel";
				event.invoker = "os";

				event.mouse.position.x = position.x;
				event.mouse.position.y = position.y;
				event.mouse.delta = delta / 120;
			};
			this->pushEvent(event.type, uf::userdata::create(event)); {
				json["type"] = event.type;
				json["invoker"] = event.invoker;

				json["mouse"]["position"]["x"]		= event.mouse.position.x;
				json["mouse"]["position"]["y"]		= event.mouse.position.y;
				json["mouse"]["delta"] 				= event.mouse.delta;

				this->pushEvent(event.type, json);
			}
		} break;
		case WM_LBUTTONDOWN:
		case WM_LBUTTONUP:
		case WM_RBUTTONDOWN:
		case WM_RBUTTONUP:
		case WM_MBUTTONDOWN:
		case WM_MBUTTONUP:
		case WM_XBUTTONDOWN:
		case WM_XBUTTONUP: if ( true || this->m_syncParse ) {
			static pod::Vector2i lastPosition = {};
			struct Event {
				std::string type = "unknown";
				std::string invoker = "???";

				struct {
					pod::Vector2i 	position = {};
					pod::Vector2i 	delta = lastPosition;
					std::string 	button = "???";
					int 			state = 0;
				} mouse;
			};
			Event event; {
				event.type = "window:Mouse.Click";
				event.invoker = "os";

				event.mouse.position.x 	= static_cast<int16_t>(LOWORD(lParam));
				event.mouse.position.y 	= static_cast<int16_t>(HIWORD(lParam));
				event.mouse.delta 		= uf::vector::subtract( event.mouse.position, lastPosition );
				switch ( message ) {	
					case WM_LBUTTONDOWN:
					case WM_LBUTTONUP: 		event.mouse.button = "Left"; break;
					
					case WM_RBUTTONDOWN:
					case WM_RBUTTONUP: 		event.mouse.button = "Right"; break;
					
					case WM_MBUTTONDOWN:
					case WM_MBUTTONUP: 		event.mouse.button = "Middle"; break;
					
					case WM_XBUTTONDOWN:
					case WM_XBUTTONUP: 		event.mouse.button = "Aux"; break;

					default: 				event.mouse.button = "???"; break;
				}
				switch ( message ) {	
					case WM_LBUTTONUP:
					case WM_RBUTTONUP:
					case WM_MBUTTONUP:
					case WM_XBUTTONUP: 		event.mouse.state = 1; break;

					case WM_LBUTTONDOWN:
					case WM_RBUTTONDOWN:
					case WM_MBUTTONDOWN:
					case WM_XBUTTONDOWN: 	event.mouse.state = -1; break;
					default: 				event.mouse.state =  0; break; 
				}
			};
			if ( true || this->m_syncParse ){
				this->pushEvent(event.type, uf::userdata::create(event));
				json["type"] = event.type;
				json["invoker"] = event.invoker;

				json["mouse"]["position"]["x"]		= event.mouse.position.x;
				json["mouse"]["position"]["y"]		= event.mouse.position.y;
				json["mouse"]["delta"]["x"] 		= event.mouse.delta.x;
				json["mouse"]["delta"]["y"] 		= event.mouse.delta.y;
				json["mouse"]["button"] 			= event.mouse.button;
				switch (event.mouse.state) {
					case 1:
						json["mouse"]["state"] = "Up";
					break;
					case -1:
						json["mouse"]["state"] = "Down";
					break;
					default: 
						json["mouse"]["state"] = "???";
					break;
				}

				this->pushEvent(event.type, json);
			}
			lastPosition = event.mouse.position;
		} break;
		case WM_MOUSELEAVE:
			if (this->m_mouseInside) {
				this->m_mouseInside = false;

				struct Event {
					std::string type = "unknown";
					std::string invoker = "???";

					struct {
						int state = 0;
					} mouse;
				};
				Event event; {
					event.type = "window:Mouse.Moved";
					event.invoker = "os";

					event.mouse.state = -1;
				};
				this->pushEvent(event.type, uf::userdata::create(event)); {
					json["type"] = event.type;
					json["invoker"] = event.invoker;

					switch (event.mouse.state) {
						case 1:
							json["mouse"]["state"] = "Entered";
						break;
						case -1:
							json["mouse"]["state"] = "Left";
						break;
						default: 
							json["mouse"]["state"] = "???";
						break;
					}
					this->pushEvent(event.type, json);
				}
			}
		break;
		case WM_MOUSEMOVE: {
			static pod::Vector2i lastPosition = {};
			struct Event {
				std::string type = "unknown";
				std::string invoker = "???";

				struct {
					int state = 0;
					pod::Vector2i position;
					pod::Vector2i delta;
					pod::Vector2i size;
				} mouse;
			};
			Event event; {
				event.type = "window:Mouse.Moved";
				event.invoker = "os";
			};

			int x = static_cast<int16_t>(LOWORD(lParam));
			int y = static_cast<int16_t>(HIWORD(lParam));
			RECT area;
			GetClientRect(this->m_handle, &area);
			if ((wParam & (MK_LBUTTON | MK_MBUTTON | MK_RBUTTON | MK_XBUTTON1 | MK_XBUTTON2)) == 0) {
				if (GetCapture() == this->m_handle) ReleaseCapture();
			}
			else if (GetCapture() != this->m_handle) {
				SetCapture(this->m_handle);
			}

			if ((x < area.left) || (x > area.right) || (y < area.top) || (y > area.bottom)) {
				if (this->m_mouseInside) {
					this->m_mouseInside = false;
					this->setTracking(false);
					event.mouse.state = -1;
				}
			} else {
				if (!this->m_mouseInside) {
					this->m_mouseInside = true;
					this->setTracking(true);
					event.mouse.state =  1;
				}
			}
			{
				event.mouse.position.x = x;
				event.mouse.position.y = y;
				event.mouse.size = {area.right, area.bottom};
				event.mouse.delta = event.mouse.position - lastPosition;
			}
			this->pushEvent(event.type, uf::userdata::create(event)); {
				json["type"] = event.type;
				json["invoker"] = event.invoker;

				json["mouse"]["position"]["x"] = event.mouse.position.x;
				json["mouse"]["position"]["y"] = event.mouse.position.y;
				json["mouse"]["delta"]["x"] = event.mouse.delta.x;
				json["mouse"]["delta"]["y"] = event.mouse.delta.y;
				json["mouse"]["size"]["x"] = event.mouse.size.x;
				json["mouse"]["size"]["y"] = event.mouse.size.y;
				switch (event.mouse.state) {
					case 1:
						json["mouse"]["state"] = "Entered";
					break;
					case -1:
						json["mouse"]["state"] = "Left";
					break;
					default: 
						json["mouse"]["state"] = "???";
					break;
				}
				this->pushEvent(event.type, json);
			}
			lastPosition.x = event.mouse.position.x;
			lastPosition.y = event.mouse.position.y;
		break;
		}
	}
}

void UF_API_CALL spec::win32::Window::setTracking(bool state) {
	TRACKMOUSEEVENT mouseEvent;
	mouseEvent.cbSize = sizeof(TRACKMOUSEEVENT);
	mouseEvent.hwndTrack = this->m_handle;
	mouseEvent.dwFlags = state ? TME_LEAVE : TME_CANCEL;
	mouseEvent.dwHoverTime = HOVER_DEFAULT;
	TrackMouseEvent(&mouseEvent);
}
void UF_API_CALL spec::win32::Window::setMouseGrabbed(bool state) {
	this->m_mouseGrabbed = state;
	this->grabMouse(state);
}
void UF_API_CALL spec::win32::Window::grabMouse(bool state) {
	if (state) {
		RECT rect;
		GetClientRect(m_handle, &rect);
		MapWindowPoints(m_handle, NULL, reinterpret_cast<LPPOINT>(&rect), 2);
		ClipCursor(&rect);
	} else {
		ClipCursor(NULL);
	}
}
void UF_API_CALL spec::win32::Window::switchToFullscreen() {
	SetWindowLong(this->m_handle, GWL_STYLE, WS_POPUP | WS_CLIPCHILDREN | WS_CLIPSIBLINGS);
	SetWindowLong(this->m_handle, GWL_EXSTYLE, WS_EX_APPWINDOW);
	SetWindowPos(this->m_handle, HWND_TOP, 0, 0, GetSystemMetrics(SM_CXSCREEN), GetSystemMetrics(SM_CYSCREEN), SWP_FRAMECHANGED);
	ShowWindow(this->m_handle, SW_SHOW);
	fullscreenWindow = (void*) this;
}

bool UF_API_CALL spec::win32::Window::isKeyPressed(const std::string& key) {
	if ( (key == "A") && (GetAsyncKeyState('A') & 0x8000) ) return true;
	if ( (key == "B") && (GetAsyncKeyState('B') & 0x8000) ) return true;
	if ( (key == "C") && (GetAsyncKeyState('C') & 0x8000) ) return true;
	if ( (key == "D") && (GetAsyncKeyState('D') & 0x8000) ) return true;
	if ( (key == "E") && (GetAsyncKeyState('E') & 0x8000) ) return true;
	if ( (key == "F") && (GetAsyncKeyState('F') & 0x8000) ) return true;
	if ( (key == "G") && (GetAsyncKeyState('G') & 0x8000) ) return true;
	if ( (key == "H") && (GetAsyncKeyState('H') & 0x8000) ) return true;
	if ( (key == "I") && (GetAsyncKeyState('I') & 0x8000) ) return true;
	if ( (key == "J") && (GetAsyncKeyState('J') & 0x8000) ) return true;
	if ( (key == "K") && (GetAsyncKeyState('K') & 0x8000) ) return true;
	if ( (key == "L") && (GetAsyncKeyState('L') & 0x8000) ) return true;
	if ( (key == "M") && (GetAsyncKeyState('M') & 0x8000) ) return true;
	if ( (key == "N") && (GetAsyncKeyState('N') & 0x8000) ) return true;
	if ( (key == "O") && (GetAsyncKeyState('O') & 0x8000) ) return true;
	if ( (key == "P") && (GetAsyncKeyState('P') & 0x8000) ) return true;
	if ( (key == "Q") && (GetAsyncKeyState('Q') & 0x8000) ) return true;
	if ( (key == "R") && (GetAsyncKeyState('R') & 0x8000) ) return true;
	if ( (key == "S") && (GetAsyncKeyState('S') & 0x8000) ) return true;
	if ( (key == "T") && (GetAsyncKeyState('T') & 0x8000) ) return true;
	if ( (key == "U") && (GetAsyncKeyState('U') & 0x8000) ) return true;
	if ( (key == "V") && (GetAsyncKeyState('V') & 0x8000) ) return true;
	if ( (key == "W") && (GetAsyncKeyState('W') & 0x8000) ) return true;
	if ( (key == "X") && (GetAsyncKeyState('X') & 0x8000) ) return true;
	if ( (key == "Y") && (GetAsyncKeyState('Y') & 0x8000) ) return true;
	if ( (key == "Z") && (GetAsyncKeyState('Z') & 0x8000) ) return true;
	if ( (key == "0") && (GetAsyncKeyState('0') & 0x8000) ) return true;
	if ( (key == "1") && (GetAsyncKeyState('1') & 0x8000) ) return true;
	if ( (key == "2") && (GetAsyncKeyState('2') & 0x8000) ) return true;
	if ( (key == "3") && (GetAsyncKeyState('3') & 0x8000) ) return true;
	if ( (key == "4") && (GetAsyncKeyState('4') & 0x8000) ) return true;
	if ( (key == "5") && (GetAsyncKeyState('5') & 0x8000) ) return true;
	if ( (key == "6") && (GetAsyncKeyState('6') & 0x8000) ) return true;
	if ( (key == "7") && (GetAsyncKeyState('7') & 0x8000) ) return true;
	if ( (key == "8") && (GetAsyncKeyState('8') & 0x8000) ) return true;
	if ( (key == "9") && (GetAsyncKeyState('9') & 0x8000) ) return true;
	if ( (key == "Escape") && (GetAsyncKeyState(VK_ESCAPE) & 0x8000) ) return true;
	if ( (key == "LControl") && (GetAsyncKeyState(VK_LCONTROL) & 0x8000) ) return true;
	if ( (key == "LShift") && (GetAsyncKeyState(VK_LSHIFT) & 0x8000) ) return true;
	if ( (key == "LMenu") && (GetAsyncKeyState(VK_LMENU) & 0x8000) ) return true;
	if ( (key == "LSystem") && (GetAsyncKeyState(VK_LWIN) & 0x8000) ) return true;
	if ( (key == "RControl") && (GetAsyncKeyState(VK_RCONTROL) & 0x8000) ) return true;
	if ( (key == "RShift") && (GetAsyncKeyState(VK_RSHIFT) & 0x8000) ) return true;
	if ( (key == "RMenu") && (GetAsyncKeyState(VK_RMENU) & 0x8000) ) return true;
	if ( (key == "RSystem") && (GetAsyncKeyState(VK_RWIN) & 0x8000) ) return true;
	if ( (key == "Apps") && (GetAsyncKeyState(VK_APPS) & 0x8000) ) return true;
	if ( (key == "OEM4") && (GetAsyncKeyState(VK_OEM_4) & 0x8000) ) return true;
	if ( (key == "OEM6") && (GetAsyncKeyState(VK_OEM_6) & 0x8000) ) return true;
	if ( (key == "OEM1") && (GetAsyncKeyState(VK_OEM_1) & 0x8000) ) return true;
	if ( (key == "OEMComma") && (GetAsyncKeyState(VK_OEM_COMMA) & 0x8000) ) return true;
	if ( (key == "OEMPeriod") && (GetAsyncKeyState(VK_OEM_PERIOD) & 0x8000) ) return true;
	if ( (key == "OEM7") && (GetAsyncKeyState(VK_OEM_7) & 0x8000) ) return true;
	if ( (key == "OEM2") && (GetAsyncKeyState(VK_OEM_2) & 0x8000) ) return true;
	if ( (key == "OEM5") && (GetAsyncKeyState(VK_OEM_5) & 0x8000) ) return true;
	if ( (key == "OEM3") && (GetAsyncKeyState(VK_OEM_3) & 0x8000) ) return true;
	if ( (key == "OEM+") && (GetAsyncKeyState(VK_OEM_PLUS) & 0x8000) ) return true;
	if ( (key == "OEM-") && (GetAsyncKeyState(VK_OEM_MINUS) & 0x8000) ) return true;
	if ( (key == " ") && (GetAsyncKeyState(VK_SPACE) & 0x8000) ) return true;
	if ( (key == "Enter") && (GetAsyncKeyState(VK_RETURN) & 0x8000) ) return true;
	if ( (key == "Back") && (GetAsyncKeyState(VK_BACK) & 0x8000) ) return true;
	if ( (key == "Tab") && (GetAsyncKeyState(VK_TAB) & 0x8000) ) return true;
	if ( (key == "Prior") && (GetAsyncKeyState(VK_PRIOR) & 0x8000) ) return true;
	if ( (key == "Next") && (GetAsyncKeyState(VK_NEXT) & 0x8000) ) return true;
	if ( (key == "End") && (GetAsyncKeyState(VK_END) & 0x8000) ) return true;
	if ( (key == "Home") && (GetAsyncKeyState(VK_HOME) & 0x8000) ) return true;
	if ( (key == "Insert") && (GetAsyncKeyState(VK_INSERT) & 0x8000) ) return true;
	if ( (key == "Delete") && (GetAsyncKeyState(VK_DELETE) & 0x8000) ) return true;
	if ( (key == "+") && (GetAsyncKeyState(VK_ADD) & 0x8000) ) return true;
	if ( (key == "-") && (GetAsyncKeyState(VK_SUBTRACT) & 0x8000) ) return true;
	if ( (key == "*") && (GetAsyncKeyState(VK_MULTIPLY) & 0x8000) ) return true;
	if ( (key == "/") && (GetAsyncKeyState(VK_DIVIDE) & 0x8000) ) return true;
	if ( (key == "Left") && (GetAsyncKeyState(VK_LEFT) & 0x8000) ) return true;
	if ( (key == "Right") && (GetAsyncKeyState(VK_RIGHT) & 0x8000) ) return true;
	if ( (key == "Up") && (GetAsyncKeyState(VK_UP) & 0x8000) ) return true;
	if ( (key == "Down") && (GetAsyncKeyState(VK_DOWN) & 0x8000) ) return true;
	if ( (key == "Num0") && (GetAsyncKeyState(VK_NUMPAD0) & 0x8000) ) return true;
	if ( (key == "Num1") && (GetAsyncKeyState(VK_NUMPAD1) & 0x8000) ) return true;
	if ( (key == "Num2") && (GetAsyncKeyState(VK_NUMPAD2) & 0x8000) ) return true;
	if ( (key == "Num3") && (GetAsyncKeyState(VK_NUMPAD3) & 0x8000) ) return true;
	if ( (key == "Num4") && (GetAsyncKeyState(VK_NUMPAD4) & 0x8000) ) return true;
	if ( (key == "Num5") && (GetAsyncKeyState(VK_NUMPAD5) & 0x8000) ) return true;
	if ( (key == "Num6") && (GetAsyncKeyState(VK_NUMPAD6) & 0x8000) ) return true;
	if ( (key == "Num7") && (GetAsyncKeyState(VK_NUMPAD7) & 0x8000) ) return true;
	if ( (key == "Num8") && (GetAsyncKeyState(VK_NUMPAD8) & 0x8000) ) return true;
	if ( (key == "Num9") && (GetAsyncKeyState(VK_NUMPAD9) & 0x8000) ) return true;
	if ( (key == "F1") && (GetAsyncKeyState(VK_F1) & 0x8000) ) return true;
	if ( (key == "F2") && (GetAsyncKeyState(VK_F2) & 0x8000) ) return true;
	if ( (key == "F3") && (GetAsyncKeyState(VK_F3) & 0x8000) ) return true;
	if ( (key == "F4") && (GetAsyncKeyState(VK_F4) & 0x8000) ) return true;
	if ( (key == "F5") && (GetAsyncKeyState(VK_F5) & 0x8000) ) return true;
	if ( (key == "F6") && (GetAsyncKeyState(VK_F6) & 0x8000) ) return true;
	if ( (key == "F7") && (GetAsyncKeyState(VK_F7) & 0x8000) ) return true;
	if ( (key == "F8") && (GetAsyncKeyState(VK_F8) & 0x8000) ) return true;
	if ( (key == "F9") && (GetAsyncKeyState(VK_F9) & 0x8000) ) return true;
	if ( (key == "F10") && (GetAsyncKeyState(VK_F10) & 0x8000) ) return true;
	if ( (key == "F11") && (GetAsyncKeyState(VK_F11) & 0x8000) ) return true;
	if ( (key == "F12") && (GetAsyncKeyState(VK_F12) & 0x8000) ) return true;
	if ( (key == "F13") && (GetAsyncKeyState(VK_F13) & 0x8000) ) return true;
	if ( (key == "F14") && (GetAsyncKeyState(VK_F14) & 0x8000) ) return true;
	if ( (key == "F15") && (GetAsyncKeyState(VK_F15) & 0x8000) ) return true;
	if ( (key == "Pause") && (GetAsyncKeyState(VK_PAUSE) & 0x8000) ) return true;

	if ( (key == "LeftMouse") && (GetAsyncKeyState(VK_LBUTTON) & 0x8000) ) return true;
	if ( (key == "RightMouse") && (GetAsyncKeyState(VK_RBUTTON) & 0x8000) ) return true;
	if ( (key == "MiddleMouse") && (GetAsyncKeyState(VK_MBUTTON) & 0x8000) ) return true;
	if ( (key == "XButton1") && (GetAsyncKeyState(VK_XBUTTON1) & 0x8000) ) return true;
	if ( (key == "XButton2") && (GetAsyncKeyState(VK_XBUTTON2) & 0x8000) ) return true;
	return false;
}
std::string UF_API_CALL spec::win32::Window::getKey(WPARAM key, LPARAM flags) {
	switch (key) {
		// Check the scancode to distinguish between left and right shift
		case VK_SHIFT: {	
			static UINT lShift = MapVirtualKeyW(VK_LSHIFT, MAPVK_VK_TO_VSC);
			UINT scancode = static_cast<UINT>((flags & (0xFF << 16)) >> 16);
			return scancode == lShift ? "LShift" : "RShift";
		}
		// Check the "extended" flag to distinguish between left and right alt
		case VK_MENU : return (HIWORD(flags) & KF_EXTENDED) ? "RAlt" : "LAlt";
		// Check the "extended" flag to distinguish between left and right control
		case VK_CONTROL : return (HIWORD(flags) & KF_EXTENDED) ? "RControl" : "LControl";

		// Other keys are reported properly
		case VK_LWIN:			return "LSystem";
		case VK_RWIN:			return "RSystem";
		case VK_APPS:			return "Menu";
		case VK_OEM_1:			return "SemiColon";
		case VK_OEM_2:			return "Slash";
		case VK_OEM_PLUS:		return "Equal";
		case VK_OEM_MINUS:  	return "Dash";
		case VK_OEM_4:			return "LBracket";
		case VK_OEM_6:			return "RBracket";
		case VK_OEM_COMMA:  	return "Comma";
		case VK_OEM_PERIOD:		return "Period";
		case VK_OEM_7:			return "Quote";
		case VK_OEM_5:			return "BackSlash";
		case VK_OEM_3:			return "Tilde";
		
		case VK_ESCAPE:			return "Escape";
		case VK_SPACE:			return "Space";
		case VK_RETURN:			return "Enter";
		case VK_BACK:			return "BackSpace";
		case VK_TAB:			return "Tab";
		case VK_PRIOR:			return "PageUp";
		case VK_NEXT:			return "PageDown";
		case VK_END:			return "End";
		case VK_HOME:			return "Home";
		case VK_INSERT:			return "Insert";
		case VK_DELETE:			return "Delete";
		case VK_ADD:			return "Add";
		case VK_SUBTRACT:		return "Subtract";
		case VK_MULTIPLY:		return "Multiply";
		case VK_DIVIDE:			return "Divide";
		case VK_PAUSE:			return "Pause";
		
		case VK_F1:				return "F1";
		case VK_F2:				return "F2";
		case VK_F3:				return "F3";
		case VK_F4:				return "F4";
		case VK_F5:				return "F5";
		case VK_F6:				return "F6";
		case VK_F7:				return "F7";
		case VK_F8:				return "F8";
		case VK_F9:				return "F9";
		case VK_F10:			return "F10";
		case VK_F11:			return "F11";
		case VK_F12:			return "F12";
		case VK_F13:			return "F13";
		case VK_F14:			return "F14";
		case VK_F15:			return "F15";
		
		case VK_LEFT:			return "Left";
		case VK_RIGHT:			return "Right";
		case VK_UP:				return "Up";
		case VK_DOWN:			return "Down";

		case VK_NUMPAD0:		return "Numpad0";
		case VK_NUMPAD1:		return "Numpad1";
		case VK_NUMPAD2:		return "Numpad2";
		case VK_NUMPAD3:		return "Numpad3";
		case VK_NUMPAD4:		return "Numpad4";
		case VK_NUMPAD5:		return "Numpad5";
		case VK_NUMPAD6:		return "Numpad6";
		case VK_NUMPAD7:		return "Numpad7";
		case VK_NUMPAD8:		return "Numpad8";
		case VK_NUMPAD9:		return "Numpad9";

		case 'Q':				return "Q";
		case 'W':				return "W";
		case 'E':				return "E";
		case 'R':				return "R";
		case 'T':				return "T";
		case 'Y':				return "Y";
		case 'U':				return "U";
		case 'I':				return "I";
		case 'O':				return "O";
		case 'P':				return "P";
		
		case 'A':				return "A";
		case 'S':				return "S";
		case 'D':				return "D";
		case 'F':				return "F";
		case 'G':				return "G";
		case 'H':				return "H";
		case 'J':				return "J";
		case 'K':				return "K";
		case 'L':				return "L";
		
		case 'Z':				return "Z";
		case 'X':				return "X";
		case 'C':				return "C";
		case 'V':				return "V";
		case 'B':				return "B";
		case 'N':				return "N";
		case 'M':				return "M";
		
		case '1':				return "Num1";
		case '2':				return "Num2";
		case '3':				return "Num3";
		case '4':				return "Num4";
		case '5':				return "Num5";
		case '6':				return "Num6";
		case '7':				return "Num7";
		case '8':				return "Num8";
		case '9':				return "Num9";
		case '0':				return "Num0";
	}
// 	return "Unknown";	
	return std::string( "" + (int) key );
}
#if defined(UF_USE_VULKAN) && UF_USE_VULKAN == 1
std::vector<const char*> UF_API_CALL spec::win32::Window::getExtensions( bool validationEnabled ) {
	std::vector<const char*> instanceExtensions = { VK_KHR_SURFACE_EXTENSION_NAME  };
	instanceExtensions.push_back(VK_KHR_WIN32_SURFACE_EXTENSION_NAME);

	if ( validationEnabled ) instanceExtensions.push_back(VK_EXT_DEBUG_UTILS_EXTENSION_NAME);
	return instanceExtensions;
}
void UF_API_CALL spec::win32::Window::createSurface( VkInstance instance, VkSurfaceKHR& surface ) {
	VkWin32SurfaceCreateInfoKHR surfaceCreateInfo = {};
	surfaceCreateInfo.sType = VK_STRUCTURE_TYPE_WIN32_SURFACE_CREATE_INFO_KHR;
	surfaceCreateInfo.hinstance = (HINSTANCE) GetModuleHandleW(NULL);
	surfaceCreateInfo.hwnd = (HWND) this->m_handle; 
	vkCreateWin32SurfaceKHR( instance, &surfaceCreateInfo, nullptr, &surface);
}
#endif
#endif