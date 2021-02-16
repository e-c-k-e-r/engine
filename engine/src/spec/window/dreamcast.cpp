#include <uf/spec/window/window.h>
#include <uf/utils/io/iostream.h>
#include <uf/utils/string/utf.h>

#if UF_ENV_DREAMCAST

/*

INIT_NONE        	-- don't do any auto init
INIT_IRQ     		-- Enable IRQs
INIT_THD_PREEMPT 	-- Enable pre-emptive threading
INIT_NET     		-- Enable networking (including sockets)
INIT_MALLOCSTATS 	-- Enable a call to malloc_stats() right before shutdown

*/

extern uint8 romdisk[];
KOS_INIT_FLAGS(INIT_DEFAULT | INIT_MALLOCSTATS);
KOS_INIT_ROMDISK(romdisk);

namespace {
	struct {
		maple_device_t* device = NULL;
		cont_state_t* state = NULL;
	} controller, keyboard;
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
	//dbglog_set_level(DBG_WARNING);
	::controller.device = maple_enum_type(0, MAPLE_FUNC_CONTROLLER);
	::keyboard.device = maple_enum_type(0, MAPLE_FUNC_KEYBOARD);
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
	if ( !::controller.device ) ::controller.device = maple_enum_type(0, MAPLE_FUNC_CONTROLLER);
	if ( !::keyboard.device ) ::keyboard.device = maple_enum_type(0, MAPLE_FUNC_KEYBOARD);

	if ( ::controller.device ) ::controller.state = (cont_state_t*) maple_dev_status(::controller.device);
	if ( ::keyboard.device ) ::keyboard.state = (cont_state_t*) maple_dev_status(::keyboard.device);

	if ( ::controller.state ) {
		if ( ::controller.state->buttons & CONT_START ) {

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
		//	std::cout << event.name << " (string)\t" << payload << std::endl;
			uf::hooks.call( "window:Event", payload );
			uf::hooks.call( event.name, payload );
		} else if ( event.payload.is<uf::Serializer>() ) {
			uf::Serializer& payload = event.payload.as<uf::Serializer>();
		//	std::cout << event.name << " (serializer)\t" << payload << std::endl;
			uf::hooks.call( "window:Event", payload );
			uf::hooks.call( event.name, payload );
		} else if ( event.payload.is<ext::json::Value>() ) {
			ext::json::Value& payload = event.payload.as<ext::json::Value>();
		//	std::cout << event.name << " (json)\t" << payload << std::endl;
			uf::hooks.call( "window:Event", payload );
			uf::hooks.call( event.name, payload );
		} else {
		//	std::cout << event.name << "(???)" << std::endl;		
			uf::hooks.call( "window:Event", event.payload );
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
	if ( !::controller.state ) return false;

	if ( (key == "Left") && (::controller.state->buttons & CONT_DPAD_LEFT) ) return true;
	if ( (key == "Right") && (::controller.state->buttons & CONT_DPAD_RIGHT) ) return true;
	if ( (key == "Up") && (::controller.state->buttons & CONT_DPAD_UP) ) return true;
	if ( (key == "Down") && (::controller.state->buttons & CONT_DPAD_DOWN) ) return true;
	if ( (key == "Start") && (::controller.state->buttons & CONT_START) ) return true;
	if ( (key == "A") && (::controller.state->buttons & CONT_A) ) return true;

	return false;
}
std::string UF_API_CALL spec::dreamcast::Window::getKey(/*WPARAM key, LPARAM flags*/) {
	return "";
}
#endif