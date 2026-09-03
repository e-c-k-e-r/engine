#include <uf/spec/window/window.h>

#if UF_ENV_WINDOWS
#include <uf/utils/io/iostream.h>
#include <uf/utils/string/utf.h>
#include <uf/utils/serialize/serializer.h>
#include <uf/utils/window/payloads.h>
#include <uf/utils/io/inputs.h>
#include <uf/utils/string/ext.h>

#define UF_OPENGL_CONTEXT_IN_WINDOW 0
#include <uf/spec/context/context.h>

#define UF_HOOK_USE_USERDATA 1
#define UF_HOOK_USE_JSON 0

#if UF_USE_IMGUI
	#include <uf/ext/imgui/imgui.h>
	#include <imgui/backends/imgui_impl_win32.h>
	extern IMGUI_IMPL_API LRESULT ImGui_ImplWin32_WndProcHandler(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam);
#endif

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
	#if UF_USE_IMGUI
		if (ImGui_ImplWin32_WndProcHandler(handle, message, wParam, lParam))
			return true;
	#endif
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

	uf::stl::vector<WPARAM> GetKeys() {
		uf::stl::vector<WPARAM> keys;
		keys.reserve(8);

		#define IF_KEY_STATE( K, V ) if ( uf::inputs::kbm::states::K ) keys.emplace_back(V);

		IF_KEY_STATE( LShift, VK_LSHIFT);
		IF_KEY_STATE( RShift, VK_RSHIFT);

		IF_KEY_STATE( LAlt, VK_LMENU);
		IF_KEY_STATE( RAlt, VK_RMENU);

		IF_KEY_STATE( LControl, VK_LCONTROL);
		IF_KEY_STATE( RControl, VK_RCONTROL);

		IF_KEY_STATE( LSystem, VK_LWIN);
		IF_KEY_STATE( RSystem, VK_RWIN);

		IF_KEY_STATE( Menu, VK_APPS);
		IF_KEY_STATE( SemiColon, VK_OEM_1);
		IF_KEY_STATE( Slash, VK_OEM_2);
		IF_KEY_STATE( Equal, VK_OEM_PLUS);
		IF_KEY_STATE( Dash, VK_OEM_MINUS);
		IF_KEY_STATE( LBracket, VK_OEM_4);
		IF_KEY_STATE( RBracket, VK_OEM_6);
		IF_KEY_STATE( Comma, VK_OEM_COMMA);
		IF_KEY_STATE( Period, VK_OEM_PERIOD);
		IF_KEY_STATE( Quote, VK_OEM_7);
		IF_KEY_STATE( BackSlash, VK_OEM_5);
		IF_KEY_STATE( Tilde, VK_OEM_3);
		
		IF_KEY_STATE( Escape, VK_ESCAPE);
		IF_KEY_STATE( Space, VK_SPACE);
		IF_KEY_STATE( Enter, VK_RETURN);
		IF_KEY_STATE( BackSpace, VK_BACK);
		IF_KEY_STATE( Tab, VK_TAB);
		IF_KEY_STATE( PageUp, VK_PRIOR);
		IF_KEY_STATE( PageDown, VK_NEXT);
		IF_KEY_STATE( End, VK_END);
		IF_KEY_STATE( Home, VK_HOME);
		IF_KEY_STATE( Insert, VK_INSERT);
		IF_KEY_STATE( Delete, VK_DELETE);
		IF_KEY_STATE( Add, VK_ADD);
		IF_KEY_STATE( Subtract, VK_SUBTRACT);
		IF_KEY_STATE( Multiply, VK_MULTIPLY);
		IF_KEY_STATE( Divide, VK_DIVIDE);
		IF_KEY_STATE( Pause, VK_PAUSE);
		
		IF_KEY_STATE( F1, VK_F1);
		IF_KEY_STATE( F2, VK_F2);
		IF_KEY_STATE( F3, VK_F3);
		IF_KEY_STATE( F4, VK_F4);
		IF_KEY_STATE( F5, VK_F5);
		IF_KEY_STATE( F6, VK_F6);
		IF_KEY_STATE( F7, VK_F7);
		IF_KEY_STATE( F8, VK_F8);
		IF_KEY_STATE( F9, VK_F9);
		IF_KEY_STATE( F10, VK_F10);
		IF_KEY_STATE( F11, VK_F11);
		IF_KEY_STATE( F12, VK_F12);
		IF_KEY_STATE( F13, VK_F13);
		IF_KEY_STATE( F14, VK_F14);
		IF_KEY_STATE( F15, VK_F15);
		
		IF_KEY_STATE( Left, VK_LEFT);
		IF_KEY_STATE( Right, VK_RIGHT);
		IF_KEY_STATE( Up, VK_UP);
		IF_KEY_STATE( Down, VK_DOWN);

		IF_KEY_STATE( Numpad0, VK_NUMPAD0);
		IF_KEY_STATE( Numpad1, VK_NUMPAD1);
		IF_KEY_STATE( Numpad2, VK_NUMPAD2);
		IF_KEY_STATE( Numpad3, VK_NUMPAD3);
		IF_KEY_STATE( Numpad4, VK_NUMPAD4);
		IF_KEY_STATE( Numpad5, VK_NUMPAD5);
		IF_KEY_STATE( Numpad6, VK_NUMPAD6);
		IF_KEY_STATE( Numpad7, VK_NUMPAD7);
		IF_KEY_STATE( Numpad8, VK_NUMPAD8);
		IF_KEY_STATE( Numpad9, VK_NUMPAD9);

		IF_KEY_STATE( Q, 'Q');
		IF_KEY_STATE( W, 'W');
		IF_KEY_STATE( E, 'E');
		IF_KEY_STATE( R, 'R');
		IF_KEY_STATE( T, 'T');
		IF_KEY_STATE( Y, 'Y');
		IF_KEY_STATE( U, 'U');
		IF_KEY_STATE( I, 'I');
		IF_KEY_STATE( O, 'O');
		IF_KEY_STATE( P, 'P');
		
		IF_KEY_STATE( A, 'A');
		IF_KEY_STATE( S, 'S');
		IF_KEY_STATE( D, 'D');
		IF_KEY_STATE( F, 'F');
		IF_KEY_STATE( G, 'G');
		IF_KEY_STATE( H, 'H');
		IF_KEY_STATE( J, 'J');
		IF_KEY_STATE( K, 'K');
		IF_KEY_STATE( L, 'L');
		
		IF_KEY_STATE( Z, 'Z');
		IF_KEY_STATE( X, 'X');
		IF_KEY_STATE( C, 'C');
		IF_KEY_STATE( V, 'V');
		IF_KEY_STATE( B, 'B');
		IF_KEY_STATE( N, 'N');
		IF_KEY_STATE( M, 'M');
		
		IF_KEY_STATE( Num1, '1');
		IF_KEY_STATE( Num2, '2');
		IF_KEY_STATE( Num3, '3');
		IF_KEY_STATE( Num4, '4');
		IF_KEY_STATE( Num5, '5');
		IF_KEY_STATE( Num6, '6');
		IF_KEY_STATE( Num7, '7');
		IF_KEY_STATE( Num8, '8');
		IF_KEY_STATE( Num9, '9');
		IF_KEY_STATE( Num0, '0');
		
		IF_KEY_STATE( Mouse1, VK_LBUTTON);
		IF_KEY_STATE( Mouse2, VK_RBUTTON);
		IF_KEY_STATE( Mouse3, VK_MBUTTON);

		return keys;
	}


	uf::stl::string _GetKeyName( WPARAM key, LPARAM flags = 0 ) {
		#define CASE_KEY_RETURN( K, V ) case K: return V;
		switch ( key ) {
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
			CASE_KEY_RETURN(VK_LWIN, "LSystem");
			CASE_KEY_RETURN(VK_RWIN, "RSystem");
			CASE_KEY_RETURN(VK_APPS, "Menu");
			CASE_KEY_RETURN(VK_OEM_1, "SemiColon");
			CASE_KEY_RETURN(VK_OEM_2, "Slash");
			CASE_KEY_RETURN(VK_OEM_PLUS, "Equal");
			CASE_KEY_RETURN(VK_OEM_MINUS, "Dash");
			CASE_KEY_RETURN(VK_OEM_4, "LBracket");
			CASE_KEY_RETURN(VK_OEM_6, "RBracket");
			CASE_KEY_RETURN(VK_OEM_COMMA, "Comma");
			CASE_KEY_RETURN(VK_OEM_PERIOD, "Period");
			CASE_KEY_RETURN(VK_OEM_7, "Quote");
			CASE_KEY_RETURN(VK_OEM_5, "BackSlash");
			CASE_KEY_RETURN(VK_OEM_3, "Tilde");
			
			CASE_KEY_RETURN(VK_ESCAPE, "Escape");
			CASE_KEY_RETURN(VK_SPACE, "Space");
			CASE_KEY_RETURN(VK_RETURN, "Enter");
			CASE_KEY_RETURN(VK_BACK, "BackSpace");
			CASE_KEY_RETURN(VK_TAB, "Tab");
			CASE_KEY_RETURN(VK_PRIOR, "PageUp");
			CASE_KEY_RETURN(VK_NEXT, "PageDown");
			CASE_KEY_RETURN(VK_END, "End");
			CASE_KEY_RETURN(VK_HOME, "Home");
			CASE_KEY_RETURN(VK_INSERT, "Insert");
			CASE_KEY_RETURN(VK_DELETE, "Delete");
			CASE_KEY_RETURN(VK_ADD, "Add");
			CASE_KEY_RETURN(VK_SUBTRACT, "Subtract");
			CASE_KEY_RETURN(VK_MULTIPLY, "Multiply");
			CASE_KEY_RETURN(VK_DIVIDE, "Divide");
			CASE_KEY_RETURN(VK_PAUSE, "Pause");
			
			CASE_KEY_RETURN(VK_F1, "F1");
			CASE_KEY_RETURN(VK_F2, "F2");
			CASE_KEY_RETURN(VK_F3, "F3");
			CASE_KEY_RETURN(VK_F4, "F4");
			CASE_KEY_RETURN(VK_F5, "F5");
			CASE_KEY_RETURN(VK_F6, "F6");
			CASE_KEY_RETURN(VK_F7, "F7");
			CASE_KEY_RETURN(VK_F8, "F8");
			CASE_KEY_RETURN(VK_F9, "F9");
			CASE_KEY_RETURN(VK_F10, "F10");
			CASE_KEY_RETURN(VK_F11, "F11");
			CASE_KEY_RETURN(VK_F12, "F12");
			CASE_KEY_RETURN(VK_F13, "F13");
			CASE_KEY_RETURN(VK_F14, "F14");
			CASE_KEY_RETURN(VK_F15, "F15");
			
			CASE_KEY_RETURN(VK_LEFT, "Left");
			CASE_KEY_RETURN(VK_RIGHT, "Right");
			CASE_KEY_RETURN(VK_UP, "Up");
			CASE_KEY_RETURN(VK_DOWN, "Down");

			CASE_KEY_RETURN(VK_NUMPAD0, "Numpad0");
			CASE_KEY_RETURN(VK_NUMPAD1, "Numpad1");
			CASE_KEY_RETURN(VK_NUMPAD2, "Numpad2");
			CASE_KEY_RETURN(VK_NUMPAD3, "Numpad3");
			CASE_KEY_RETURN(VK_NUMPAD4, "Numpad4");
			CASE_KEY_RETURN(VK_NUMPAD5, "Numpad5");
			CASE_KEY_RETURN(VK_NUMPAD6, "Numpad6");
			CASE_KEY_RETURN(VK_NUMPAD7, "Numpad7");
			CASE_KEY_RETURN(VK_NUMPAD8, "Numpad8");
			CASE_KEY_RETURN(VK_NUMPAD9, "Numpad9");

			CASE_KEY_RETURN('Q', "Q");
			CASE_KEY_RETURN('W', "W");
			CASE_KEY_RETURN('E', "E");
			CASE_KEY_RETURN('R', "R");
			CASE_KEY_RETURN('T', "T");
			CASE_KEY_RETURN('Y', "Y");
			CASE_KEY_RETURN('U', "U");
			CASE_KEY_RETURN('I', "I");
			CASE_KEY_RETURN('O', "O");
			CASE_KEY_RETURN('P', "P");
			
			CASE_KEY_RETURN('A', "A");
			CASE_KEY_RETURN('S', "S");
			CASE_KEY_RETURN('D', "D");
			CASE_KEY_RETURN('F', "F");
			CASE_KEY_RETURN('G', "G");
			CASE_KEY_RETURN('H', "H");
			CASE_KEY_RETURN('J', "J");
			CASE_KEY_RETURN('K', "K");
			CASE_KEY_RETURN('L', "L");
			
			CASE_KEY_RETURN('Z', "Z");
			CASE_KEY_RETURN('X', "X");
			CASE_KEY_RETURN('C', "C");
			CASE_KEY_RETURN('V', "V");
			CASE_KEY_RETURN('B', "B");
			CASE_KEY_RETURN('N', "N");
			CASE_KEY_RETURN('M', "M");
			
			CASE_KEY_RETURN('1', "Num1");
			CASE_KEY_RETURN('2', "Num2");
			CASE_KEY_RETURN('3', "Num3");
			CASE_KEY_RETURN('4', "Num4");
			CASE_KEY_RETURN('5', "Num5");
			CASE_KEY_RETURN('6', "Num6");
			CASE_KEY_RETURN('7', "Num7");
			CASE_KEY_RETURN('8', "Num8");
			CASE_KEY_RETURN('9', "Num9");
			CASE_KEY_RETURN('0', "Num0");
		}
		return FMT_FORMAT("{}", key);
	}
	uf::stl::string GetKeyName( WPARAM key, LPARAM flags = 0 ) {
		return uf::string::uppercase( _GetKeyName( key, flags ) );
	}

	WPARAM GetKeyCode( const uf::stl::string& _name ) {
		uf::stl::string name = uf::string::uppercase(_name);
		#define IF_KEY_RETURN( K, V ) if ( name == K ) return V

		IF_KEY_RETURN( "A", 'A');
		else IF_KEY_RETURN( "B", 'B');
		else IF_KEY_RETURN( "C", 'C');
		else IF_KEY_RETURN( "D", 'D');
		else IF_KEY_RETURN( "E", 'E');
		else IF_KEY_RETURN( "F", 'F');
		else IF_KEY_RETURN( "G", 'G');
		else IF_KEY_RETURN( "H", 'H');
		else IF_KEY_RETURN( "I", 'I');
		else IF_KEY_RETURN( "J", 'J');
		else IF_KEY_RETURN( "K", 'K');
		else IF_KEY_RETURN( "L", 'L');
		else IF_KEY_RETURN( "M", 'M');
		else IF_KEY_RETURN( "N", 'N');
		else IF_KEY_RETURN( "O", 'O');
		else IF_KEY_RETURN( "P", 'P');
		else IF_KEY_RETURN( "Q", 'Q');
		else IF_KEY_RETURN( "R", 'R');
		else IF_KEY_RETURN( "S", 'S');
		else IF_KEY_RETURN( "T", 'T');
		else IF_KEY_RETURN( "U", 'U');
		else IF_KEY_RETURN( "V", 'V');
		else IF_KEY_RETURN( "W", 'W');
		else IF_KEY_RETURN( "X", 'X');
		else IF_KEY_RETURN( "Y", 'Y');
		else IF_KEY_RETURN( "Z", 'Z');
		else IF_KEY_RETURN( "0", '0');
		else IF_KEY_RETURN( "1", '1');
		else IF_KEY_RETURN( "2", '2');
		else IF_KEY_RETURN( "3", '3');
		else IF_KEY_RETURN( "4", '4');
		else IF_KEY_RETURN( "5", '5');
		else IF_KEY_RETURN( "6", '6');
		else IF_KEY_RETURN( "7", '7');
		else IF_KEY_RETURN( "8", '8');
		else IF_KEY_RETURN( "9", '9');
		else IF_KEY_RETURN( "ESCAPE", VK_ESCAPE);
		else IF_KEY_RETURN( "LCONTROL", VK_LCONTROL);
		else IF_KEY_RETURN( "LSHIFT", VK_LSHIFT);
		else IF_KEY_RETURN( "LALT", VK_LMENU);
		else IF_KEY_RETURN( "LSYSTEM", VK_LWIN);
		else IF_KEY_RETURN( "RCONTROL", VK_RCONTROL);
		else IF_KEY_RETURN( "RSHIFT", VK_RSHIFT);
		else IF_KEY_RETURN( "RALT", VK_RMENU);
		else IF_KEY_RETURN( "RSYSTEM", VK_RWIN);
		else IF_KEY_RETURN( "APPS", VK_APPS);
		else IF_KEY_RETURN( "OEM4", VK_OEM_4);
		else IF_KEY_RETURN( "OEM6", VK_OEM_6);
		else IF_KEY_RETURN( "OEM1", VK_OEM_1);
		else IF_KEY_RETURN( "OEMCOMMA", VK_OEM_COMMA);
		else IF_KEY_RETURN( "OEMPERIOD", VK_OEM_PERIOD);
		else IF_KEY_RETURN( "OEM7", VK_OEM_7);
		else IF_KEY_RETURN( "OEM2", VK_OEM_2);
		else IF_KEY_RETURN( "OEM5", VK_OEM_5);
		else IF_KEY_RETURN( "OEM3", VK_OEM_3);
		else IF_KEY_RETURN( "OEM+", VK_OEM_PLUS);
		else IF_KEY_RETURN( "OEM-", VK_OEM_MINUS);
		else IF_KEY_RETURN( " ", VK_SPACE);
		else IF_KEY_RETURN( "SPACE", VK_SPACE);
		else IF_KEY_RETURN( "ENTER", VK_RETURN);
		else IF_KEY_RETURN( "BACK", VK_BACK);
		else IF_KEY_RETURN( "TAB", VK_TAB);
		else IF_KEY_RETURN( "PRIOR", VK_PRIOR);
		else IF_KEY_RETURN( "NEXT", VK_NEXT);
		else IF_KEY_RETURN( "END", VK_END);
		else IF_KEY_RETURN( "HOME", VK_HOME);
		else IF_KEY_RETURN( "INSERT", VK_INSERT);
		else IF_KEY_RETURN( "DELETE", VK_DELETE);
		else IF_KEY_RETURN( "+", VK_ADD);
		else IF_KEY_RETURN( "-", VK_SUBTRACT);
		else IF_KEY_RETURN( "*", VK_MULTIPLY);
		else IF_KEY_RETURN( "/", VK_DIVIDE);
		else IF_KEY_RETURN( "LEFT", VK_LEFT);
		else IF_KEY_RETURN( "RIGHT", VK_RIGHT);
		else IF_KEY_RETURN( "UP", VK_UP);
		else IF_KEY_RETURN( "DOWN", VK_DOWN);
		else IF_KEY_RETURN( "NUM0", VK_NUMPAD0);
		else IF_KEY_RETURN( "NUM1", VK_NUMPAD1);
		else IF_KEY_RETURN( "NUM2", VK_NUMPAD2);
		else IF_KEY_RETURN( "NUM3", VK_NUMPAD3);
		else IF_KEY_RETURN( "NUM4", VK_NUMPAD4);
		else IF_KEY_RETURN( "NUM5", VK_NUMPAD5);
		else IF_KEY_RETURN( "NUM6", VK_NUMPAD6);
		else IF_KEY_RETURN( "NUM7", VK_NUMPAD7);
		else IF_KEY_RETURN( "NUM8", VK_NUMPAD8);
		else IF_KEY_RETURN( "NUM9", VK_NUMPAD9);
		else IF_KEY_RETURN( "F1", VK_F1);
		else IF_KEY_RETURN( "F2", VK_F2);
		else IF_KEY_RETURN( "F3", VK_F3);
		else IF_KEY_RETURN( "F4", VK_F4);
		else IF_KEY_RETURN( "F5", VK_F5);
		else IF_KEY_RETURN( "F6", VK_F6);
		else IF_KEY_RETURN( "F7", VK_F7);
		else IF_KEY_RETURN( "F8", VK_F8);
		else IF_KEY_RETURN( "F9", VK_F9);
		else IF_KEY_RETURN( "F10", VK_F10);
		else IF_KEY_RETURN( "F11", VK_F11);
		else IF_KEY_RETURN( "F12", VK_F12);
		else IF_KEY_RETURN( "F13", VK_F13);
		else IF_KEY_RETURN( "F14", VK_F14);
		else IF_KEY_RETURN( "F15", VK_F15);
		else IF_KEY_RETURN( "PAUSE", VK_PAUSE);
		
		else IF_KEY_RETURN( "MOUSE1", VK_LBUTTON);
		else IF_KEY_RETURN( "MOUSE2", VK_RBUTTON);
		else IF_KEY_RETURN( "MOUSE3", VK_MBUTTON);
		else IF_KEY_RETURN( "XBUTTON1", VK_XBUTTON1);
		else IF_KEY_RETURN( "XBUTTON2", VK_XBUTTON2);
		return 0;
	}

	float lastMouseWheel;
}

