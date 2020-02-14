#include <uf/config.h>

#include <uf/spec/context/context.h>
#include <cmath>
#include <set>

#if UF_USE_OPENGL
namespace {
	// This per-thread variable holds the current context for each thread
	// sf::ThreadLocalPtr<spec::Context> currentContext(NULL);
	spec::uni::Context* currentContext = NULL;

	// The hidden, inactive context that will be shared with all other contexts
	spec::uni::Context* sharedContext = NULL;

	// Internal contexts
	// sf::ThreadLocalPtr<spec::Context> internalContext(NULL);
	spec::uni::Context* internalContext = NULL;
	std::set<spec::uni::Context*> internalContexts;
	// sf::Mutex internalContextsMutex;

	// Check if the internal context of the current thread is valid
	bool hasInternalContext() {
		// The internal context can be null...
		if (!internalContext) return false;
		// ... or non-null but deleted from the list of internal contexts
	//	sf::Lock lock(internalContextsMutex);
		return internalContexts.find(internalContext) != internalContexts.end();
	}

	// Retrieve the internal context for the current thread
	spec::uni::Context* getInternalContext() {
		if (!hasInternalContext()) {
			internalContext = spec::uni::Context::create();
		//	sf::Lock lock(internalContextsMutex);
			internalContexts.insert(internalContext);
		}

		return internalContext;
	}
}
//
bool UF_API_CALL spec::uni::Context::setActive(bool active) {
	if (active) {
		if (this != currentContext) {
			// Activate the context
			if (this->makeCurrent()) {
				// Set it as the new current context for this thread
				currentContext = this;
				return true;
			}
			return false;
		}
		return true; // This context is already the active one on this thread, don't do anything
	}
	
	if (this == currentContext) {
		// To deactivate the context, we actually activate another one so that we make
		// sure that there is always an active context for subsequent graphics operations
		return getInternalContext()->setActive(true);
	}
	return true; // This context is not the active one on this thread, don't do anything
}
//
void UF_API_CALL spec::uni::Context::initialize() {
	// Activate the context
	this->setActive(true);
}
void UF_API_CALL spec::uni::Context::globalInit() {
	// Create the shared context
	sharedContext = new spec::Context(NULL);
	sharedContext->initialize();

	// This call makes sure that:
	// - the shared context is inactive (it must never be)
	// - another valid context is activated in the current thread
	sharedContext->setActive(false);
}
void UF_API_CALL spec::uni::Context::globalCleanup() {
	// Destroy the shared context
	delete sharedContext;
	sharedContext = NULL;

	// Destroy the internal contexts
	// sf::Lock lock(internalContextsMutex);
	for (std::set<spec::uni::Context*>::iterator it = internalContexts.begin(); it != internalContexts.end(); ++it) delete *it;
	internalContexts.clear();
}
void UF_API_CALL spec::uni::Context::ensureContext() {
	// If there's no active context on the current thread, activate an internal one
	if (!currentContext) getInternalContext()->setActive(true);
}
spec::uni::Context* UF_API_CALL spec::uni::Context::create() {
	spec::uni::Context* context = new spec::Context(sharedContext);
	context->initialize();

	return context;
}
spec::uni::Context* UF_API_CALL spec::uni::Context::create(const spec::uni::Context::Settings& settings, const Context::window_t& owner) {
	// Make sure that there's an active context (context creation may need extensions, and thus a valid context)
	Context::ensureContext();
	// Create the context
	spec::uni::Context* context = new spec::Context(sharedContext, settings, owner);
	context->initialize();
	return context;
}
spec::uni::Context* UF_API_CALL spec::uni::Context::create(const spec::uni::Context::Settings& settings, unsigned int width, unsigned int height) {
	// Make sure that there's an active context (context creation may need extensions, and thus a valid context)
	Context::ensureContext();
	// Create the context
	spec::uni::Context* context = new spec::Context(sharedContext, settings, width, height);
	context->initialize();
	return context;
}
//

//
UF_API_CALL spec::uni::Context::Context() :
	m_window 		(NULL),
	m_ownsWindow 	(false)
{

}
UF_API_CALL spec::uni::Context::Context( Context::window_t::handle_t window, bool ownsWindow, const spec::uni::Context::Settings& settings ) :
	m_window 		(window),
	m_ownsWindow 	(ownsWindow),
	m_settings 		(settings)
{

}
spec::uni::Context::~Context() {
	if ( sharedContext ) this->setActive(false);
//	this->terminate();
}

int UF_API_CALL spec::uni::Context::evaluateFormat( const spec::uni::Context::Settings& settings, int colorBits, int depthBits, int stencilBits, int antialiasing ) {
	return std::abs(int(settings.bitsPerPixel	  - colorBits))   +
		   std::abs(int(settings.depthBits		 - depthBits))   +
		   std::abs(int(settings.stencilBits	   - stencilBits)) +
		   std::abs(int(settings.antialiasingLevel - antialiasing));
}
#else
#endif