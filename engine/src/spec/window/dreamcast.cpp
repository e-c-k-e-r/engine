#include <uf/spec/window/window.h>
#include <uf/utils/io/iostream.h>
#include <uf/utils/string/utf.h>
#include <uf/utils/string/ext.h>

#if UF_ENV_DREAMCAST

/*

INIT_NONE        	-- don't do any auto init
INIT_IRQ     		-- Enable IRQs
INIT_THD_PREEMPT 	-- Enable pre-emptive threading
INIT_NET     		-- Enable networking (including sockets)
INIT_MALLOCSTATS 	-- Enable a call to malloc_stats() right before shutdown

*/

#include <kos.h>

extern uint8 romdisk[];
KOS_INIT_FLAGS(INIT_DEFAULT | INIT_MALLOCSTATS);
KOS_INIT_ROMDISK(romdisk);

namespace {
	struct {
		maple_device_t* device = NULL;
		kbd_state_t* state = NULL;
	} keyboard;
	struct {
		maple_device_t* device = NULL;
		mouse_state_t* state = NULL;
	} mouse;

	bool GetModifier( uint8_t modifier ) {
		if ( !::keyboard.state ) return false;
		return ::keyboard.state->cond.modifiers & modifier;
	}
	std::vector<uint8_t> GetKeys() {
		std::vector<uint8_t> keys;
		keys.reserve(6);
		for ( size_t i = 0; i < MAX_PRESSED_KEYS && ::keyboard.state; ++i ) keys.emplace_back(::keyboard.state->cond.keys[i]);
		return keys;
	}
	uint8_t GetKeyState( uint8_t key ) {
		if ( !::keyboard.state || key < MAX_KBD_KEYS ) return 0;
		return ::keyboard.state->matrix[key];
	}
	std::string GetKeyName( uint8_t key ) {
		switch ( key ) {
			case KBD_KEY_A: return "A";
			case KBD_KEY_B: return "B";
			case KBD_KEY_C: return "C";
			case KBD_KEY_D: return "D";
			case KBD_KEY_E: return "E";
			case KBD_KEY_F: return "F";
			case KBD_KEY_G: return "G";
			case KBD_KEY_H: return "H";
			case KBD_KEY_I: return "I";
			case KBD_KEY_J: return "J";
			case KBD_KEY_K: return "K";
			case KBD_KEY_L: return "L";
			case KBD_KEY_M: return "M";
			case KBD_KEY_N: return "N";
			case KBD_KEY_O: return "O";
			case KBD_KEY_P: return "P";
			case KBD_KEY_Q: return "Q";
			case KBD_KEY_R: return "R";
			case KBD_KEY_S: return "S";
			case KBD_KEY_T: return "T";
			case KBD_KEY_U: return "U";
			case KBD_KEY_V: return "V";
			case KBD_KEY_W: return "W";
			case KBD_KEY_X: return "X";
			case KBD_KEY_Y: return "Y";
			case KBD_KEY_Z: return "Z";
			case KBD_KEY_1: return "1";
			case KBD_KEY_2: return "2";
			case KBD_KEY_3: return "3";
			case KBD_KEY_4: return "4";
			case KBD_KEY_5: return "5";
			case KBD_KEY_6: return "6";
			case KBD_KEY_7: return "7";
			case KBD_KEY_8: return "8";
			case KBD_KEY_9: return "9";
			case KBD_KEY_0: return "0";
			case KBD_KEY_NONE: return "NONE";
			case KBD_KEY_ERROR: return "ERROR";
			case KBD_KEY_ERR2: return "ERR2";
			case KBD_KEY_ERR3: return "ERR3";
			case KBD_KEY_ENTER: return "ENTER";
			case KBD_KEY_ESCAPE: return "ESCAPE";
			case KBD_KEY_BACKSPACE: return "BACKSPACE";
			case KBD_KEY_TAB: return "TAB";
			case KBD_KEY_SPACE: return "SPACE";
			case KBD_KEY_MINUS: return "MINUS";
			case KBD_KEY_PLUS: return "PLUS";
			case KBD_KEY_LBRACKET: return "LBRACKET";
			case KBD_KEY_RBRACKET: return "RBRACKET";
			case KBD_KEY_BACKSLASH: return "BACKSLASH";
			case KBD_KEY_SEMICOLON: return "SEMICOLON";
			case KBD_KEY_QUOTE: return "QUOTE";
			case KBD_KEY_TILDE: return "TILDE";
			case KBD_KEY_COMMA: return "COMMA";
			case KBD_KEY_PERIOD: return "PERIOD";
			case KBD_KEY_SLASH: return "SLASH";
			case KBD_KEY_CAPSLOCK: return "CAPSLOCK";
			case KBD_KEY_F1: return "F1";
			case KBD_KEY_F2: return "F2";
			case KBD_KEY_F3: return "F3";
			case KBD_KEY_F4: return "F4";
			case KBD_KEY_F5: return "F5";
			case KBD_KEY_F6: return "F6";
			case KBD_KEY_F7: return "F7";
			case KBD_KEY_F8: return "F8";
			case KBD_KEY_F9: return "F9";
			case KBD_KEY_F10: return "F10";
			case KBD_KEY_F11: return "F11";
			case KBD_KEY_F12: return "F12";
			case KBD_KEY_PRINT: return "PRINT";
			case KBD_KEY_SCRLOCK: return "SCRLOCK";
			case KBD_KEY_PAUSE: return "PAUSE";
			case KBD_KEY_INSERT: return "INSERT";
			case KBD_KEY_HOME: return "HOME";
			case KBD_KEY_PGUP: return "PGUP";
			case KBD_KEY_DEL: return "DEL";
			case KBD_KEY_END: return "END";
			case KBD_KEY_PGDOWN: return "PGDOWN";
			case KBD_KEY_RIGHT: return "RIGHT";
			case KBD_KEY_LEFT: return "LEFT";
			case KBD_KEY_DOWN: return "DOWN";
			case KBD_KEY_UP: return "UP";
			case KBD_KEY_PAD_NUMLOCK: return "PAD_NUMLOCK";
			case KBD_KEY_PAD_DIVIDE: return "PAD_DIVIDE";
			case KBD_KEY_PAD_MULTIPLY: return "PAD_MULTIPLY";
			case KBD_KEY_PAD_MINUS: return "PAD_MINUS";
			case KBD_KEY_PAD_PLUS: return "PAD_PLUS";
			case KBD_KEY_PAD_ENTER: return "PAD_ENTER";
			case KBD_KEY_PAD_1: return "PAD_1";
			case KBD_KEY_PAD_2: return "PAD_2";
			case KBD_KEY_PAD_3: return "PAD_3";
			case KBD_KEY_PAD_4: return "PAD_4";
			case KBD_KEY_PAD_5: return "PAD_5";
			case KBD_KEY_PAD_6: return "PAD_6";
			case KBD_KEY_PAD_7: return "PAD_7";
			case KBD_KEY_PAD_8: return "PAD_8";
			case KBD_KEY_PAD_9: return "PAD_9";
			case KBD_KEY_PAD_0: return "PAD_0";
			case KBD_KEY_PAD_PERIOD: return "PAD_PERIOD";
			case KBD_KEY_S3: return "S3";
		}
		return "";
	}
	uint8_t GetKeyCode( const std::string& _key ) {
		std::string key = uf::string::uppercase(_key);

		if ( key == "A" ) return KBD_KEY_A;
		else if ( key == "B" ) return KBD_KEY_B;
		else if ( key == "C" ) return KBD_KEY_C;
		else if ( key == "D" ) return KBD_KEY_D;
		else if ( key == "E" ) return KBD_KEY_E;
		else if ( key == "F" ) return KBD_KEY_F;
		else if ( key == "G" ) return KBD_KEY_G;
		else if ( key == "H" ) return KBD_KEY_H;
		else if ( key == "I" ) return KBD_KEY_I;
		else if ( key == "J" ) return KBD_KEY_J;
		else if ( key == "K" ) return KBD_KEY_K;
		else if ( key == "L" ) return KBD_KEY_L;
		else if ( key == "M" ) return KBD_KEY_M;
		else if ( key == "N" ) return KBD_KEY_N;
		else if ( key == "O" ) return KBD_KEY_O;
		else if ( key == "P" ) return KBD_KEY_P;
		else if ( key == "Q" ) return KBD_KEY_Q;
		else if ( key == "R" ) return KBD_KEY_R;
		else if ( key == "S" ) return KBD_KEY_S;
		else if ( key == "T" ) return KBD_KEY_T;
		else if ( key == "U" ) return KBD_KEY_U;
		else if ( key == "V" ) return KBD_KEY_V;
		else if ( key == "W" ) return KBD_KEY_W;
		else if ( key == "X" ) return KBD_KEY_X;
		else if ( key == "Y" ) return KBD_KEY_Y;
		else if ( key == "Z" ) return KBD_KEY_Z;
		else if ( key == "1" ) return KBD_KEY_1;
		else if ( key == "2" ) return KBD_KEY_2;
		else if ( key == "3" ) return KBD_KEY_3;
		else if ( key == "4" ) return KBD_KEY_4;
		else if ( key == "5" ) return KBD_KEY_5;
		else if ( key == "6" ) return KBD_KEY_6;
		else if ( key == "7" ) return KBD_KEY_7;
		else if ( key == "8" ) return KBD_KEY_8;
		else if ( key == "9" ) return KBD_KEY_9;
		else if ( key == "0" ) return KBD_KEY_0;
		else if ( key == "NONE" ) return KBD_KEY_NONE;
		else if ( key == "ERROR" ) return KBD_KEY_ERROR;
		else if ( key == "ERR2" ) return KBD_KEY_ERR2;
		else if ( key == "ERR3" ) return KBD_KEY_ERR3;
		else if ( key == "ENTER" ) return KBD_KEY_ENTER;
		else if ( key == "ESCAPE" ) return KBD_KEY_ESCAPE;
		else if ( key == "BACKSPACE" ) return KBD_KEY_BACKSPACE;
		else if ( key == "TAB" ) return KBD_KEY_TAB;
		else if ( key == "SPACE" ) return KBD_KEY_SPACE;
		else if ( key == "MINUS" ) return KBD_KEY_MINUS;
		else if ( key == "PLUS" ) return KBD_KEY_PLUS;
		else if ( key == "LBRACKET" ) return KBD_KEY_LBRACKET;
		else if ( key == "RBRACKET" ) return KBD_KEY_RBRACKET;
		else if ( key == "BACKSLASH" ) return KBD_KEY_BACKSLASH;
		else if ( key == "SEMICOLON" ) return KBD_KEY_SEMICOLON;
		else if ( key == "QUOTE" ) return KBD_KEY_QUOTE;
		else if ( key == "TILDE" ) return KBD_KEY_TILDE;
		else if ( key == "COMMA" ) return KBD_KEY_COMMA;
		else if ( key == "PERIOD" ) return KBD_KEY_PERIOD;
		else if ( key == "SLASH" ) return KBD_KEY_SLASH;
		else if ( key == "CAPSLOCK" ) return KBD_KEY_CAPSLOCK;
		else if ( key == "F1" ) return KBD_KEY_F1;
		else if ( key == "F2" ) return KBD_KEY_F2;
		else if ( key == "F3" ) return KBD_KEY_F3;
		else if ( key == "F4" ) return KBD_KEY_F4;
		else if ( key == "F5" ) return KBD_KEY_F5;
		else if ( key == "F6" ) return KBD_KEY_F6;
		else if ( key == "F7" ) return KBD_KEY_F7;
		else if ( key == "F8" ) return KBD_KEY_F8;
		else if ( key == "F9" ) return KBD_KEY_F9;
		else if ( key == "F10" ) return KBD_KEY_F10;
		else if ( key == "F11" ) return KBD_KEY_F11;
		else if ( key == "F12" ) return KBD_KEY_F12;
		else if ( key == "PRINT" ) return KBD_KEY_PRINT;
		else if ( key == "SCRLOCK" ) return KBD_KEY_SCRLOCK;
		else if ( key == "PAUSE" ) return KBD_KEY_PAUSE;
		else if ( key == "INSERT" ) return KBD_KEY_INSERT;
		else if ( key == "HOME" ) return KBD_KEY_HOME;
		else if ( key == "PGUP" ) return KBD_KEY_PGUP;
		else if ( key == "DEL" ) return KBD_KEY_DEL;
		else if ( key == "END" ) return KBD_KEY_END;
		else if ( key == "PGDOWN" ) return KBD_KEY_PGDOWN;
		else if ( key == "RIGHT" ) return KBD_KEY_RIGHT;
		else if ( key == "LEFT" ) return KBD_KEY_LEFT;
		else if ( key == "DOWN" ) return KBD_KEY_DOWN;
		else if ( key == "UP" ) return KBD_KEY_UP;
		else if ( key == "PAD_NUMLOCK" ) return KBD_KEY_PAD_NUMLOCK;
		else if ( key == "PAD_DIVIDE" ) return KBD_KEY_PAD_DIVIDE;
		else if ( key == "PAD_MULTIPLY" ) return KBD_KEY_PAD_MULTIPLY;
		else if ( key == "PAD_MINUS" ) return KBD_KEY_PAD_MINUS;
		else if ( key == "PAD_PLUS" ) return KBD_KEY_PAD_PLUS;
		else if ( key == "PAD_ENTER" ) return KBD_KEY_PAD_ENTER;
		else if ( key == "PAD_1" ) return KBD_KEY_PAD_1;
		else if ( key == "PAD_2" ) return KBD_KEY_PAD_2;
		else if ( key == "PAD_3" ) return KBD_KEY_PAD_3;
		else if ( key == "PAD_4" ) return KBD_KEY_PAD_4;
		else if ( key == "PAD_5" ) return KBD_KEY_PAD_5;
		else if ( key == "PAD_6" ) return KBD_KEY_PAD_6;
		else if ( key == "PAD_7" ) return KBD_KEY_PAD_7;
		else if ( key == "PAD_8" ) return KBD_KEY_PAD_8;
		else if ( key == "PAD_9" ) return KBD_KEY_PAD_9;
		else if ( key == "PAD_0" ) return KBD_KEY_PAD_0;
		else if ( key == "PAD_PERIOD" ) return KBD_KEY_PAD_PERIOD;
		else if ( key == "S3" ) return KBD_KEY_S3;
		return 0;
	}
}