spec::win32::Window::Window() : 
	m_handle 			(NULL),
	m_context 			(NULL),
	m_callback			(0),
	m_cursor			(NULL),
	m_icon				(NULL),
	m_lastSize			({}),
	m_keyRepeatEnabled	(true),
	m_resizing			(false),
	m_mouseInside		(false),
	m_mouseGrabbed		(false),
	m_syncParse			(true),
	m_asyncParse		(false)
{
}
spec::win32::Window::Window( spec::win32::Window::handle_t handle ) :
	m_handle 			(handle),
	m_callback			(0),
	m_cursor			(NULL),
	m_icon				(NULL),
	m_lastSize			({}),
	m_keyRepeatEnabled	(true),
	m_resizing			(false),
	m_mouseInside		(false),
	m_mouseGrabbed		(false),
	m_syncParse			(true),
	m_asyncParse		(false)
{
	if ( handle ) {
		SetWindowLongPtrW(this->m_handle, GWLP_USERDATA, reinterpret_cast<LONG_PTR>(this));
		m_callback = SetWindowLongPtrW(this->m_handle, GWLP_WNDPROC, reinterpret_cast<LONG_PTR>(&::globalOnEvent));
	}
}
spec::win32::Window::Window( const spec::win32::Window::vector_t& size, const spec::win32::Window::title_t& title ) : 
	m_handle 			(NULL),
	m_context 			(NULL),
	m_callback			(0),
	m_cursor			(NULL),
	m_icon				(NULL),
	m_keyRepeatEnabled	(true),
	m_resizing			(false),
	m_mouseInside		(false),
	m_mouseGrabbed		(false),
	m_syncParse			(true),
	m_asyncParse		(false)
{
	this->create(size, title);
}
void spec::win32::Window::create( const spec::win32::Window::vector_t& _size, const spec::win32::Window::title_t& title ) {
	setProcessDpiAware();
	if ( windowCount == 0 ) this->registerWindowClass();

	auto size = _size;
	if ( size.x <= 0 && size.y <= 0 ) {
		size.x = GetSystemMetrics(SM_CXSCREEN);
		size.y = GetSystemMetrics(SM_CYSCREEN);
	}
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
		std::wstring(title.begin(), title.end()).c_str(),
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

#if UF_USE_OPENGL && UF_OPENGL_CONTEXT_IN_WINDOW
	this->m_context = (void*) spec::uni::Context::create( settings, *this );
#endif
}

