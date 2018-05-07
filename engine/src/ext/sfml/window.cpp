#include <uf/config.h>
#if defined(UF_USE_SFML)

#include <uf/ext/sfml/window.h>
#include <uf/utils/string/string.h>
#include <json/json.h>
#include <uf/spec/window/window.h>
#include <uf/utils/io/iostream.h>
#include <uf/utils/string/utf.h>
#include <uf/utils/serialize/serializer.h>
#include <unordered_map>

UF_API_CALL ext::sfml::Window::Window() {
}
UF_API_CALL ext::sfml::Window::Window( ext::sfml::Window::handle_t handle ) {
}
UF_API_CALL ext::sfml::Window::Window( const ext::sfml::Window::vector_t& size, const ext::sfml::Window::title_t& title, const spec::uni::Context::Settings& settings ) {
	this->create( size, title, settings );
}
void UF_API_CALL ext::sfml::Window::create( const ext::sfml::Window::vector_t& size, const ext::sfml::Window::title_t& title, const spec::uni::Context::Settings& settings ) {
	sf::ContextSettings sfmlSettings;
	sfmlSettings.depthBits 			= settings.depthBits;
	sfmlSettings.stencilBits 		= settings.stencilBits;
	sfmlSettings.antialiasingLevel 	= settings.antialiasingLevel;
	sfmlSettings.majorVersion 		= settings.majorVersion;
	sfmlSettings.minorVersion 		= settings.minorVersion;

	this->m_handle.create( sf::VideoMode( size.x, size.y ), std::string(title), sf::Style::Default, sfmlSettings );
}
// 	D-tors
ext::sfml::Window::~Window() {
	this->terminate();
}
void UF_API_CALL ext::sfml::Window::terminate() {
	this->m_handle.close();
}
// 	Gets
const ext::sfml::Window::handle_t& UF_API_CALL ext::sfml::Window::getHandle() const {
	return this->m_handle;
}
ext::sfml::Window::vector_t UF_API_CALL ext::sfml::Window::getPosition() const {
	auto sfmlVector = this->m_handle.getPosition();
	return Window::vector_t{ (int) sfmlVector.x, (int) sfmlVector.y };
}
ext::sfml::Window::vector_t UF_API_CALL ext::sfml::Window::getSize() const {
	auto sfmlVector = this->m_handle.getSize();
	return Window::vector_t{ (int) sfmlVector.x, (int) sfmlVector.y };
}
// 	Attribute modifiers
void UF_API_CALL ext::sfml::Window::setPosition( const Window::vector_t& position ) {
	this->m_handle.setPosition( sf::Vector2i(position.x, position.y) );
}
void UF_API_CALL ext::sfml::Window::centerWindow() {
}
void UF_API_CALL ext::sfml::Window::setMousePosition( const Window::vector_t& position ) {
	sf::Mouse::setPosition( sf::Vector2i(position.x, position.y), this->m_handle );
}
void UF_API_CALL ext::sfml::Window::setSize( const Window::vector_t& size ) {
	this->m_handle.setSize( sf::Vector2u(size.x, size.y) );
}
void UF_API_CALL ext::sfml::Window::setTitle( const Window::title_t& title ) {
	this->m_handle.setTitle( std::string(title) );
}
void UF_API_CALL ext::sfml::Window::setIcon( const Window::vector_t& size, uint8_t* pixels ) {
	this->m_handle.setIcon( size.x, size.y, pixels );
}
void UF_API_CALL ext::sfml::Window::setVisible( bool visibility ) {
	this->m_handle.setVisible(visibility);
}
void UF_API_CALL ext::sfml::Window::setCursorVisible( bool visibility ) {
	this->m_handle.setMouseCursorVisible(visibility);
}
void UF_API_CALL ext::sfml::Window::setKeyRepeatEnabled( bool state ) {
	this->m_handle.setKeyRepeatEnabled(state);
}
void UF_API_CALL ext::sfml::Window::setMouseGrabbed( bool state ) {
	this->m_handle.setMouseCursorGrabbed(state);
}

void UF_API_CALL ext::sfml::Window::requestFocus() {
	this->m_handle.setActive(true);
}
bool UF_API_CALL ext::sfml::Window::hasFocus() const {
	return this->m_handle.hasFocus();
}
void UF_API_CALL ext::sfml::Window::display() {
	this->m_handle.display();
}

