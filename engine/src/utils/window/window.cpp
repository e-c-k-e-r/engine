#include <uf/config.h>
#if !defined(UF_USE_SFML) || (defined(UF_USE_SFML) && UF_USE_SFML == 0)

#include <uf/utils/window/window.h>
#include <uf/utils/io/iostream.h>
bool uf::Window::focused = false;

// 	C-tors
UF_API_CALL uf::Window::Window() :
	m_window(NULL),
	m_context(NULL)
{

}
UF_API_CALL uf::Window::Window( const spec::uni::Window::vector_t& size, const spec::uni::Window::title_t& title, const spec::Context::Settings& settings ) : Window() {
	this->create(size, title, settings);
}
void UF_API_CALL uf::Window::create( const spec::uni::Window::vector_t& size, const spec::uni::Window::title_t& title, const spec::Context::Settings& settings ) {
	this->m_window = new uf::Window::window_t;
	this->m_window->create( size, title );

//	this->m_context = (spec::Context*) spec::uni::Context::create( settings, *this->m_window );
}
// 	D-tors
uf::Window::~Window() {
	this->terminate();
}
void UF_API_CALL uf::Window::terminate() {
	if ( this->m_context ) {
		this->m_context->terminate();
		delete this->m_context;
		this->m_context = NULL;
	}
	if ( this->m_window ) {
		this->m_window->terminate();
		delete this->m_window;
		this->m_window = NULL;
	}
}
// 	Gets
spec::uni::Window::vector_t UF_API_CALL uf::Window::getPosition() const {
	static spec::uni::Window::vector_t null = {};
	return this->m_window ? this->m_window->getPosition() : null;
}
spec::uni::Window::vector_t UF_API_CALL uf::Window::getSize() const {
	static spec::uni::Window::vector_t null = {};
	return this->m_window ? this->m_window->getSize() : null;
}
// 	Attribute modifiers
void UF_API_CALL uf::Window::setPosition( const spec::uni::Window::vector_t& position ) {
	if ( this->m_window ) this->m_window->setPosition(position);
}
void UF_API_CALL uf::Window::centerWindow() {
	if ( this->m_window ) this->m_window->centerWindow();
}
void UF_API_CALL uf::Window::setMousePosition( const spec::uni::Window::vector_t& position ) {
	if ( this->m_window ) this->m_window->setMousePosition(position);
}
spec::uni::Window::vector_t UF_API_CALL uf::Window::getMousePosition() {
	if ( this->m_window ) return this->m_window->getMousePosition();
	return { 0, 0 };
}
void UF_API_CALL uf::Window::setSize( const spec::uni::Window::vector_t& size ) {
	if ( this->m_window ) this->m_window->setSize(size);
}
void UF_API_CALL uf::Window::setTitle( const spec::uni::Window::title_t& title ) {
	if ( this->m_window ) this->m_window->setTitle(title);
}
void UF_API_CALL uf::Window::setIcon( const spec::uni::Window::vector_t& size, uint8_t* pixels ) {
	if ( this->m_window ) this->m_window->setIcon(size, pixels);
}
void UF_API_CALL uf::Window::setVisible( bool visibility ) {
	if ( this->m_window ) this->m_window->setVisible(visibility);
}
void UF_API_CALL uf::Window::setCursorVisible( bool visibility ) {
	if ( this->m_window ) this->m_window->setCursorVisible(visibility);
}
void UF_API_CALL uf::Window::setKeyRepeatEnabled( bool state ) {
	if ( this->m_window ) this->m_window->setKeyRepeatEnabled(state);
}
void UF_API_CALL uf::Window::setMouseGrabbed( bool state ) {
	if ( this->m_window ) this->m_window->setMouseGrabbed(state);
}

void UF_API_CALL uf::Window::requestFocus() {
	if ( this->m_window ) this->m_window->requestFocus();
}
bool UF_API_CALL uf::Window::hasFocus() const {
	return uf::Window::focused = (this->m_window ? this->m_window->hasFocus() : false);
}
pod::Vector2ui UF_API_CALL uf::Window::getResolution() {
	return uf::Window::window_t::getResolution();
}
void UF_API_CALL uf::Window::switchToFullscreen( bool borderless ) {
	if ( this->m_window ) this->m_window->switchToFullscreen( borderless );
}
// 	Update
void UF_API_CALL uf::Window::processEvents() {
	if ( this->m_window ) this->m_window->processEvents();
}
bool UF_API_CALL uf::Window::pollEvents( bool block ) {
	return this->m_window ? this->m_window->pollEvents(block) : false;
}
bool UF_API_CALL uf::Window::isKeyPressed( const std::string& key ) {
	return uf::Window::focused && uf::Window::window_t::isKeyPressed(key);
}
bool UF_API_CALL uf::Window::setActive(bool active) {
	if (this->m_context) {
	//	if (this->m_context->setActive(active)) return true;
		uf::iostream << "[" << uf::IoStream::Color()("Red") << "ERROR" << "]" << "Failed to activate the window's context" << "\n";
		return false;
	}
	return false;
}
#if defined(UF_USE_VULKAN) && UF_USE_VULKAN == 1
std::vector<std::string> UF_API_CALL uf::Window::getExtensions( bool x ) {
	return this->m_window->getExtensions( x );
}
void UF_API_CALL uf::Window::createSurface( VkInstance instance, VkSurfaceKHR& surface ) {
	this->m_window->createSurface( instance, surface );
}
#endif


////////////////////////////////////////////////////////////

#include <thread>
#include <chrono>
#include <uf/utils/time/time.h>
void UF_API_CALL uf::Window::display() {
	// Display the backbuffer on screen
	if (this->m_context && this->setActive()) this->m_context->display();

	/* FPS */ {
		static double limit = 1.0 / 60;
		static uf::Timer<long long> timer(false);
		if ( !timer.running() ) timer.start();
		double delta = limit - timer.elapsed().asDouble();
		if ( delta > 0 ) {
			std::this_thread::sleep_for( std::chrono::milliseconds( (int) delta * 1000 ) );
			timer.reset();
		}
	}

/*
	// Limit the framerate if needed
	if (m_frameTimeLimit != Time::Zero)
	{
		sleep(m_frameTimeLimit - m_clock.getElapsedTime());
		m_clock.restart();
	}
*/
}

#endif