spec::win32::Window::~Window() {
	if ( this->m_icon ) DestroyIcon(this->m_icon);
	if ( !this->m_callback ) {
		if ( this->m_handle ) DestroyWindow(this->m_handle);
		if ( --windowCount == 0 ) UnregisterClassW( className.c_str(), GetModuleHandleW(NULL) );
	} else {
		SetWindowLongPtrW( this->m_handle, GWLP_WNDPROC, this->m_callback );
	}

#if UF_OPENGL_CONTEXT_IN_WINDOW
	if ( this->m_context ) {
		spec::Context* context = (spec::Context*) this->m_context;
		context->terminate();
		delete context;
		this->m_context = NULL;
	}
#endif
}
void spec::win32::Window::terminate() {
	if ( this == (spec::win32::Window*) fullscreenWindow ) {
		ChangeDisplaySettingsW(NULL, 0);
		fullscreenWindow = NULL;
	}
	this->setCursorVisible(true);
	this->setTracking(false);
	ReleaseCapture();
}

void* spec::win32::Window::getHandle() const {
	return (void*)this->m_handle;
}
spec::win32::Window::vector_t spec::win32::Window::getPosition() const {
	RECT rectangle;
	GetWindowRect( this->m_handle, &rectangle );
	spec::win32::Window::vector_t vec;
	vec.x = rectangle.left;
	vec.y = rectangle.top;
	return vec;
}
spec::win32::Window::vector_t spec::win32::Window::getSize() const {
	RECT rectangle;
	GetClientRect( this->m_handle, &rectangle );
	spec::win32::Window::vector_t vec;
	vec.x = rectangle.right - rectangle.left;
	vec.y = rectangle.bottom - rectangle.top;
	return vec;
}
size_t spec::win32::Window::getRefreshRate() const {
	HDC screenDC = GetDC(NULL);
	int refreshRate = GetDeviceCaps( screenDC, VREFRESH );
	ReleaseDC(NULL, screenDC);
	return refreshRate;
}