UF_API_CALL spec::dreamcast::Window::Window() : 
	m_handle 			(NULL),
//	m_callback			(0),
//	m_cursor			(NULL),
//	m_icon				(NULL),
	m_lastSize			({}),
	m_keyRepeatEnabled	(true),
	m_resizing			(false),
	m_mouseInside		(false),
	m_mouseGrabbed		(false),
	m_syncParse			(true),
	m_asyncParse		(false)
{
}
UF_API_CALL spec::dreamcast::Window::Window( spec::dreamcast::Window::handle_t handle ) :
	m_handle 			(handle),
//	m_callback			(0),
//	m_cursor			(NULL),
//	m_icon				(NULL),
	m_lastSize			({}),
	m_keyRepeatEnabled	(true),
	m_resizing			(false),
	m_mouseInside		(false),
	m_mouseGrabbed		(false),
	m_syncParse			(true),
	m_asyncParse		(false)
{

}
UF_API_CALL spec::dreamcast::Window::Window( const spec::dreamcast::Window::vector_t& size, const spec::dreamcast::Window::title_t& title ) : 
	m_handle 			(NULL),
//	m_callback			(0),
//	m_cursor			(NULL),
//	m_icon				(NULL),
	m_keyRepeatEnabled	(true),
	m_resizing			(false),
	m_mouseInside		(false),
	m_mouseGrabbed		(false),
	m_syncParse			(true),
	m_asyncParse		(false)
{
	this->create(size, title);
}
void UF_API_CALL spec::dreamcast::Window::create( const spec::dreamcast::Window::vector_t& _size, const spec::dreamcast::Window::title_t& title ) {
	::keyboard.device = maple_enum_type(1, MAPLE_FUNC_KEYBOARD);
}

