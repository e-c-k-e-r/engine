#include <uf/spec/window/window.h>
#include <uf/utils/io/iostream.h>
#include <uf/utils/string/utf.h>
#include <uf/utils/serialize/serializer.h>
#include <uf/utils/window/payloads.h>

#define UF_USE_USERDATA 1
#define UF_USE_JSON 1

#if UF_ENV_WINDOWS && (!UF_USE_SFML || (UF_USE_SFML && UF_USE_SFML == 0))
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

	std::vector<WPARAM> GetKeys() {
		std::vector<WPARAM> keys;
		keys.reserve(8);

		if ( GetAsyncKeyState('A') & 0x8000 ) keys.push_back('A');
		if ( GetAsyncKeyState('B') & 0x8000 ) keys.push_back('B');
		if ( GetAsyncKeyState('C') & 0x8000 ) keys.push_back('C');
		if ( GetAsyncKeyState('D') & 0x8000 ) keys.push_back('D');
		if ( GetAsyncKeyState('E') & 0x8000 ) keys.push_back('E');
		if ( GetAsyncKeyState('F') & 0x8000 ) keys.push_back('F');
		if ( GetAsyncKeyState('G') & 0x8000 ) keys.push_back('G');
		if ( GetAsyncKeyState('H') & 0x8000 ) keys.push_back('H');
		if ( GetAsyncKeyState('I') & 0x8000 ) keys.push_back('I');
		if ( GetAsyncKeyState('J') & 0x8000 ) keys.push_back('J');
		if ( GetAsyncKeyState('K') & 0x8000 ) keys.push_back('K');
		if ( GetAsyncKeyState('L') & 0x8000 ) keys.push_back('L');
		if ( GetAsyncKeyState('M') & 0x8000 ) keys.push_back('M');
		if ( GetAsyncKeyState('N') & 0x8000 ) keys.push_back('N');
		if ( GetAsyncKeyState('O') & 0x8000 ) keys.push_back('O');
		if ( GetAsyncKeyState('P') & 0x8000 ) keys.push_back('P');
		if ( GetAsyncKeyState('Q') & 0x8000 ) keys.push_back('Q');
		if ( GetAsyncKeyState('R') & 0x8000 ) keys.push_back('R');
		if ( GetAsyncKeyState('S') & 0x8000 ) keys.push_back('S');
		if ( GetAsyncKeyState('T') & 0x8000 ) keys.push_back('T');
		if ( GetAsyncKeyState('U') & 0x8000 ) keys.push_back('U');
		if ( GetAsyncKeyState('V') & 0x8000 ) keys.push_back('V');
		if ( GetAsyncKeyState('W') & 0x8000 ) keys.push_back('W');
		if ( GetAsyncKeyState('X') & 0x8000 ) keys.push_back('X');
		if ( GetAsyncKeyState('Y') & 0x8000 ) keys.push_back('Y');
		if ( GetAsyncKeyState('Z') & 0x8000 ) keys.push_back('Z');
		if ( GetAsyncKeyState('0') & 0x8000 ) keys.push_back('0');
		if ( GetAsyncKeyState('1') & 0x8000 ) keys.push_back('1');
		if ( GetAsyncKeyState('2') & 0x8000 ) keys.push_back('2');
		if ( GetAsyncKeyState('3') & 0x8000 ) keys.push_back('3');
		if ( GetAsyncKeyState('4') & 0x8000 ) keys.push_back('4');
		if ( GetAsyncKeyState('5') & 0x8000 ) keys.push_back('5');
		if ( GetAsyncKeyState('6') & 0x8000 ) keys.push_back('6');
		if ( GetAsyncKeyState('7') & 0x8000 ) keys.push_back('7');
		if ( GetAsyncKeyState('8') & 0x8000 ) keys.push_back('8');
		if ( GetAsyncKeyState('9') & 0x8000 ) keys.push_back('9');
		if ( GetAsyncKeyState(VK_ESCAPE) & 0x8000 ) keys.push_back(VK_ESCAPE);
		if ( GetAsyncKeyState(VK_LCONTROL) & 0x8000 ) keys.push_back(VK_LCONTROL);
		if ( GetAsyncKeyState(VK_LSHIFT) & 0x8000 ) keys.push_back(VK_LSHIFT);
		if ( GetAsyncKeyState(VK_LMENU) & 0x8000 ) keys.push_back(VK_LMENU);
		if ( GetAsyncKeyState(VK_LWIN) & 0x8000 ) keys.push_back(VK_LWIN);
		if ( GetAsyncKeyState(VK_RCONTROL) & 0x8000 ) keys.push_back(VK_RCONTROL);
		if ( GetAsyncKeyState(VK_RSHIFT) & 0x8000 ) keys.push_back(VK_RSHIFT);
		if ( GetAsyncKeyState(VK_RMENU) & 0x8000 ) keys.push_back(VK_RMENU);
		if ( GetAsyncKeyState(VK_RWIN) & 0x8000 ) keys.push_back(VK_RWIN);
		if ( GetAsyncKeyState(VK_APPS) & 0x8000 ) keys.push_back(VK_APPS);
		if ( GetAsyncKeyState(VK_OEM_4) & 0x8000 ) keys.push_back(VK_OEM_4);
		if ( GetAsyncKeyState(VK_OEM_6) & 0x8000 ) keys.push_back(VK_OEM_6);
		if ( GetAsyncKeyState(VK_OEM_1) & 0x8000 ) keys.push_back(VK_OEM_1);
		if ( GetAsyncKeyState(VK_OEM_COMMA) & 0x8000 ) keys.push_back(VK_OEM_COMMA);
		if ( GetAsyncKeyState(VK_OEM_PERIOD) & 0x8000 ) keys.push_back(VK_OEM_PERIOD);
		if ( GetAsyncKeyState(VK_OEM_7) & 0x8000 ) keys.push_back(VK_OEM_7);
		if ( GetAsyncKeyState(VK_OEM_2) & 0x8000 ) keys.push_back(VK_OEM_2);
		if ( GetAsyncKeyState(VK_OEM_5) & 0x8000 ) keys.push_back(VK_OEM_5);
		if ( GetAsyncKeyState(VK_OEM_3) & 0x8000 ) keys.push_back(VK_OEM_3);
		if ( GetAsyncKeyState(VK_OEM_PLUS) & 0x8000 ) keys.push_back(VK_OEM_PLUS);
		if ( GetAsyncKeyState(VK_OEM_MINUS) & 0x8000 ) keys.push_back(VK_OEM_MINUS);
		if ( GetAsyncKeyState(VK_SPACE) & 0x8000 ) keys.push_back(VK_SPACE);
		if ( GetAsyncKeyState(VK_RETURN) & 0x8000 ) keys.push_back(VK_RETURN);
		if ( GetAsyncKeyState(VK_BACK) & 0x8000 ) keys.push_back(VK_BACK);
		if ( GetAsyncKeyState(VK_TAB) & 0x8000 ) keys.push_back(VK_TAB);
		if ( GetAsyncKeyState(VK_PRIOR) & 0x8000 ) keys.push_back(VK_PRIOR);
		if ( GetAsyncKeyState(VK_NEXT) & 0x8000 ) keys.push_back(VK_NEXT);
		if ( GetAsyncKeyState(VK_END) & 0x8000 ) keys.push_back(VK_END);
		if ( GetAsyncKeyState(VK_HOME) & 0x8000 ) keys.push_back(VK_HOME);
		if ( GetAsyncKeyState(VK_INSERT) & 0x8000 ) keys.push_back(VK_INSERT);
		if ( GetAsyncKeyState(VK_DELETE) & 0x8000 ) keys.push_back(VK_DELETE);
		if ( GetAsyncKeyState(VK_ADD) & 0x8000 ) keys.push_back(VK_ADD);
		if ( GetAsyncKeyState(VK_SUBTRACT) & 0x8000 ) keys.push_back(VK_SUBTRACT);
		if ( GetAsyncKeyState(VK_MULTIPLY) & 0x8000 ) keys.push_back(VK_MULTIPLY);
		if ( GetAsyncKeyState(VK_DIVIDE) & 0x8000 ) keys.push_back(VK_DIVIDE);
		if ( GetAsyncKeyState(VK_LEFT) & 0x8000 ) keys.push_back(VK_LEFT);
		if ( GetAsyncKeyState(VK_RIGHT) & 0x8000 ) keys.push_back(VK_RIGHT);
		if ( GetAsyncKeyState(VK_UP) & 0x8000 ) keys.push_back(VK_UP);
		if ( GetAsyncKeyState(VK_DOWN) & 0x8000 ) keys.push_back(VK_DOWN);
		if ( GetAsyncKeyState(VK_NUMPAD0) & 0x8000 ) keys.push_back(VK_NUMPAD0);
		if ( GetAsyncKeyState(VK_NUMPAD1) & 0x8000 ) keys.push_back(VK_NUMPAD1);
		if ( GetAsyncKeyState(VK_NUMPAD2) & 0x8000 ) keys.push_back(VK_NUMPAD2);
		if ( GetAsyncKeyState(VK_NUMPAD3) & 0x8000 ) keys.push_back(VK_NUMPAD3);
		if ( GetAsyncKeyState(VK_NUMPAD4) & 0x8000 ) keys.push_back(VK_NUMPAD4);
		if ( GetAsyncKeyState(VK_NUMPAD5) & 0x8000 ) keys.push_back(VK_NUMPAD5);
		if ( GetAsyncKeyState(VK_NUMPAD6) & 0x8000 ) keys.push_back(VK_NUMPAD6);
		if ( GetAsyncKeyState(VK_NUMPAD7) & 0x8000 ) keys.push_back(VK_NUMPAD7);
		if ( GetAsyncKeyState(VK_NUMPAD8) & 0x8000 ) keys.push_back(VK_NUMPAD8);
		if ( GetAsyncKeyState(VK_NUMPAD9) & 0x8000 ) keys.push_back(VK_NUMPAD9);
		if ( GetAsyncKeyState(VK_F1) & 0x8000 ) keys.push_back(VK_F1);
		if ( GetAsyncKeyState(VK_F2) & 0x8000 ) keys.push_back(VK_F2);
		if ( GetAsyncKeyState(VK_F3) & 0x8000 ) keys.push_back(VK_F3);
		if ( GetAsyncKeyState(VK_F4) & 0x8000 ) keys.push_back(VK_F4);
		if ( GetAsyncKeyState(VK_F5) & 0x8000 ) keys.push_back(VK_F5);
		if ( GetAsyncKeyState(VK_F6) & 0x8000 ) keys.push_back(VK_F6);
		if ( GetAsyncKeyState(VK_F7) & 0x8000 ) keys.push_back(VK_F7);
		if ( GetAsyncKeyState(VK_F8) & 0x8000 ) keys.push_back(VK_F8);
		if ( GetAsyncKeyState(VK_F9) & 0x8000 ) keys.push_back(VK_F9);
		if ( GetAsyncKeyState(VK_F10) & 0x8000 ) keys.push_back(VK_F10);
		if ( GetAsyncKeyState(VK_F11) & 0x8000 ) keys.push_back(VK_F11);
		if ( GetAsyncKeyState(VK_F12) & 0x8000 ) keys.push_back(VK_F12);
		if ( GetAsyncKeyState(VK_F13) & 0x8000 ) keys.push_back(VK_F13);
		if ( GetAsyncKeyState(VK_F14) & 0x8000 ) keys.push_back(VK_F14);
		if ( GetAsyncKeyState(VK_F15) & 0x8000 ) keys.push_back(VK_F15);
		if ( GetAsyncKeyState(VK_PAUSE) & 0x8000 ) keys.push_back(VK_PAUSE);

   		if ( GetAsyncKeyState(VK_LBUTTON) & 0x8000 ) keys.push_back(VK_LBUTTON);
		if ( GetAsyncKeyState(VK_RBUTTON) & 0x8000 ) keys.push_back(VK_RBUTTON);
		if ( GetAsyncKeyState(VK_MBUTTON) & 0x8000 ) keys.push_back(VK_MBUTTON);
		if ( GetAsyncKeyState(VK_XBUTTON1) & 0x8000 ) keys.push_back(VK_XBUTTON1);
		if ( GetAsyncKeyState(VK_XBUTTON2) & 0x8000 ) keys.push_back(VK_XBUTTON2);
		return keys;
	}
	std::string _GetKeyName( WPARAM key, LPARAM flags = 0 ) {
	#if 1
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
	#else
		switch ( key ) {
			case 'A': return "A";
			case 'B': return "B";
			case 'C': return "C";
			case 'D': return "D";
			case 'E': return "E";
			case 'F': return "F";
			case 'G': return "G";
			case 'H': return "H";
			case 'I': return "I";
			case 'J': return "J";
			case 'K': return "K";
			case 'L': return "L";
			case 'M': return "M";
			case 'N': return "N";
			case 'O': return "O";
			case 'P': return "P";
			case 'Q': return "Q";
			case 'R': return "R";
			case 'S': return "S";
			case 'T': return "T";
			case 'U': return "U";
			case 'V': return "V";
			case 'W': return "W";
			case 'X': return "X";
			case 'Y': return "Y";
			case 'Z': return "Z";
			case '0': return "0";
			case '1': return "1";
			case '2': return "2";
			case '3': return "3";
			case '4': return "4";
			case '5': return "5";
			case '6': return "6";
			case '7': return "7";
			case '8': return "8";
			case '9': return "9";
			case VK_ESCAPE: return "Escape";
			case VK_LCONTROL: return "LControl";
			case VK_LSHIFT: return "LShift";
			case VK_LMENU: return "LAlt";
			case VK_LWIN: return "LSystem";
			case VK_RCONTROL: return "RControl";
			case VK_RSHIFT: return "RShift";
			case VK_RMENU: return "RAlt";
			case VK_RWIN: return "RSystem";
			case VK_APPS: return "Apps";
			case VK_OEM_4: return "OEM4";
			case VK_OEM_6: return "OEM6";
			case VK_OEM_1: return "OEM1";
			case VK_OEM_COMMA: return "OEMComma";
			case VK_OEM_PERIOD: return "OEMPeriod";
			case VK_OEM_7: return "OEM7";
			case VK_OEM_2: return "OEM2";
			case VK_OEM_5: return "OEM5";
			case VK_OEM_3: return "OEM3";
			case VK_OEM_PLUS: return "OEM+";
			case VK_OEM_MINUS: return "OEM-";
			case VK_SPACE: return " ";
			case VK_RETURN: return "Enter";
			case VK_BACK: return "Back";
			case VK_TAB: return "Tab";
			case VK_PRIOR: return "Prior";
			case VK_NEXT: return "Next";
			case VK_END: return "End";
			case VK_HOME: return "Home";
			case VK_INSERT: return "Insert";
			case VK_DELETE: return "Delete";
			case VK_ADD: return "+";
			case VK_SUBTRACT: return "-";
			case VK_MULTIPLY: return "*";
			case VK_DIVIDE: return "/";
			case VK_LEFT: return "Left";
			case VK_RIGHT: return "Right";
			case VK_UP: return "Up";
			case VK_DOWN: return "Down";
			case VK_NUMPAD0: return "Num0";
			case VK_NUMPAD1: return "Num1";
			case VK_NUMPAD2: return "Num2";
			case VK_NUMPAD3: return "Num3";
			case VK_NUMPAD4: return "Num4";
			case VK_NUMPAD5: return "Num5";
			case VK_NUMPAD6: return "Num6";
			case VK_NUMPAD7: return "Num7";
			case VK_NUMPAD8: return "Num8";
			case VK_NUMPAD9: return "Num9";
			case VK_F1: return "F1";
			case VK_F2: return "F2";
			case VK_F3: return "F3";
			case VK_F4: return "F4";
			case VK_F5: return "F5";
			case VK_F6: return "F6";
			case VK_F7: return "F7";
			case VK_F8: return "F8";
			case VK_F9: return "F9";
			case VK_F10: return "F10";
			case VK_F11: return "F11";
			case VK_F12: return "F12";
			case VK_F13: return "F13";
			case VK_F14: return "F14";
			case VK_F15: return "F15";
			case VK_PAUSE: return "Pause";

			case VK_LBUTTON: return "LeftMouse";
			case VK_RBUTTON: return "RightMouse";
			case VK_MBUTTON: return "MiddleMouse";
			case VK_XBUTTON1: return "XButton1";
			case VK_XBUTTON2: return "XButton2";
		}
	#endif
		return std::to_string((int) key);
	}
	std::string GetKeyName( WPARAM key, LPARAM flags = 0 ) {
		return uf::string::uppercase( _GetKeyName( key, flags ) );
	}

	WPARAM GetKeyCode( const std::string& _name ) {
		std::string name = uf::string::uppercase(_name);
		if ( name == "A" ) return 'A';
		else if ( name == "B" ) return 'B';
		else if ( name == "C" ) return 'C';
		else if ( name == "D" ) return 'D';
		else if ( name == "E" ) return 'E';
		else if ( name == "F" ) return 'F';
		else if ( name == "G" ) return 'G';
		else if ( name == "H" ) return 'H';
		else if ( name == "I" ) return 'I';
		else if ( name == "J" ) return 'J';
		else if ( name == "K" ) return 'K';
		else if ( name == "L" ) return 'L';
		else if ( name == "M" ) return 'M';
		else if ( name == "N" ) return 'N';
		else if ( name == "O" ) return 'O';
		else if ( name == "P" ) return 'P';
		else if ( name == "Q" ) return 'Q';
		else if ( name == "R" ) return 'R';
		else if ( name == "S" ) return 'S';
		else if ( name == "T" ) return 'T';
		else if ( name == "U" ) return 'U';
		else if ( name == "V" ) return 'V';
		else if ( name == "W" ) return 'W';
		else if ( name == "X" ) return 'X';
		else if ( name == "Y" ) return 'Y';
		else if ( name == "Z" ) return 'Z';
		else if ( name == "0" ) return '0';
		else if ( name == "1" ) return '1';
		else if ( name == "2" ) return '2';
		else if ( name == "3" ) return '3';
		else if ( name == "4" ) return '4';
		else if ( name == "5" ) return '5';
		else if ( name == "6" ) return '6';
		else if ( name == "7" ) return '7';
		else if ( name == "8" ) return '8';
		else if ( name == "9" ) return '9';
		else if ( name == "ESCAPE" ) return VK_ESCAPE;
		else if ( name == "LCONTROL" ) return VK_LCONTROL;
		else if ( name == "LSHIFT" ) return VK_LSHIFT;
		else if ( name == "LALT" ) return VK_LMENU;
		else if ( name == "LSYSTEM" ) return VK_LWIN;
		else if ( name == "RCONTROL" ) return VK_RCONTROL;
		else if ( name == "RSHIFT" ) return VK_RSHIFT;
		else if ( name == "RALT" ) return VK_RMENU;
		else if ( name == "RSYSTEM" ) return VK_RWIN;
		else if ( name == "APPS" ) return VK_APPS;
		else if ( name == "OEM4" ) return VK_OEM_4;
		else if ( name == "OEM6" ) return VK_OEM_6;
		else if ( name == "OEM1" ) return VK_OEM_1;
		else if ( name == "OEMCOMMA" ) return VK_OEM_COMMA;
		else if ( name == "OEMPERIOD" ) return VK_OEM_PERIOD;
		else if ( name == "OEM7" ) return VK_OEM_7;
		else if ( name == "OEM2" ) return VK_OEM_2;
		else if ( name == "OEM5" ) return VK_OEM_5;
		else if ( name == "OEM3" ) return VK_OEM_3;
		else if ( name == "OEM+" ) return VK_OEM_PLUS;
		else if ( name == "OEM-" ) return VK_OEM_MINUS;
		else if ( name == " " ) return VK_SPACE;
		else if ( name == "ENTER" ) return VK_RETURN;
		else if ( name == "BACK" ) return VK_BACK;
		else if ( name == "TAB" ) return VK_TAB;
		else if ( name == "PRIOR" ) return VK_PRIOR;
		else if ( name == "NEXT" ) return VK_NEXT;
		else if ( name == "END" ) return VK_END;
		else if ( name == "HOME" ) return VK_HOME;
		else if ( name == "INSERT" ) return VK_INSERT;
		else if ( name == "DELETE" ) return VK_DELETE;
		else if ( name == "+" ) return VK_ADD;
		else if ( name == "-" ) return VK_SUBTRACT;
		else if ( name == "*" ) return VK_MULTIPLY;
		else if ( name == "/" ) return VK_DIVIDE;
		else if ( name == "LEFT" ) return VK_LEFT;
		else if ( name == "RIGHT" ) return VK_RIGHT;
		else if ( name == "UP" ) return VK_UP;
		else if ( name == "DOWN" ) return VK_DOWN;
		else if ( name == "NUM0" ) return VK_NUMPAD0;
		else if ( name == "NUM1" ) return VK_NUMPAD1;
		else if ( name == "NUM2" ) return VK_NUMPAD2;
		else if ( name == "NUM3" ) return VK_NUMPAD3;
		else if ( name == "NUM4" ) return VK_NUMPAD4;
		else if ( name == "NUM5" ) return VK_NUMPAD5;
		else if ( name == "NUM6" ) return VK_NUMPAD6;
		else if ( name == "NUM7" ) return VK_NUMPAD7;
		else if ( name == "NUM8" ) return VK_NUMPAD8;
		else if ( name == "NUM9" ) return VK_NUMPAD9;
		else if ( name == "F1" ) return VK_F1;
		else if ( name == "F2" ) return VK_F2;
		else if ( name == "F3" ) return VK_F3;
		else if ( name == "F4" ) return VK_F4;
		else if ( name == "F5" ) return VK_F5;
		else if ( name == "F6" ) return VK_F6;
		else if ( name == "F7" ) return VK_F7;
		else if ( name == "F8" ) return VK_F8;
		else if ( name == "F9" ) return VK_F9;
		else if ( name == "F10" ) return VK_F10;
		else if ( name == "F11" ) return VK_F11;
		else if ( name == "F12" ) return VK_F12;
		else if ( name == "F13" ) return VK_F13;
		else if ( name == "F14" ) return VK_F14;
		else if ( name == "F15" ) return VK_F15;
		else if ( name == "PAUSE" ) return VK_PAUSE;

		else if ( name == "LEFTMOUSE" ) return VK_LBUTTON;
		else if ( name == "RIGHTMOUSE" ) return VK_RBUTTON;
		else if ( name == "MIDDLEMOUSE" ) return VK_MBUTTON;
		else if ( name == "XBUTTON1" ) return VK_XBUTTON1;
		else if ( name == "XBUTTON2" ) return VK_XBUTTON2;
		return 0;
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
	m_syncParse			(true),
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
	m_syncParse			(true),
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
	m_syncParse			(true),
	m_asyncParse		(false)
{
	this->create(size, title);
}
void UF_API_CALL spec::win32::Window::create( const spec::win32::Window::vector_t& _size, const spec::win32::Window::title_t& title ) {
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
size_t UF_API_CALL spec::win32::Window::getRefreshRate() const {
	HDC screenDC = GetDC(NULL);
	int refreshRate = GetDeviceCaps( screenDC, VREFRESH );
	ReleaseDC(NULL, screenDC);
	return refreshRate;
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
	ScreenToClient( this->m_handle, &pt );
	return { pt.x, pt.y };
}
void UF_API_CALL spec::win32::Window::setSize( const spec::win32::Window::vector_t& size ) {
	if ( fullscreenWindow == (void*) this ) return;
	RECT rectangle = { 0, 0, size.x, size.y };
	if ( rectangle.right <= 0 && rectangle.bottom <= 0 ) {
		rectangle.right = GetSystemMetrics(SM_CXSCREEN);
		rectangle.bottom = GetSystemMetrics(SM_CYSCREEN);
	}
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

void UF_API_CALL spec::win32::Window::processEvents() {
	if ( !this->m_callback ) {
		MSG message;
		while ( PeekMessageW( &message, NULL, 0, 0, PM_REMOVE ) ) {
			TranslateMessage(&message);
			DispatchMessage(&message);
		}
	}
	/* Key inputs */ if ( this->m_asyncParse ) {
		std::vector<WPARAM> keys = GetKeys();
		pod::payloads::windowKey event{
			"window:Key",
			"os",
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
	#if UF_USE_JSON			
		uf::Serializer json;	
		json["type"] 							= event.type + "." + ((event.key.state == -1)?"Pressed":"Released");
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

		#if UF_USE_USERDATA
			this->pushEvent(event.type, event);
			this->pushEvent(event.type + "." + code, event);
		#endif
		#if UF_USE_JSON			
			json["key"]["code"] = code;
			json["key"]["raw"] = key;
			this->pushEvent(event.type, json);
			this->pushEvent(event.type + "." + code, json);
		#endif
		}
	}
}
bool UF_API_CALL spec::win32::Window::pollEvents( bool block ) {
	if ( this->m_events.empty() ) {
		do {
			this->processEvents();
		} while ( block && this->m_events.empty() );
	}

	while ( !this->m_events.empty() ) {
		auto& event = this->m_events.front();
		if ( event.payload.is<std::string>() ) {
			ext::json::Value payload = uf::Serializer( event.payload.as<std::string>() );
			uf::hooks.call( event.name, payload );
		} else if ( event.payload.is<uf::Serializer>() ) {
			uf::Serializer& payload = event.payload.as<uf::Serializer>();
			uf::hooks.call( event.name, payload );
		} else if ( event.payload.is<ext::json::Value>() ) {
			ext::json::Value& payload = event.payload.as<ext::json::Value>();
			uf::hooks.call( event.name, payload );
		} else {
			uf::hooks.call( event.name, event.payload );
		}
		this->m_events.pop();
	}
	return true;
#if 0
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
#endif
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
	
	std::string hook = "window:Unknown";
	std::string serialized = "";
	std::stringstream serializer;
	bool labelAsDelta = true;

	uf::Serializer json;

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
		#if UF_USE_USERDATA
			this->pushEvent(event.type, event);
		#endif
		#if UF_USE_JSON
			uf::Serializer json;
			json["type"] = event.type;
			json["invoker"] = event.invoker;
			this->pushEvent(event.type, json);
		#endif
		} break;
		case WM_SIZE: {
			if (wParam == SIZE_MINIMIZED || this->m_resizing || this->m_lastSize == getSize()) break;
			this->m_lastSize = this->getSize();

			pod::payloads::windowResized event{
				"window:Resized",
				"os",
				{ this->m_lastSize },
			};
		#if UF_USE_USERDATA
			this->pushEvent(event.type, event);
		#endif
		#if UF_USE_JSON
			uf::Serializer json;
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
					"window:Resized",
					"os",
					{ this->m_lastSize },
				};
			#if UF_USE_USERDATA
				this->pushEvent(event.type, event);
			#endif
			#if UF_USE_JSON
				uf::Serializer json;
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
			#if UF_USE_USERDATA
				this->pushEvent(event.type, event);
			#endif
			#if UF_USE_JSON
				uf::Serializer json;
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
			pod::payloads::windowFocusedChanged event{
				"window:Focus.Changed",
				"os",
			};
			switch ( message ) {
				case WM_SETFOCUS: event.window.state 	=  1; break;
				case WM_KILLFOCUS: event.window.state 	= -1; break;
			}
		#if UF_USE_USERDATA
			this->pushEvent(event.type, event);
		#endif
		#if UF_USE_JSON
			uf::Serializer json;
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

					pod::payloads::windowTextEntered event{
						"window:Text.Entered",
						"os",
						{
							character,
							std::string(utf8.begin(), utf8.end()),
						}
					};
				#if UF_USE_USERDATA
					this->pushEvent(event.type, event);
				#endif
				#if UF_USE_JSON
					uf::Serializer json;
					json["type"] = event.type;
					json["invoker"] = event.invoker;
					json["text"]["uint32_t"] = event.text.utf32;
					json["text"]["unicode"] = event.text.unicode;
					this->pushEvent(event.type, json);
				#endif
				}
			}
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
						"window:Key",
						"os",
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
				#if UF_USE_USERDATA
					this->pushEvent(event.type, event);
					this->pushEvent(event.type + "." + event.key.code, event);
				#endif
				#if UF_USE_JSON
					uf::Serializer json;
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
					json["key"]["lparam"] 					= lParam;
					this->pushEvent(event.type, json);
					this->pushEvent(event.type + "." + event.key.code, json);
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
				"window:Mouse.Wheel",
				"os",
				{
					pod::Vector2ui{ position.x, position.y },
					delta,
				}
			};
		#if UF_USE_USERDATA
			this->pushEvent(event.type, event);
		#endif
		#if UF_USE_JSON
			uf::Serializer json;
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
			std::string button = ""; 
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
				"window:Mouse.Click",
				"os",
				{
					currentPosition,
					currentPosition - lastPosition,
					button,
					state
				}
			};
		#if UF_USE_USERDATA
			this->pushEvent(event.type, event);
		#endif
		#if UF_USE_JSON
			uf::Serializer json;
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
					"window:Mouse.Moved",
					"os",
					{
						{}
					},
					{
						{},
						{},
						-1
					}
				};
			#if UF_USE_USERDATA
				this->pushEvent(event.type, event);
			#endif
			#if UF_USE_JSON
				uf::Serializer json;
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
				"window:Mouse.Moved",
				"os",
				{
					pod::Vector2ui{ area.right, area.bottom },
				},
				{
					currentPosition,
					currentPosition - lastPosition,
					state
				}
			};
		#if UF_USE_USERDATA
			this->pushEvent(event.type, event);
		#endif
		#if UF_USE_JSON
			uf::Serializer json;
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
pod::Vector2ui UF_API_CALL spec::win32::Window::getResolution() {
	return { GetSystemMetrics(SM_CXSCREEN), GetSystemMetrics(SM_CYSCREEN) };
}
void UF_API_CALL spec::win32::Window::switchToFullscreen( bool borderless ) {
	if ( borderless ) {
		SetWindowLong(this->m_handle, GWL_STYLE, WS_POPUP );
	//	SetWindowLong(this->m_handle, GWL_EXSTYLE, 0);
	//	SetWindowPos(this->m_handle, HWND_TOP, 0, 0, GetSystemMetrics(SM_CXSCREEN), GetSystemMetrics(SM_CYSCREEN), SWP_FRAMECHANGED);
		ShowWindow(this->m_handle, SW_SHOW);
		return;
	}
	SetWindowLong(this->m_handle, GWL_STYLE, WS_POPUP | WS_CLIPCHILDREN | WS_CLIPSIBLINGS);
	SetWindowLong(this->m_handle, GWL_EXSTYLE, WS_EX_APPWINDOW);
	SetWindowPos(this->m_handle, HWND_TOP, 0, 0, GetSystemMetrics(SM_CXSCREEN), GetSystemMetrics(SM_CYSCREEN), SWP_FRAMECHANGED);
	ShowWindow(this->m_handle, SW_SHOW);
	fullscreenWindow = (void*) this;
}

bool UF_API_CALL spec::win32::Window::isKeyPressed(const std::string& key) {
	return GetAsyncKeyState( GetKeyCode( key ) ) & 0x8000;
}
std::string UF_API_CALL spec::win32::Window::getKey(WPARAM key, LPARAM flags) {
	return GetKeyName( key, flags );
}
#if defined(UF_USE_VULKAN) && UF_USE_VULKAN == 1
std::vector<std::string> UF_API_CALL spec::win32::Window::getExtensions( bool validationEnabled ) {
	std::vector<std::string> instanceExtensions = { VK_KHR_SURFACE_EXTENSION_NAME, VK_KHR_WIN32_SURFACE_EXTENSION_NAME  };
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