void spec::win32::Window::centerWindow() {
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
void spec::win32::Window::setPosition( const spec::win32::Window::vector_t& position ) {
	if ( fullscreenWindow == (void*) this ) return;
	SetWindowPos(this->m_handle, NULL, position.x, position.y, 0, 0, SWP_NOSIZE | SWP_NOZORDER);
}
void spec::win32::Window::setMousePosition( const spec::win32::Window::vector_t& position ) {
	POINT pt = { position.x, position.y };
	ClientToScreen(this->m_handle, &pt);
	ReleaseCapture();
	SetCursorPos(pt.x,pt.y);
	SetCapture(this->m_handle);
}
spec::win32::Window::vector_t spec::win32::Window::getMousePosition( ) {
	POINT pt;
	GetCursorPos( &pt );
	ScreenToClient( this->m_handle, &pt );
	return { pt.x, pt.y };
}
void spec::win32::Window::setSize( const spec::win32::Window::vector_t& size ) {
	if ( fullscreenWindow == (void*) this ) return;
	RECT rectangle = { 0, 0, size.x, size.y };
	if ( rectangle.right <= 0 && rectangle.bottom <= 0 ) {
		rectangle.right = GetSystemMetrics(SM_CXSCREEN);
		rectangle.bottom = GetSystemMetrics(SM_CYSCREEN);
	}
	AdjustWindowRect( &rectangle, GetWindowLong(this->m_handle, GWL_STYLE), false );
	SetWindowPos(this->m_handle, NULL, 0, 0, rectangle.right - rectangle.left, rectangle.bottom - rectangle.top, SWP_NOMOVE | SWP_NOZORDER);
}

void spec::win32::Window::setTitle( const spec::win32::Window::title_t& title ) {
	SetWindowTextW(this->m_handle, std::wstring(title.begin(), title.end()).c_str());
//	SetWindowTextW(this->m_handle, (wchar_t*) std::wstring(title.begin(), title.end()).c_str());
}
void spec::win32::Window::setIcon( const spec::win32::Window::vector_t& size, uint8_t* pixels ) {
	// First destroy the previous one

	uf::stl::vector<uint8_t> iconPixels(size.x * size.y * 4);
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
	uf::stl::vector<uint8_t> icon( uf::vector::product(size) * 4 );
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
void spec::win32::Window::setVisible( bool visibility ) {
	ShowWindow(this->m_handle, visibility ? SW_SHOW : SW_HIDE);
}
void spec::win32::Window::setCursorVisible( bool visibility ) {
	this->m_cursor = ( visibility ? LoadCursor(NULL, IDC_ARROW) : NULL );
	SetCursor(this->m_cursor);
}
void spec::win32::Window::setKeyRepeatEnabled( bool state ) {
	this->m_keyRepeatEnabled = state;
}

void spec::win32::Window::requestFocus() {
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
bool spec::win32::Window::hasFocus() const {
#if UF_USE_IMGUI
	if ( ext::imgui::focused ) return false;
#endif
	return this->m_handle == GetForegroundWindow();
}

void spec::win32::Window::bufferInputs() {
	uf::Window::focused = this->hasFocus();

	#define GET_KEYSTATE(X) uf::Window::focused ? GetAsyncKeyState(X) & 0x8000 : false;
	#define STORE_KEYSTATE(K, V) uf::inputs::kbm::states::K = GET_KEYSTATE(V);

	STORE_KEYSTATE(LShift, VK_LSHIFT);
	STORE_KEYSTATE(RShift, VK_RSHIFT);

	STORE_KEYSTATE(LAlt, VK_LMENU);
	STORE_KEYSTATE(RAlt, VK_RMENU);

	STORE_KEYSTATE(LControl, VK_LCONTROL);
	STORE_KEYSTATE(RControl, VK_RCONTROL);

	STORE_KEYSTATE(LSystem, VK_LWIN);
	STORE_KEYSTATE(RSystem, VK_RWIN);

	STORE_KEYSTATE(Menu, VK_APPS);
	STORE_KEYSTATE(SemiColon, VK_OEM_1);
	STORE_KEYSTATE(Slash, VK_OEM_2);
	STORE_KEYSTATE(Equal, VK_OEM_PLUS);
	STORE_KEYSTATE(Dash, VK_OEM_MINUS);
	STORE_KEYSTATE(LBracket, VK_OEM_4);
	STORE_KEYSTATE(RBracket, VK_OEM_6);
	STORE_KEYSTATE(Comma, VK_OEM_COMMA);
	STORE_KEYSTATE(Period, VK_OEM_PERIOD);
	STORE_KEYSTATE(Quote, VK_OEM_7);
	STORE_KEYSTATE(BackSlash, VK_OEM_5);
	STORE_KEYSTATE(Tilde, VK_OEM_3);
	
	STORE_KEYSTATE(Escape, VK_ESCAPE);
	STORE_KEYSTATE(Space, VK_SPACE);
	STORE_KEYSTATE(Enter, VK_RETURN);
	STORE_KEYSTATE(BackSpace, VK_BACK);
	STORE_KEYSTATE(Tab, VK_TAB);
	STORE_KEYSTATE(PageUp, VK_PRIOR);
	STORE_KEYSTATE(PageDown, VK_NEXT);
	STORE_KEYSTATE(End, VK_END);
	STORE_KEYSTATE(Home, VK_HOME);
	STORE_KEYSTATE(Insert, VK_INSERT);
	STORE_KEYSTATE(Delete, VK_DELETE);
	STORE_KEYSTATE(Add, VK_ADD);
	STORE_KEYSTATE(Subtract, VK_SUBTRACT);
	STORE_KEYSTATE(Multiply, VK_MULTIPLY);
	STORE_KEYSTATE(Divide, VK_DIVIDE);
	STORE_KEYSTATE(Pause, VK_PAUSE);
	
	STORE_KEYSTATE(F1, VK_F1);
	STORE_KEYSTATE(F2, VK_F2);
	STORE_KEYSTATE(F3, VK_F3);
	STORE_KEYSTATE(F4, VK_F4);
	STORE_KEYSTATE(F5, VK_F5);
	STORE_KEYSTATE(F6, VK_F6);
	STORE_KEYSTATE(F7, VK_F7);
	STORE_KEYSTATE(F8, VK_F8);
	STORE_KEYSTATE(F9, VK_F9);
	STORE_KEYSTATE(F10, VK_F10);
	STORE_KEYSTATE(F11, VK_F11);
	STORE_KEYSTATE(F12, VK_F12);
	STORE_KEYSTATE(F13, VK_F13);
	STORE_KEYSTATE(F14, VK_F14);
	STORE_KEYSTATE(F15, VK_F15);
	
	STORE_KEYSTATE(Left, VK_LEFT);
	STORE_KEYSTATE(Right, VK_RIGHT);
	STORE_KEYSTATE(Up, VK_UP);
	STORE_KEYSTATE(Down, VK_DOWN);

	STORE_KEYSTATE(Numpad0, VK_NUMPAD0);
	STORE_KEYSTATE(Numpad1, VK_NUMPAD1);
	STORE_KEYSTATE(Numpad2, VK_NUMPAD2);
	STORE_KEYSTATE(Numpad3, VK_NUMPAD3);
	STORE_KEYSTATE(Numpad4, VK_NUMPAD4);
	STORE_KEYSTATE(Numpad5, VK_NUMPAD5);
	STORE_KEYSTATE(Numpad6, VK_NUMPAD6);
	STORE_KEYSTATE(Numpad7, VK_NUMPAD7);
	STORE_KEYSTATE(Numpad8, VK_NUMPAD8);
	STORE_KEYSTATE(Numpad9, VK_NUMPAD9);

	STORE_KEYSTATE(Q, 'Q');
	STORE_KEYSTATE(W, 'W');
	STORE_KEYSTATE(E, 'E');
	STORE_KEYSTATE(R, 'R');
	STORE_KEYSTATE(T, 'T');
	STORE_KEYSTATE(Y, 'Y');
	STORE_KEYSTATE(U, 'U');
	STORE_KEYSTATE(I, 'I');
	STORE_KEYSTATE(O, 'O');
	STORE_KEYSTATE(P, 'P');
	
	STORE_KEYSTATE(A, 'A');
	STORE_KEYSTATE(S, 'S');
	STORE_KEYSTATE(D, 'D');
	STORE_KEYSTATE(F, 'F');
	STORE_KEYSTATE(G, 'G');
	STORE_KEYSTATE(H, 'H');
	STORE_KEYSTATE(J, 'J');
	STORE_KEYSTATE(K, 'K');
	STORE_KEYSTATE(L, 'L');
	
	STORE_KEYSTATE(Z, 'Z');
	STORE_KEYSTATE(X, 'X');
	STORE_KEYSTATE(C, 'C');
	STORE_KEYSTATE(V, 'V');
	STORE_KEYSTATE(B, 'B');
	STORE_KEYSTATE(N, 'N');
	STORE_KEYSTATE(M, 'M');
	
	STORE_KEYSTATE(Num1, '1');
	STORE_KEYSTATE(Num2, '2');
	STORE_KEYSTATE(Num3, '3');
	STORE_KEYSTATE(Num4, '4');
	STORE_KEYSTATE(Num5, '5');
	STORE_KEYSTATE(Num6, '6');
	STORE_KEYSTATE(Num7, '7');
	STORE_KEYSTATE(Num8, '8');
	STORE_KEYSTATE(Num9, '9');
	STORE_KEYSTATE(Num0, '0');
	
	STORE_KEYSTATE(Mouse1, VK_LBUTTON);
	STORE_KEYSTATE(Mouse2, VK_RBUTTON);
	STORE_KEYSTATE(Mouse3, VK_MBUTTON);

	uf::inputs::kbm::states::MouseWheel = uf::Window::focused ? ::lastMouseWheel : 0;
	::lastMouseWheel = 0;
}
void spec::win32::Window::processEvents() {
	if ( !this->m_callback ) {
		MSG message;
		while ( PeekMessageW( &message, NULL, 0, 0, PM_REMOVE ) ) {
			TranslateMessage(&message);
			DispatchMessage(&message);
		}
	}
	/* Key inputs */ if ( this->m_asyncParse ) {
		uf::stl::vector<WPARAM> keys = GetKeys();
		pod::payloads::windowKey event{
			{
				"window:Key",
				"os",
			},
			{
				"",
				0,

				-1,
				true,
				{
					HIWORD(GetAsyncKeyState(VK_MENU))		!= 0,
					HIWORD(GetAsyncKeyState(VK_CONTROL)) 	!= 0,
					HIWORD(GetAsyncKeyState(VK_SHIFT))		!= 0,
					HIWORD(GetAsyncKeyState(VK_LWIN)) 	|| HIWORD(GetAsyncKeyState(VK_RWIN)),
				}
			}
		};
	#if UF_HOOK_USE_JSON			
		ext::json::Value json;	
		json["type"] 							= FMT_FORMAT("{}.{}", event.type, (event.key.state == -1) ? "Pressed" : "Released");
		json["invoker"] 						= event.invoker;
		json["key"]["code"] 					= "";
		json["key"]["raw"] 						= 0;
		json["key"]["state"] 					= (event.key.state == -1) ? "Down" : "Up";
		json["key"]["async"] 					= event.key.async;
		json["key"]["modifier"]["alt"]			= event.key.modifier.alt;
		json["key"]["modifier"]["control"] 		= event.key.modifier.ctrl;
		json["key"]["modifier"]["shift"]		= event.key.modifier.shift;
		json["key"]["modifier"]["system"]  		= event.key.modifier.sys;
	#endif
		for ( auto& key : keys ) {
			const auto code = GetKeyName(key);
			event.key.code 	= code;
			event.key.raw  	= key;

		auto eventName = FMT_FORMAT("{}.{}", event.type, code);
		#if UF_HOOK_USE_USERDATA
			this->pushEvent(event.type, event);
			this->pushEvent(eventName, event);
		#endif
		#if UF_HOOK_USE_JSON			
			json["key"]["code"] = code;
			json["key"]["raw"] = key;
			this->pushEvent(event.type, json);
			this->pushEvent(eventName, json);
		#endif
		}
	}
}
bool spec::win32::Window::pollEvents( bool block ) {
	if ( this->m_events.empty() ) {
		do {
			this->processEvents();
		} while ( block && this->m_events.empty() );
	}

	while ( !this->m_events.empty() ) {
		auto& event = this->m_events.front();
		uf::hooks.call( "window:Event", event.payload );
		uf::hooks.call( event.name, event.payload );
		this->m_events.pop();
	}
	return true;
}

void spec::win32::Window::registerWindowClass() {
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
void spec::win32::Window::processEvent(UINT message, WPARAM wParam, LPARAM lParam) {
	if (!this->m_handle) return;
	
	uf::stl::string hook = "window:Unknown";
	uf::stl::string serialized = "";
	uf::stl::stringstream serializer;
	bool labelAsDelta = true;

	ext::json::Value json;

	switch (message) {
		case WM_DESTROY:
			this->terminate();
		break;
		case WM_SETCURSOR:
			if (LOWORD(lParam) == HTCLIENT) SetCursor(this->m_cursor);
		break;
		case WM_CLOSE: {
			pod::payloads::windowEvent event{
				"window:Closed",
				"os",
			};
		#if UF_HOOK_USE_USERDATA
			this->pushEvent(event.type, event);
		#endif
		#if UF_HOOK_USE_JSON
			ext::json::Value json;
			json["type"] = event.type;
			json["invoker"] = event.invoker;
			this->pushEvent(event.type, json);
		#endif
		} break;
		case WM_SIZE: {
			if (wParam == SIZE_MINIMIZED || this->m_resizing || this->m_lastSize == getSize()) break;
			this->m_lastSize = this->getSize();

			pod::payloads::windowResized event{
				{
					"window:Resized",
					"os",
				},
				{ this->m_lastSize },
			};
		#if UF_HOOK_USE_USERDATA
			this->pushEvent(event.type, event);
		#endif
		#if UF_HOOK_USE_JSON
			ext::json::Value json;
			json["type"] = event.type;
			json["invoker"] = event.invoker;
			json["window"]["size"] = uf::vector::encode(event.window.size);
			this->pushEvent(event.type, json);
		#endif
			this->grabMouse(this->m_mouseGrabbed);
		} break;
		case WM_ENTERSIZEMOVE: {	
			this->m_resizing = true;
			this->grabMouse(false);
		} break;
		case WM_EXITSIZEMOVE:{
			this->m_resizing = false;
			if( this->m_lastSize != this->getSize() ) {
				this->m_lastSize = this->getSize();

				pod::payloads::windowResized event = {
					{
						"window:Resized",
						"os",
					},
					{ this->m_lastSize },
				};
			#if UF_HOOK_USE_USERDATA
				this->pushEvent(event.type, event);
			#endif
			#if UF_HOOK_USE_JSON
				ext::json::Value json;
				json["type"] = event.type;
				json["invoker"] = event.invoker;
				json["window"]["size"] = uf::vector::encode(event.window.size);
				this->pushEvent(event.type, json);
			#endif
			} else {
				pod::payloads::windowEvent event{
					"window:Moved",
					"os",
				};
			#if UF_HOOK_USE_USERDATA
				this->pushEvent(event.type, event);
			#endif
			#if UF_HOOK_USE_JSON
				ext::json::Value json;
				json["type"] = event.type;
				json["invoker"] = event.invoker;
				this->pushEvent(event.type, json);
			#endif
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
			int_fast8_t state{};
			switch ( message ) {
				case WM_SETFOCUS: state 	=  1; break;
				case WM_KILLFOCUS: state 	= -1; break;
			}
			pod::payloads::windowFocusedChanged event{
				{
					"window:Focus.Changed",
					"os",
				},
				{
					state
				}
			};
		#if UF_HOOK_USE_USERDATA
			this->pushEvent(event.type, event);
		#endif
		#if UF_HOOK_USE_JSON
			ext::json::Value json;
			json["type"] = event.type;
			json["invoker"] = event.invoker;
			switch ( message ) {
				case WM_SETFOCUS: json["window"]["state"] = "Gained"; break;
				case WM_KILLFOCUS: json["window"]["state"] =  "Lost"; break;
			}
			this->pushEvent(event.type, json);
		#endif
		} break;
		// Text event
		case WM_CHAR: if ( /*true ||*/ this->m_syncParse ) {
		#if 0
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
					std::basic_string<char> 		utf8; 	utf8.reserve(utf32.length());
					uf::Utf32::toUtf8(utf32.begin(), utf32.end(), std::back_inserter(utf8));

					pod::payloads::windowTextEntered event{
						{
							"window:Text.Entered",
							"os",
						},
						{
							character,
							uf::stl::string(utf8.begin(), utf8.end()),
						}
					};
				#if UF_HOOK_USE_USERDATA
					this->pushEvent(event.type, event);
				#endif
				#if UF_HOOK_USE_JSON
					ext::json::Value json;
					json["type"] = event.type;
					json["invoker"] = event.invoker;
					json["text"]["uint32_t"] = event.text.utf32;
					json["text"]["unicode"] = event.text.unicode;
					this->pushEvent(event.type, json);
				#endif
				}
			}
		#endif
		} break;
		case WM_KEYDOWN:
		case WM_SYSKEYDOWN:
		case WM_KEYUP:
		case WM_SYSKEYUP: if ( /*true ||*/ this->m_syncParse ) {
			if (this->m_keyRepeatEnabled || ((HIWORD(lParam) & KF_REPEAT) == 0)) {
				int_fast8_t state = 0;
				switch ( message ) {
					case WM_KEYDOWN:
					case WM_SYSKEYDOWN: state 	= -1; break;

					case WM_KEYUP:
					case WM_SYSKEYUP: state 	=  1; break;
				}
				pod::payloads::windowKey event{
					{
						"window:Key",
						"os",
					},
					{
						this->getKey(wParam, lParam),
						wParam,
						state,
						false,
						{	
							.alt		= HIWORD(GetAsyncKeyState(VK_MENU))		!= 0,
							.ctrl 		= HIWORD(GetAsyncKeyState(VK_CONTROL)) 	!= 0,
							.shift		= HIWORD(GetAsyncKeyState(VK_SHIFT))	!= 0,
							.sys  		= HIWORD(GetAsyncKeyState(VK_LWIN)) 	|| HIWORD(GetAsyncKeyState(VK_RWIN)),
						}
					}
				};
				auto eventName = FMT_FORMAT("{}.{}", event.type, event.key.code);
				#if UF_HOOK_USE_USERDATA
					this->pushEvent(event.type, event);
					this->pushEvent(eventName, event);
				#endif
				#if UF_HOOK_USE_JSON
					ext::json::Value json;
					json["type"] 							= FMT_FORMAT("{}.{}", event.type, (event.key.state == -1) ? "Pressed" : "Released");
					json["invoker"] 						= event.invoker;
					json["key"]["state"] 					= (event.key.state == -1) ? "Down" : "Up";
					json["key"]["async"] 					= event.key.async;
					json["key"]["modifier"]["alt"]			= event.key.modifier.alt;
					json["key"]["modifier"]["control"] 		= event.key.modifier.ctrl;
					json["key"]["modifier"]["shift"]		= event.key.modifier.shift;
					json["key"]["modifier"]["system"]  		= event.key.modifier.sys;
					
					json["key"]["code"] 					= event.key.code;
					json["key"]["raw"] 						= event.key.raw;
					json["key"]["lparam"] 					= lParam;
					this->pushEvent(event.type, json);
					this->pushEvent(eventName, json);
				#endif
			}
		} break;
		case WM_MOUSEWHEEL: {
			POINT position;
			position.x = static_cast<int16_t>(LOWORD(lParam));
			position.y = static_cast<int16_t>(HIWORD(lParam));
			ScreenToClient(this->m_handle, &position);

			int16_t delta = static_cast<int16_t>(HIWORD(wParam));
			pod::payloads::windowMouseWheel event{
				{
					"window:Mouse.Wheel",
					"os",
				},
				{
					pod::Vector2ui{ position.x, position.y },
					::lastMouseWheel = delta,
				}
			};
		#if UF_HOOK_USE_USERDATA
			this->pushEvent(event.type, event);
		#endif
		#if UF_HOOK_USE_JSON
			ext::json::Value json;
			json["type"] = event.type;
			json["invoker"] = event.invoker;
			json["mouse"]["position"] = uf::vector::encode(event.mouse.position);
			json["mouse"]["delta"] = event.mouse.delta;
			this->pushEvent(event.type, json);
		#endif
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
			
			const pod::Vector2i currentPosition = pod::Vector2ui{ static_cast<int16_t>(LOWORD(lParam)), static_cast<int16_t>(HIWORD(lParam)) };
			uf::stl::string button = ""; 
			int_fast8_t state = 0;

			switch ( message ) {	
				case WM_LBUTTONDOWN:
				case WM_LBUTTONUP: 		button = "Left"; break;
				
				case WM_RBUTTONDOWN:
				case WM_RBUTTONUP: 		button = "Right"; break;
				
				case WM_MBUTTONDOWN:
				case WM_MBUTTONUP: 		button = "Middle"; break;
				
				case WM_XBUTTONDOWN:
				case WM_XBUTTONUP: 		button = "Aux"; break;

				default: 				button = "???"; break;
			}
			switch ( message ) {	
				case WM_LBUTTONUP:
				case WM_RBUTTONUP:
				case WM_MBUTTONUP:
				case WM_XBUTTONUP: 		state = 1; break;

				case WM_LBUTTONDOWN:
				case WM_RBUTTONDOWN:
				case WM_MBUTTONDOWN:
				case WM_XBUTTONDOWN: 	state = -1; break;
				default: 				state =  0; break; 
			}
			pod::payloads::windowMouseClick event{
				{
					"window:Mouse.Click",
					"os",
				},
				{
					currentPosition,
					currentPosition - lastPosition,
					button,
					state
				}
			};
		#if UF_HOOK_USE_USERDATA
			this->pushEvent(event.type, event);
		#endif
		#if UF_HOOK_USE_JSON
			ext::json::Value json;
			json["type"] = event.type;
			json["invoker"] = event.invoker;
			json["mouse"]["position"] = uf::vector::encode(event.mouse.position);
			json["mouse"]["delta"] = uf::vector::encode(event.mouse.delta);
			json["mouse"]["button"] = event.mouse.button;
			switch (event.mouse.state) {
				case 1: json["mouse"]["state"] = "Up"; break;
				case -1: json["mouse"]["state"] = "Down"; break;
			}
			this->pushEvent(event.type, json);
		#endif

			lastPosition = currentPosition;
		} break;
		case WM_MOUSELEAVE:
			if ( this->m_mouseInside ) {
				this->m_mouseInside = false;
				pod::payloads::windowMouseMoved event{
					{
						{
							"window:Mouse.Moved",
							"client",
						},
						{
							{},
						},
					},
					{
						{},
						{},
						-1
					}
				};
			#if UF_HOOK_USE_USERDATA
				this->pushEvent(event.type, event);
			#endif
			#if UF_HOOK_USE_JSON
				ext::json::Value json;
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
			#endif
			}
		break;
		case WM_MOUSEMOVE: {
			static pod::Vector2i lastPosition = {};
			const pod::Vector2i currentPosition = {
				static_cast<int16_t>(LOWORD(lParam)),
				static_cast<int16_t>(HIWORD(lParam)),
			};
			RECT area;
			GetClientRect(this->m_handle, &area);
			if ((wParam & (MK_LBUTTON | MK_MBUTTON | MK_RBUTTON | MK_XBUTTON1 | MK_XBUTTON2)) == 0) {
				if (GetCapture() == this->m_handle) ReleaseCapture();
			}
			else if (GetCapture() != this->m_handle) {
				SetCapture(this->m_handle);
			}

			int_fast8_t state{};
			if ((currentPosition.x < area.left) || (currentPosition.x > area.right) || (currentPosition.y < area.top) || (currentPosition.y > area.bottom)) {
				if ( this->m_mouseInside ) {
					this->m_mouseInside = false;
					this->setTracking(false);
					state = -1;
				}
			} else {
				if ( !this->m_mouseInside ) {
					this->m_mouseInside = true;
					this->setTracking(true);
					state = 1;
				}
			}
			pod::payloads::windowMouseMoved event{
				{
					{
						"window:Mouse.Moved",
						"client",
					},
					{
						pod::Vector2ui{ area.right, area.bottom },
					},
				},
				{
					currentPosition,
					currentPosition - lastPosition,
					0
				}
			};
		#if UF_HOOK_USE_USERDATA
			this->pushEvent(event.type, event);
		#endif
		#if UF_HOOK_USE_JSON
			ext::json::Value json;
			json["type"] = event.type;
			json["invoker"] = event.invoker;

			json["window"]["size"] = uf::vector::encode(event.window.size);

			json["mouse"]["position"] = uf::vector::encode(event.mouse.position);
			json["mouse"]["delta"] = uf::vector::encode(event.mouse.delta);
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
		#endif

			lastPosition = currentPosition;
		break;
		}
	}
}

void spec::win32::Window::setTracking(bool state) {
	TRACKMOUSEEVENT mouseEvent;
	mouseEvent.cbSize = sizeof(TRACKMOUSEEVENT);
	mouseEvent.hwndTrack = this->m_handle;
	mouseEvent.dwFlags = state ? TME_LEAVE : TME_CANCEL;
	mouseEvent.dwHoverTime = HOVER_DEFAULT;
	TrackMouseEvent(&mouseEvent);
}
void spec::win32::Window::setMouseGrabbed(bool state) {
	this->m_mouseGrabbed = state;
	this->grabMouse(state);
}
void spec::win32::Window::grabMouse(bool state) {
	if (state) {
		RECT rect;
		GetClientRect(m_handle, &rect);
		MapWindowPoints(m_handle, NULL, reinterpret_cast<LPPOINT>(&rect), 2);
		ClipCursor(&rect);
	} else {
		ClipCursor(NULL);
	}
}
pod::Vector2ui spec::win32::Window::getResolution_v() {
	return { GetSystemMetrics(SM_CXSCREEN), GetSystemMetrics(SM_CYSCREEN) };
}
void spec::win32::Window::toggleFullscreen( bool borderless ) {
	static pod::Vector2ui lastSize = this->getSize();
	static LONG lastStyle;
	static LONG lastExStyle;

	if ( fullscreenWindow == (void*) this ) {
		fullscreenWindow = NULL;
		SetWindowLong(this->m_handle, GWL_STYLE, lastStyle);
		SetWindowLong(this->m_handle, GWL_EXSTYLE, lastExStyle);

		this->setSize( lastSize );
		this->centerWindow();
		return;
	}

	lastSize = this->getSize();
	lastStyle = GetWindowLong(this->m_handle, GWL_STYLE);
	lastExStyle = GetWindowLong(this->m_handle, GWL_EXSTYLE);

	if ( borderless ) {
		SetWindowLong(this->m_handle, GWL_STYLE, WS_POPUP );
	//	SetWindowLong(this->m_handle, GWL_EXSTYLE, 0);
		SetWindowPos(this->m_handle, HWND_TOP, 0, 0, GetSystemMetrics(SM_CXSCREEN), GetSystemMetrics(SM_CYSCREEN), SWP_FRAMECHANGED);
		ShowWindow(this->m_handle, SW_SHOW);
		return;
	}

	SetWindowLong(this->m_handle, GWL_STYLE, WS_POPUP | WS_CLIPCHILDREN | WS_CLIPSIBLINGS);
	SetWindowLong(this->m_handle, GWL_EXSTYLE, WS_EX_APPWINDOW);
	SetWindowPos(this->m_handle, HWND_TOP, 0, 0, GetSystemMetrics(SM_CXSCREEN), GetSystemMetrics(SM_CYSCREEN), SWP_FRAMECHANGED);
	ShowWindow(this->m_handle, SW_SHOW);

	fullscreenWindow = (void*) this;
}

bool spec::win32::Window::isKeyPressed_v(const uf::stl::string& key) {
	return uf::Window::focused && (GetAsyncKeyState( GetKeyCode( key ) ) & 0x8000);
}
uf::stl::string spec::win32::Window::getKey(WPARAM key, LPARAM flags) {
	return GetKeyName( key, flags );
}
#if defined(UF_USE_VULKAN) && UF_USE_VULKAN == 1
uf::stl::vector<uf::stl::string> spec::win32::Window::getExtensions( bool validationEnabled ) {
	uf::stl::vector<uf::stl::string> instanceExtensions = { VK_KHR_SURFACE_EXTENSION_NAME, VK_KHR_WIN32_SURFACE_EXTENSION_NAME };
	if ( validationEnabled ) instanceExtensions.push_back(VK_EXT_DEBUG_UTILS_EXTENSION_NAME);
	return instanceExtensions;
}
void spec::win32::Window::createSurface( VkInstance instance, VkSurfaceKHR& surface ) {
	VkWin32SurfaceCreateInfoKHR surfaceCreateInfo = {};
	surfaceCreateInfo.sType = VK_STRUCTURE_TYPE_WIN32_SURFACE_CREATE_INFO_KHR;
	surfaceCreateInfo.hinstance = (HINSTANCE) GetModuleHandleW(NULL);
	surfaceCreateInfo.hwnd = (HWND) this->m_handle; 
	vkCreateWin32SurfaceKHR( instance, &surfaceCreateInfo, nullptr, &surface);
}
#endif

void spec::win32::Window::display() {
#if UF_USE_OPENGL && UF_OPENGL_CONTEXT_IN_WINDOW
	if ( this->m_context ){
		spec::Context* context = (spec::Context*) this->m_context;
		if ( context->setActive(true) ) context->display();
	}
#endif
}

#endif