spec::dreamcast::Window::~Window() {
}
void UF_API_CALL spec::dreamcast::Window::terminate() {
}

spec::dreamcast::Window::handle_t UF_API_CALL spec::dreamcast::Window::getHandle() const {
	return this->m_handle;
}
spec::dreamcast::Window::vector_t UF_API_CALL spec::dreamcast::Window::getPosition() const {
	return { 0, 0 };
}
spec::dreamcast::Window::vector_t UF_API_CALL spec::dreamcast::Window::getSize() const {
	return { 640, 480 };
}
size_t UF_API_CALL spec::dreamcast::Window::getRefreshRate() const {
	return 60;
}

void UF_API_CALL spec::dreamcast::Window::centerWindow() {
}
void UF_API_CALL spec::dreamcast::Window::setPosition( const spec::dreamcast::Window::vector_t& position ) {
}
void UF_API_CALL spec::dreamcast::Window::setMousePosition( const spec::dreamcast::Window::vector_t& position ) {
}
spec::dreamcast::Window::vector_t UF_API_CALL spec::dreamcast::Window::getMousePosition( ) {
	return { 320, 240 };
}
void UF_API_CALL spec::dreamcast::Window::setSize( const spec::dreamcast::Window::vector_t& size ) {
}

void UF_API_CALL spec::dreamcast::Window::setTitle( const spec::dreamcast::Window::title_t& title ) {
}
void UF_API_CALL spec::dreamcast::Window::setIcon( const spec::dreamcast::Window::vector_t& size, uint8_t* pixels ) {
}
void UF_API_CALL spec::dreamcast::Window::setVisible( bool visibility ) {
}
void UF_API_CALL spec::dreamcast::Window::setCursorVisible( bool visibility ) {
}
void UF_API_CALL spec::dreamcast::Window::setKeyRepeatEnabled( bool state ) {
	this->m_keyRepeatEnabled = state;
}