// 	Update
namespace {
	std::string UF_API_CALL parseMouseButton( const sf::Mouse::Button button ) {
		switch ( button ) {	
			case sf::Mouse::Button::Left: 			return "Left";
			case sf::Mouse::Button::Right: 			return "Right";
			case sf::Mouse::Button::Middle: 		return "Middle";
			case sf::Mouse::Button::XButton1: 		return "XButton1";
			case sf::Mouse::Button::XButton2: 		return "XButton2";
			case sf::Mouse::Button::ButtonCount: 	return "";
		}
		return "";
	}
	std::string UF_API_CALL parseKeyCode( const sf::Keyboard::Key key ) {
		switch ( key ) {
			case sf::Keyboard::Key::Unknown: 	return "Unknown";
			case sf::Keyboard::Key::A: 			return "A";
			case sf::Keyboard::Key::B: 			return "B";
			case sf::Keyboard::Key::C: 			return "C";
			case sf::Keyboard::Key::D: 			return "D";
			case sf::Keyboard::Key::E: 			return "E";
			case sf::Keyboard::Key::F: 			return "F";
			case sf::Keyboard::Key::G: 			return "G";
			case sf::Keyboard::Key::H: 			return "H";
			case sf::Keyboard::Key::I: 			return "I";
			case sf::Keyboard::Key::J: 			return "J";
			case sf::Keyboard::Key::K: 			return "K";
			case sf::Keyboard::Key::L: 			return "L";
			case sf::Keyboard::Key::M: 			return "M";
			case sf::Keyboard::Key::N: 			return "N";
			case sf::Keyboard::Key::O: 			return "O";
			case sf::Keyboard::Key::P: 			return "P";
			case sf::Keyboard::Key::Q: 			return "Q";
			case sf::Keyboard::Key::R: 			return "R";
			case sf::Keyboard::Key::S: 			return "S";
			case sf::Keyboard::Key::T: 			return "T";
			case sf::Keyboard::Key::U: 			return "U";
			case sf::Keyboard::Key::V: 			return "V";
			case sf::Keyboard::Key::W: 			return "W";
			case sf::Keyboard::Key::X: 			return "X";
			case sf::Keyboard::Key::Y: 			return "Y";
			case sf::Keyboard::Key::Z: 			return "Z";
			case sf::Keyboard::Key::Num0: 		return "Num0";
			case sf::Keyboard::Key::Num1: 		return "Num1";
			case sf::Keyboard::Key::Num2: 		return "Num2";
			case sf::Keyboard::Key::Num3: 		return "Num3";
			case sf::Keyboard::Key::Num4: 		return "Num4";
			case sf::Keyboard::Key::Num5: 		return "Num5";
			case sf::Keyboard::Key::Num6: 		return "Num6";
			case sf::Keyboard::Key::Num7: 		return "Num7";
			case sf::Keyboard::Key::Num8: 		return "Num8";
			case sf::Keyboard::Key::Num9: 		return "Num9";
			case sf::Keyboard::Key::Escape: 	return "Escape";
			case sf::Keyboard::Key::LControl: 	return "LControl";
			case sf::Keyboard::Key::LShift: 	return "LShift";
			case sf::Keyboard::Key::LAlt: 		return "LAlt";
			case sf::Keyboard::Key::LSystem: 	return "LSystem";
			case sf::Keyboard::Key::RControl: 	return "RControl";
			case sf::Keyboard::Key::RShift: 	return "RShift";
			case sf::Keyboard::Key::RAlt: 		return "RAlt";
			case sf::Keyboard::Key::RSystem: 	return "RSystem";
			case sf::Keyboard::Key::Menu: 		return "Menu";
			case sf::Keyboard::Key::LBracket: 	return "LBracket";
			case sf::Keyboard::Key::RBracket: 	return "RBracket";
			case sf::Keyboard::Key::SemiColon: 	return "SemiColon";
			case sf::Keyboard::Key::Comma: 		return "Comma";
			case sf::Keyboard::Key::Period: 	return "Period";
			case sf::Keyboard::Key::Quote: 		return "Quote";
			case sf::Keyboard::Key::Slash: 		return "Slash";
			case sf::Keyboard::Key::BackSlash: 	return "BackSlash";
			case sf::Keyboard::Key::Tilde: 		return "Tilde";
			case sf::Keyboard::Key::Equal: 		return "Equal";
			case sf::Keyboard::Key::Dash: 		return "Dash";
			case sf::Keyboard::Key::Space: 		return "Space";
			case sf::Keyboard::Key::Return: 	return "Return";
			case sf::Keyboard::Key::BackSpace: 	return "BackSpace";
			case sf::Keyboard::Key::Tab: 		return "Tab";
			case sf::Keyboard::Key::PageUp: 	return "PageUp";
			case sf::Keyboard::Key::PageDown: 	return "PageDown";
			case sf::Keyboard::Key::End: 		return "End";
			case sf::Keyboard::Key::Home: 		return "Home";
			case sf::Keyboard::Key::Insert: 	return "Insert";
			case sf::Keyboard::Key::Delete: 	return "Delete";
			case sf::Keyboard::Key::Add: 		return "Add";
			case sf::Keyboard::Key::Subtract: 	return "Subtract";
			case sf::Keyboard::Key::Multiply: 	return "Multiply";
			case sf::Keyboard::Key::Divide: 	return "Divide";
			case sf::Keyboard::Key::Left: 		return "Left";
			case sf::Keyboard::Key::Right: 		return "Right";
			case sf::Keyboard::Key::Up: 		return "Up";
			case sf::Keyboard::Key::Down: 		return "Down";
			case sf::Keyboard::Key::Numpad0: 	return "Numpad0";
			case sf::Keyboard::Key::Numpad1: 	return "Numpad1";
			case sf::Keyboard::Key::Numpad2: 	return "Numpad2";
			case sf::Keyboard::Key::Numpad3: 	return "Numpad3";
			case sf::Keyboard::Key::Numpad4: 	return "Numpad4";
			case sf::Keyboard::Key::Numpad5: 	return "Numpad5";
			case sf::Keyboard::Key::Numpad6: 	return "Numpad6";
			case sf::Keyboard::Key::Numpad7: 	return "Numpad7";
			case sf::Keyboard::Key::Numpad8: 	return "Numpad8";
			case sf::Keyboard::Key::Numpad9: 	return "Numpad9";
			case sf::Keyboard::Key::F1: 		return "F1";
			case sf::Keyboard::Key::F2: 		return "F2";
			case sf::Keyboard::Key::F3: 		return "F3";
			case sf::Keyboard::Key::F4: 		return "F4";
			case sf::Keyboard::Key::F5: 		return "F5";
			case sf::Keyboard::Key::F6: 		return "F6";
			case sf::Keyboard::Key::F7: 		return "F7";
			case sf::Keyboard::Key::F8: 		return "F8";
			case sf::Keyboard::Key::F9: 		return "F9";
			case sf::Keyboard::Key::F10: 		return "F10";
			case sf::Keyboard::Key::F11: 		return "F11";
			case sf::Keyboard::Key::F12: 		return "F12";
			case sf::Keyboard::Key::F13: 		return "F13";
			case sf::Keyboard::Key::F14: 		return "F14";
			case sf::Keyboard::Key::F15: 		return "F15";
			case sf::Keyboard::Key::Pause: 		return "Pause";
			case sf::Keyboard::Key::KeyCount: 	return "";
		}
		return "";
	}
	bool UF_API_CALL isKeyDown( const std::string& str ) {
		#define prefix sf::Keyboard
		auto keyPressed = [](const sf::Keyboard::Key& key)->bool{
			return sf::Keyboard::isKeyPressed(key);
		};
		if ( str == "F1" )			return keyPressed(prefix::F1 );
		if ( str == "F2" )			return keyPressed(prefix::F2 );
		if ( str == "F3" )			return keyPressed(prefix::F3 );
		if ( str == "F4" )			return keyPressed(prefix::F4 );
		if ( str == "F5" )			return keyPressed(prefix::F5 );
		if ( str == "F6" )			return keyPressed(prefix::F6 );
		if ( str == "F7" )			return keyPressed(prefix::F7 );
		if ( str == "F8" )			return keyPressed(prefix::F8 );
		if ( str == "F9" )			return keyPressed(prefix::F9 );
		if ( str == "F10" ) 		return keyPressed(prefix::F10 );
		if ( str == "F11" ) 		return keyPressed(prefix::F11 );
		if ( str == "F12" ) 		return keyPressed(prefix::F12 );

		if ( str == "Q" ) 			return keyPressed(prefix::Q );
		if ( str == "W" ) 			return keyPressed(prefix::W );
		if ( str == "E" ) 			return keyPressed(prefix::E );
		if ( str == "R" ) 			return keyPressed(prefix::R );
		if ( str == "T" ) 			return keyPressed(prefix::T );
		if ( str == "Y" ) 			return keyPressed(prefix::Y );
		if ( str == "U" ) 			return keyPressed(prefix::U );
		if ( str == "I" ) 			return keyPressed(prefix::I );
		if ( str == "O" ) 			return keyPressed(prefix::O );
		if ( str == "P" ) 			return keyPressed(prefix::P );

		if ( str == "A" ) 			return keyPressed(prefix::A );
		if ( str == "S" ) 			return keyPressed(prefix::S );
		if ( str == "D" ) 			return keyPressed(prefix::D );
		if ( str == "F" ) 			return keyPressed(prefix::F );
		if ( str == "G" ) 			return keyPressed(prefix::G );
		if ( str == "H" ) 			return keyPressed(prefix::H );
		if ( str == "J" ) 			return keyPressed(prefix::J );
		if ( str == "K" ) 			return keyPressed(prefix::K );
		if ( str == "L" ) 			return keyPressed(prefix::L );

		if ( str == "Z" ) 			return keyPressed(prefix::Z );
		if ( str == "X" ) 			return keyPressed(prefix::X );
		if ( str == "C" ) 			return keyPressed(prefix::C );
		if ( str == "V" ) 			return keyPressed(prefix::V );
		if ( str == "B" ) 			return keyPressed(prefix::B );
		if ( str == "N" ) 			return keyPressed(prefix::N );
		if ( str == "M" ) 			return keyPressed(prefix::M );

		if ( str == "Numpad0" ) 	return keyPressed(prefix::Numpad0);
		if ( str == "Numpad1" ) 	return keyPressed(prefix::Numpad1);
		if ( str == "Numpad2" ) 	return keyPressed(prefix::Numpad2);
		if ( str == "Numpad3" ) 	return keyPressed(prefix::Numpad3);
		if ( str == "Numpad4" ) 	return keyPressed(prefix::Numpad4);
		if ( str == "Numpad5" ) 	return keyPressed(prefix::Numpad5);
		if ( str == "Numpad6" ) 	return keyPressed(prefix::Numpad6);
		if ( str == "Numpad7" ) 	return keyPressed(prefix::Numpad7);
		if ( str == "Numpad8" ) 	return keyPressed(prefix::Numpad8);
		if ( str == "Numpad9" ) 	return keyPressed(prefix::Numpad9);

		if ( str == "Num0" ) 		return keyPressed(prefix::Num0);
		if ( str == "Num1" ) 		return keyPressed(prefix::Num1);
		if ( str == "Num2" ) 		return keyPressed(prefix::Num2);
		if ( str == "Num3" ) 		return keyPressed(prefix::Num3);
		if ( str == "Num4" ) 		return keyPressed(prefix::Num4);
		if ( str == "Num5" ) 		return keyPressed(prefix::Num5);
		if ( str == "Num6" ) 		return keyPressed(prefix::Num6);
		if ( str == "Num7" ) 		return keyPressed(prefix::Num7);
		if ( str == "Num8" ) 		return keyPressed(prefix::Num8);
		if ( str == "Num9" ) 		return keyPressed(prefix::Num9);

		if ( str == "Escape" ) 		return keyPressed(prefix::Escape);
		if ( str == "LControl" ) 	return keyPressed(prefix::LControl);
		if ( str == "LShift" ) 		return keyPressed(prefix::LShift);
		if ( str == "LAlt" ) 		return keyPressed(prefix::LAlt);
		if ( str == "LSystem" ) 	return keyPressed(prefix::LSystem);
		if ( str == "RControl" ) 	return keyPressed(prefix::RControl);
		if ( str == "RShift" ) 		return keyPressed(prefix::RShift);
		if ( str == "RAlt" ) 		return keyPressed(prefix::RAlt);
		if ( str == "RSystem" ) 	return keyPressed(prefix::RSystem);
		if ( str == "Menu" ) 		return keyPressed(prefix::Menu);
		
		if ( str == "LBracket" ) 	return keyPressed(prefix::LBracket);
		if ( str == "RBracket" ) 	return keyPressed(prefix::RBracket);
		if ( str == "SemiColon" ) 	return keyPressed(prefix::SemiColon);
		if ( str == "Comma" ) 		return keyPressed(prefix::Comma);
		if ( str == "Period" ) 		return keyPressed(prefix::Period);
		if ( str == "Quote" ) 		return keyPressed(prefix::Quote);
		if ( str == "Slash" ) 		return keyPressed(prefix::Slash);
		if ( str == "BackSlash" ) 	return keyPressed(prefix::BackSlash);
		if ( str == "Tilde" ) 		return keyPressed(prefix::Tilde);
		if ( str == "Equal" ) 		return keyPressed(prefix::Equal);
		if ( str == "Dash" ) 		return keyPressed(prefix::Dash);

		if ( str == "Space" ) 		return keyPressed(prefix::Space);
		if ( str == "Return" ) 		return keyPressed(prefix::Return);
		if ( str == "BackSpace" ) 	return keyPressed(prefix::BackSpace);
		if ( str == "Tab" ) 		return keyPressed(prefix::Tab);

		if ( str == "PageUp" ) 		return keyPressed(prefix::PageUp);
		if ( str == "PageDown" ) 	return keyPressed(prefix::PageDown);
		if ( str == "End" ) 		return keyPressed(prefix::End);
		if ( str == "Home" ) 		return keyPressed(prefix::Home);
		if ( str == "Insert" ) 		return keyPressed(prefix::Insert);
		if ( str == "Delete" ) 		return keyPressed(prefix::Delete);

		if ( str == "Add" ) 		return keyPressed(prefix::Add);
		if ( str == "Subtract" ) 	return keyPressed(prefix::Subtract);
		if ( str == "Multiply" ) 	return keyPressed(prefix::Multiply);
		if ( str == "Divide" ) 		return keyPressed(prefix::Divide);

		if ( str == "Left" ) 		return keyPressed(prefix::Left);
		if ( str == "Right" ) 		return keyPressed(prefix::Right);
		if ( str == "Up" ) 			return keyPressed(prefix::Up);
		if ( str == "Down" ) 		return keyPressed(prefix::Down);
		#undef prefix

		return false;
	}
}
bool UF_API_CALL ext::sfml::Window::isKeyPressed(const std::string& key) {
	return isKeyDown(key);
}
void UF_API_CALL ext::sfml::Window::processEvents() {
	uf::Serializer json;
	std::string hook = "window:Unknown";
	std::string serialized = "";
	std::stringstream serializer;
//	bool labelAsDelta = true;

	/* Parse better key inputs */ if ( this->m_asyncParse ) {
		static std::vector<std::string> previous;
		std::vector<std::string> keys;
		std::unordered_map<std::string,bool> map;

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
		Event e; {
			e.type 			= "window:Key.Changed",
			e.invoker 		= "window",
			e.key.code 		= "",
			e.key.raw 		= 0,
			e.key.async 	= true;
			e.key.modifier 	= {	
				.alt		= false,
				.ctrl 		= false,
				.shift		= false,
				.sys  		= false,
			};
			e.key.state 	= -1;

			json["type"] 							= e.type;
			json["invoker"] 						= e.invoker;
			json["key"]["state"] 					= (e.key.state == -1) ? "Down" : "Up";
			json["key"]["async"] 					= e.key.async;
			json["key"]["modifier"]["alt"]			= e.key.modifier.alt;
			json["key"]["modifier"]["control"] 		= e.key.modifier.ctrl;
			json["key"]["modifier"]["shift"]		= e.key.modifier.shift;
			json["key"]["modifier"]["system"]  		= e.key.modifier.sys;
		}
		if ( isKeyDown("F1") ) 			keys.push_back("F1" );
		if ( isKeyDown("F2") ) 			keys.push_back("F2" );
		if ( isKeyDown("F3") ) 			keys.push_back("F3" );
		if ( isKeyDown("F4") ) 			keys.push_back("F4" );
		if ( isKeyDown("F5") ) 			keys.push_back("F5" );
		if ( isKeyDown("F6") ) 			keys.push_back("F6" );
		if ( isKeyDown("F7") ) 			keys.push_back("F7" );
		if ( isKeyDown("F8") ) 			keys.push_back("F8" );
		if ( isKeyDown("F9") ) 			keys.push_back("F9" );
		if ( isKeyDown("F10") ) 		keys.push_back("F10" );
		if ( isKeyDown("F11") ) 		keys.push_back("F11" );
		if ( isKeyDown("F12") ) 		keys.push_back("F12" );

		if ( isKeyDown("Q") ) 			keys.push_back("Q" );
		if ( isKeyDown("W") ) 			keys.push_back("W" );
		if ( isKeyDown("E") ) 			keys.push_back("E" );
		if ( isKeyDown("R") ) 			keys.push_back("R" );
		if ( isKeyDown("T") ) 			keys.push_back("T" );
		if ( isKeyDown("Y") ) 			keys.push_back("Y" );
		if ( isKeyDown("U") ) 			keys.push_back("U" );
		if ( isKeyDown("I") ) 			keys.push_back("I" );
		if ( isKeyDown("O") ) 			keys.push_back("O" );
		if ( isKeyDown("P") ) 			keys.push_back("P" );

		if ( isKeyDown("A") ) 			keys.push_back("A" );
		if ( isKeyDown("S") ) 			keys.push_back("S" );
		if ( isKeyDown("D") ) 			keys.push_back("D" );
		if ( isKeyDown("F") ) 			keys.push_back("F" );
		if ( isKeyDown("G") ) 			keys.push_back("G" );
		if ( isKeyDown("H") ) 			keys.push_back("H" );
		if ( isKeyDown("J") ) 			keys.push_back("J" );
		if ( isKeyDown("K") ) 			keys.push_back("K" );
		if ( isKeyDown("L") ) 			keys.push_back("L" );

		if ( isKeyDown("Z") ) 			keys.push_back("Z" );
		if ( isKeyDown("X") ) 			keys.push_back("X" );
		if ( isKeyDown("C") ) 			keys.push_back("C" );
		if ( isKeyDown("V") ) 			keys.push_back("V" );
		if ( isKeyDown("B") ) 			keys.push_back("B" );
		if ( isKeyDown("N") ) 			keys.push_back("N" );
		if ( isKeyDown("M") ) 			keys.push_back("M" );

		if ( isKeyDown("Numpad0") ) 	keys.push_back("Numpad0");
		if ( isKeyDown("Numpad1") ) 	keys.push_back("Numpad1");
		if ( isKeyDown("Numpad2") ) 	keys.push_back("Numpad2");
		if ( isKeyDown("Numpad3") ) 	keys.push_back("Numpad3");
		if ( isKeyDown("Numpad4") ) 	keys.push_back("Numpad4");
		if ( isKeyDown("Numpad5") ) 	keys.push_back("Numpad5");
		if ( isKeyDown("Numpad6") ) 	keys.push_back("Numpad6");
		if ( isKeyDown("Numpad7") ) 	keys.push_back("Numpad7");
		if ( isKeyDown("Numpad8") ) 	keys.push_back("Numpad8");
		if ( isKeyDown("Numpad9") ) 	keys.push_back("Numpad9");

		if ( isKeyDown("Num0") ) 		keys.push_back("Num0");
		if ( isKeyDown("Num1") ) 		keys.push_back("Num1");
		if ( isKeyDown("Num2") ) 		keys.push_back("Num2");
		if ( isKeyDown("Num3") ) 		keys.push_back("Num3");
		if ( isKeyDown("Num4") ) 		keys.push_back("Num4");
		if ( isKeyDown("Num5") ) 		keys.push_back("Num5");
		if ( isKeyDown("Num6") ) 		keys.push_back("Num6");
		if ( isKeyDown("Num7") ) 		keys.push_back("Num7");
		if ( isKeyDown("Num8") ) 		keys.push_back("Num8");
		if ( isKeyDown("Num9") ) 		keys.push_back("Num9");

		if ( isKeyDown("Escape") ) 		keys.push_back("Escape");
		if ( isKeyDown("LControl") ) 	keys.push_back("LControl"), e.key.modifier.ctrl = true;
		if ( isKeyDown("LShift") ) 		keys.push_back("LShift"), e.key.modifier.shift = true;
		if ( isKeyDown("LAlt") ) 		keys.push_back("LAlt"), e.key.modifier.alt = true;
		if ( isKeyDown("LSystem") ) 	keys.push_back("LSystem"), e.key.modifier.sys = true;
		if ( isKeyDown("RControl") ) 	keys.push_back("RControl"), e.key.modifier.ctrl = true;
		if ( isKeyDown("RShift") ) 		keys.push_back("RShift"), e.key.modifier.shift = true;
		if ( isKeyDown("RAlt") ) 		keys.push_back("RAlt"), e.key.modifier.alt = true;
		if ( isKeyDown("RSystem") ) 	keys.push_back("RSystem"), e.key.modifier.sys = true;
		if ( isKeyDown("Menu") ) 		keys.push_back("Menu");

		if ( isKeyDown("LBracket") ) 	keys.push_back("LBracket");
		if ( isKeyDown("RBracket") ) 	keys.push_back("RBracket");
		if ( isKeyDown("SemiColon") ) 	keys.push_back("SemiColon");
		if ( isKeyDown("Comma") ) 		keys.push_back("Comma");
		if ( isKeyDown("Period") ) 		keys.push_back("Period");
		if ( isKeyDown("Quote") ) 		keys.push_back("Quote");
		if ( isKeyDown("Slash") ) 		keys.push_back("Slash");
		if ( isKeyDown("BackSlash") ) 	keys.push_back("BackSlash");
		if ( isKeyDown("Tilde") ) 		keys.push_back("Tilde");
		if ( isKeyDown("Equal") ) 		keys.push_back("Equal");
		if ( isKeyDown("Dash") ) 		keys.push_back("Dash");

		if ( isKeyDown("Space") ) 		keys.push_back("Space");
		if ( isKeyDown("Return") ) 		keys.push_back("Return");
		if ( isKeyDown("BackSpace") ) 	keys.push_back("BackSpace");
		if ( isKeyDown("Tab") ) 		keys.push_back("Tab");

		if ( isKeyDown("PageUp") ) 		keys.push_back("PageUp");
		if ( isKeyDown("PageDown") ) 	keys.push_back("PageDown");
		if ( isKeyDown("End") ) 		keys.push_back("End");
		if ( isKeyDown("Home") ) 		keys.push_back("Home");
		if ( isKeyDown("Insert") ) 		keys.push_back("Insert");
		if ( isKeyDown("Delete") ) 		keys.push_back("Delete");

		if ( isKeyDown("Add") ) 		keys.push_back("Add");
		if ( isKeyDown("Subtract") ) 	keys.push_back("Subtract");
		if ( isKeyDown("Multiply") ) 	keys.push_back("Multiply");
		if ( isKeyDown("Divide") ) 		keys.push_back("Divide");

		if ( isKeyDown("Left") ) 		keys.push_back("Left");
		if ( isKeyDown("Right") ) 		keys.push_back("Right");
		if ( isKeyDown("Up") ) 			keys.push_back("Up");
		if ( isKeyDown("Down") ) 		keys.push_back("Down");

		for ( auto& key : keys ) {
			e.key.code = key;
			json["key"]["code"] = e.key.code;
			this->pushEvent(e.type, uf::userdata::create(e));
			this->pushEvent(e.type, json);
		}

		previous = keys;
	}

	sf::Event message;
	while ( this->m_handle.pollEvent(message) ) {
		switch ( message.type ) {
			case sf::Event::Closed: {
				struct Event {
					std::string type = "unknown";
					std::string invoker = "???";
				};
				Event e; {
					e.type = "window:Closed";
					e.invoker = "os";
				};
				this->pushEvent(e.type, uf::userdata::create(e)); {
					json["type"] = e.type;
					json["invoker"] = e.invoker;
					this->pushEvent(e.type, json);
				}
			} break;
			case sf::Event::Resized: {
				struct Event {
					std::string type = "unknown";
					std::string invoker = "???";

					struct {
						pod::Vector2i size;
					} window;
				};
				Event e; {
					e.type = "window:Resized";
					e.invoker = "os";
					e.window.size.x = message.size.width;
					e.window.size.y = message.size.height;
				};
				this->pushEvent(e.type, uf::userdata::create(e)); {
					json["type"] = e.type;
					json["invoker"] = e.invoker;
					json["window"]["size"]["x"] = e.window.size.x;
					json["window"]["size"]["y"] = e.window.size.y;
					this->pushEvent(e.type, json);
				}
			} break;
			case sf::Event::GainedFocus:
			case sf::Event::LostFocus: {
				struct Event {
					std::string type = "unknown";
					std::string invoker = "???";

					struct {
						int state = 0;
					} window;
				};
				Event e; {
					e.type = "window:Focus.Changed";
					e.invoker = "os";

					if ( message.type == sf::Event::GainedFocus) 	e.window.state 	=  1;
					if ( message.type == sf::Event::LostFocus) 		e.window.state 	= -1;
				};
				this->pushEvent(e.type, uf::userdata::create(e)); {
					json["type"] = e.type;
					json["invoker"] = e.invoker;
				 	if ( message.type == sf::Event::GainedFocus) 	json["window"]["state"] = "Gained";
					if ( message.type == sf::Event::LostFocus) 		json["window"]["state"] =  "Lost";
					this->pushEvent(e.type, json);
				}
			} break;
			case sf::Event::TextEntered: if ( this->m_syncParse ) {
				struct Event {
					std::string type = "unknown";
					std::string invoker = "???";

					struct {
						uint32_t utf32;
						std::string unicode;
					} text;
				};
				Event e; {
					e.type = "window:Text.Entered";
					e.invoker = "os";

					e.text.utf32 = (uint32_t) message.text.unicode;
					e.text.unicode = (std::string) uf::String((uint32_t) message.text.unicode);
				};
				this->pushEvent(e.type, uf::userdata::create(e)); {
					json["type"] = e.type;
					json["invoker"] = e.invoker;

					json["text"]["uint32_t"] = e.text.utf32;
					json["text"]["unicode"] = e.text.unicode;
					this->pushEvent(e.type, json);
				}
			} break;
			case sf::Event::KeyPressed: 
			case sf::Event::KeyReleased: if ( this->m_syncParse ) {
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
				Event e; {
					e.type 			= "window:Key.Changed",
					e.invoker 		= "window",
					e.key.code 		= parseKeyCode(message.key.code),
					e.key.raw 		= message.key.code,
					e.key.async 	= false;
					e.key.modifier 	= {	
						.alt		= message.key.alt,
						.ctrl 		= message.key.control,
						.shift		= message.key.shift,
						.sys  		= message.key.system,
					};
					if ( message.type == sf::Event::KeyPressed ) e.key.state 	= -1;
					if ( message.type == sf::Event::KeyReleased ) e.key.state 	=  1;
				}
				this->pushEvent(e.type, uf::userdata::create(e)); {
					json["type"] 							= e.type;
					json["invoker"] 						= e.invoker;
					json["key"]["state"] 					= (e.key.state == -1) ? "Down" : "Up";
					json["key"]["async"] 					= e.key.async;
					json["key"]["modifier"]["alt"]			= e.key.modifier.alt;
					json["key"]["modifier"]["control"] 		= e.key.modifier.ctrl;
					json["key"]["modifier"]["shift"]		= e.key.modifier.shift;
					json["key"]["modifier"]["system"]  		= e.key.modifier.sys;
					
					json["key"]["code"] 					= e.key.code;
					json["key"]["raw"] 						= e.key.raw;
					this->pushEvent(e.type, json);
				}
			} break;
			case sf::Event::MouseWheelMoved: {
				struct Event {
					std::string type = "unknown";
					std::string invoker = "???";

					struct {
						pod::Vector2ui 	position = {};
						int16_t 		delta = 0;
					} mouse;
				};
				Event e; {
					e.type = "window:Mouse.Wheel";
					e.invoker = "os";

					e.mouse.position.x = (uint16_t) message.mouseWheel.x;
					e.mouse.position.y = (uint16_t) message.mouseWheel.y;
					e.mouse.delta = message.mouseWheel.delta;
				};
				this->pushEvent(e.type, uf::userdata::create(e)); {
					json["type"] = e.type;
					json["invoker"] = e.invoker;

					json["mouse"]["position"]["x"]		= e.mouse.position.x;
					json["mouse"]["position"]["y"]		= e.mouse.position.y;
					json["mouse"]["delta"] 				= e.mouse.delta;

					this->pushEvent(e.type, json);
				}
			} break;
			case sf::Event::MouseWheelScrolled: {
			} break;
			case sf::Event::MouseButtonPressed:
			case sf::Event::MouseButtonReleased: if( this->m_syncParse ) {
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
				Event e; {
					e.type = "window:Mouse.Click";
					e.invoker = "os";

					e.mouse.position.x 	= message.mouseButton.x;
					e.mouse.position.y 	= message.mouseButton.y;
					e.mouse.delta 		= uf::vector::subtract(e.mouse.position, lastPosition);
					e.mouse.button 		= parseMouseButton( message.mouseButton.button );
					if ( message.type == sf::Event::MouseButtonPressed ) e.mouse.state = -1;
					if ( message.type == sf::Event::MouseButtonReleased ) e.mouse.state = 1;
				};
				lastPosition = e.mouse.position;
				this->pushEvent(e.type, uf::userdata::create(e)); {
					json["type"] = e.type;
					json["invoker"] = e.invoker;

					json["mouse"]["position"]["x"]		= e.mouse.position.x;
					json["mouse"]["position"]["y"]		= e.mouse.position.y;
					json["mouse"]["delta"]["x"] 		= e.mouse.delta.x;
					json["mouse"]["delta"]["y"] 		= e.mouse.delta.y;
					json["mouse"]["button"] 			= e.mouse.button;
					switch (e.mouse.state) {
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

					this->pushEvent(e.type, json);
				}
			} break;
			case sf::Event::MouseMoved: {
				static pod::Vector2i lastPosition = {};
				struct Event {
					std::string type = "unknown";
					std::string invoker = "???";

					struct {
						int state = 0;
						pod::Vector2i position;
						pod::Vector2i delta;
					} mouse;
				};
				Event e; {
					e.type = "window:Mouse.Moved";
					e.invoker = "os";
				};
				this->pushEvent(e.type, uf::userdata::create(e)); {
					json["type"] = e.type;
					json["invoker"] = e.invoker;

					json["mouse"]["position"]["x"] = e.mouse.position.x;
					json["mouse"]["position"]["y"] = e.mouse.position.y;
					json["mouse"]["delta"]["x"] = e.mouse.delta.x;
					json["mouse"]["delta"]["y"] = e.mouse.delta.y;
					switch (e.mouse.state) {
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
					this->pushEvent(e.type, json);
				}
				lastPosition = e.mouse.position;
			} break;
			case sf::Event::MouseEntered: {
				struct Event {
					std::string type = "unknown";
					std::string invoker = "???";

					struct {
						int state = 0;
					} mouse;
				};
				Event e; {
					e.type = "window:Mouse.Moved";
					e.invoker = "os";

					e.mouse.state = 1;
				};
				this->pushEvent(e.type, uf::userdata::create(e)); {
					json["type"] = e.type;
					json["invoker"] = e.invoker;

					switch (e.mouse.state) {
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
					this->pushEvent(e.type, json);
				}
			} break;
			case sf::Event::MouseLeft: {
				struct Event {
					std::string type = "unknown";
					std::string invoker = "???";

					struct {
						int state = 0;
					} mouse;
				};
				Event e; {
					e.type = "window:Mouse.Moved";
					e.invoker = "os";

					e.mouse.state = -1;
				};
				this->pushEvent(e.type, uf::userdata::create(e)); {
					json["type"] = e.type;
					json["invoker"] = e.invoker;

					switch (e.mouse.state) {
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
					this->pushEvent(e.type, json);
				}
			} break;
			case sf::Event::JoystickButtonPressed: {

			} break;
			case sf::Event::JoystickButtonReleased: {

			} break;
			case sf::Event::JoystickMoved: {

			} break;
			case sf::Event::JoystickConnected: {

			} break;
			case sf::Event::JoystickDisconnected: {

			} break;
			case sf::Event::TouchBegan: {

			} break;
			case sf::Event::TouchMoved: {

			} break;
			case sf::Event::TouchEnded: {

			} break;
			case sf::Event::SensorChanged: {

			} break;
			case sf::Event::Count: {

			} break;
		}
	}
}

#endif