#include <uf/spec/window/window.h>
#include <uf/utils/io/iostream.h>
#include <uf/utils/string/utf.h>

#if UF_ENV_DREAMCAST

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
	return { 0, 0 };
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
	return { 0, 0 };
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
}
bool UF_API_CALL spec::dreamcast::Window::pollEvents( bool block ) {
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
	return { 0, 0 };
}
void UF_API_CALL spec::dreamcast::Window::switchToFullscreen( bool borderless ) {
}

bool UF_API_CALL spec::dreamcast::Window::isKeyPressed(const std::string& key) {
	return false;
}
std::string UF_API_CALL spec::dreamcast::Window::getKey(/*WPARAM key, LPARAM flags*/) {
	return "";
}
#endif