void UF_API_CALL spec::dreamcast::Window::requestFocus() {
}
bool UF_API_CALL spec::dreamcast::Window::hasFocus() const {
	return true;
}


#include <uf/utils/serialize/serializer.h>
void UF_API_CALL spec::dreamcast::Window::processEvents() {	
	if ( !::keyboard.device ) ::keyboard.device = maple_enum_type(0, MAPLE_FUNC_KEYBOARD);
	if ( ::keyboard.device ) ::keyboard.state = (kbd_state_t*) maple_dev_status(::keyboard.device);
	
	if ( !::mouse.device ) ::mouse.device = maple_enum_type(0, MAPLE_FUNC_MOUSE);
	if ( ::mouse.device ) ::mouse.state = (mouse_state_t*) maple_dev_status(::mouse.device);

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
					GetModifier(KBD_MOD_LALT) || GetModifier(KBD_MOD_RALT),
					GetModifier(KBD_MOD_LCTRL) || GetModifier(KBD_MOD_RCTRL),
					GetModifier(KBD_MOD_LSHIFT) || GetModifier(KBD_MOD_RSHIFT),
					GetModifier(KBD_MOD_S1) || GetModifier(KBD_MOD_S2),
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
bool UF_API_CALL spec::dreamcast::Window::pollEvents( bool block ) {
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
	/*
		try {
			uf::hooks.call( "window:Event", payload );
			uf::hooks.call( event.name, payload );
		} catch ( ... ) {
			// Let the hook handler handle the exceptions
		}
	*/
		this->m_events.pop();
	}
	return true;
}

void UF_API_CALL spec::dreamcast::Window::registerWindowClass() {
}
void UF_API_CALL spec::dreamcast::Window::processEvent(/*UINT message, WPARAM wParam, LPARAM lParam*/) {
}

void UF_API_CALL spec::dreamcast::Window::setTracking(bool state) {
}
void UF_API_CALL spec::dreamcast::Window::setMouseGrabbed(bool state) {
	this->m_mouseGrabbed = state;
	this->grabMouse(state);
}
void UF_API_CALL spec::dreamcast::Window::grabMouse(bool state) {
}
pod::Vector2ui UF_API_CALL spec::dreamcast::Window::getResolution() {
	return { 640, 480 };
}
void UF_API_CALL spec::dreamcast::Window::switchToFullscreen( bool borderless ) {
}

bool UF_API_CALL spec::dreamcast::Window::isKeyPressed(const std::string& key) {
	auto code = GetKeyCode(key);
	auto keys = GetKeys();
	for ( auto key : keys ) if ( key == code ) return true;
	return false;
}
#endif