#if UF_USE_OPENGL

#include <uf/ext/opengl/opengl.h>
#include <uf/ext/opengl/device.h>
#include <uf/ext/opengl/initializers.h>
#include <uf/utils/window/window.h>
#include <uf/utils/string/ext.h>
#include <uf/utils/memory/pool.h>
#include <uf/ext/openvr/openvr.h>

#include <set>
#include <map>

#include <uf/utils/serialize/serializer.h>

#if UF_USE_OPENGL_FIXED_FUNCTION
namespace {
	namespace buffers {
		struct Pair {
			bool aliased = false;
			size_t index = 0;
		};
		uf::stl::vector<void*> local;
		uf::stl::vector<void*> aliases;
		uf::stl::vector<::buffers::Pair> type;
	}
}
#endif

size_t ext::opengl::Device::HandlePool::pageSize = 128;
void ext::opengl::Device::HandlePool::allocate() {
	return allocate(HandlePool::pageSize);
}
void ext::opengl::Device::HandlePool::allocate( size_t count ) {
	size_t initial = available.size();
	available.resize(initial + count);
	switch ( type ) {
		case enums::Command::GENERATE_TEXTURE: GL_ERROR_CHECK(glGenTextures(count, &available[initial])); break;
	#if !UF_USE_OPENGL_FIXED_FUNCTION
		case enums::Command::GENERATE_BUFFER: GL_ERROR_CHECK(glGenBuffers(count, &available[initial])); break;
	#endif
	}
}
void ext::opengl::Device::HandlePool::clear() {
	switch ( type ) {
		case enums::Command::GENERATE_TEXTURE: GL_ERROR_CHECK(glDeleteTextures(available.size(), &available[0])); break;
	#if !UF_USE_OPENGL_FIXED_FUNCTION
		case enums::Command::GENERATE_BUFFER: GL_ERROR_CHECK(glDeleteBuffers(available.size(), &available[0])); break;
	#endif
	}
	available.clear();
	pool.clear();
}
bool ext::opengl::Device::HandlePool::used( ext::opengl::Device::HandlePool::handle_t handle ) {
	return std::find( pool.begin(), pool.end(), handle ) != pool.end();
}
size_t ext::opengl::Device::HandlePool::alloc( ext::opengl::Device::HandlePool::handle_t& target ) {
	switch ( type ) {
		case enums::Command::GENERATE_TEXTURE: if ( glIsTexture(target) ) return SIZE_MAX; break;
	#if !UF_USE_OPENGL_FIXED_FUNCTION
		case enums::Command::GENERATE_BUFFER: if ( glIsBuffer(target) ) return SIZE_MAX; break;
	#endif
	}
	if ( used(target) ) return SIZE_MAX;
	target = available.front();
	available.pop_front();

	size_t index = pool.size();
	pool.emplace_back(target);
	return index;
}
void ext::opengl::Device::HandlePool::free( ext::opengl::Device::HandlePool::handle_t& handle ) {
	ext::opengl::Device::HandlePool::handle_t index = handle;
	handle = 0;

	if ( !used(index) ) return;

	pool.erase( std::remove(pool.begin(), pool.end(), index), pool.end() );

	switch ( type ) {
		case enums::Command::GENERATE_TEXTURE: if ( glIsTexture(index) ) glDeleteTextures(1, &index); break;
	#if !UF_USE_OPENGL_FIXED_FUNCTION
		case enums::Command::GENERATE_BUFFER: if ( glIsBuffer(index) ) glDeleteBuffers(1, &index); break;
	#endif
	}
}

void ext::opengl::Device::initialize() {
	spec::Context::globalInit();
	activateContext();

#if UF_USE_OPENGL_GLDC
	GLdcConfig config;
	glKosInitConfig(&config);
	config.fsaa_enabled = GL_FALSE;
	glKosInitEx(&config);
#elif UF_ENV_DREAMCAST
	glKosInit();
#else
	glewExperimental = GL_TRUE;
	GLenum error;
	if ( (error = glewInit()) != GLEW_OK ) {
		UF_EXCEPTION("ERROR: {}", (const char*) glewGetErrorString(error));
	}
#endif
#if UF_USE_OPENGL_FIXED_FUNCTION
	{
		::buffers::type.emplace_back(::buffers::Pair{.aliased = false, .index = 0});
		::buffers::local.emplace_back((void*) NULL);
	}
#endif
	{
		commandBuffer.initialize(*this);
		commandBuffer.start();
	}
	if ( false ) {
		textures.type = enums::Command::GENERATE_TEXTURE;
		textures.allocate();
		
		buffers.type = enums::Command::GENERATE_BUFFER;
		buffers.allocate();
	}
}
void ext::opengl::Device::destroy() {
	commandBuffer.end();
	commandBuffer.flush();
	for ( auto& pair : this->contexts.container() ) {
		if ( pair.second ) delete pair.second;
		pair.second = NULL;
	}
	spec::Context::globalCleanup();
}
spec::Context& ext::opengl::Device::activateContext() {
	return activateContext( std::this_thread::get_id() );
}
spec::Context& ext::opengl::Device::activateContext( std::thread::id id ) {
	bool exists = this->contexts.has(id);
	auto& context = this->contexts.get(id);
	if ( !exists ) {
		context = (spec::Context*) spec::uni::Context::create( contextSettings, *this->window );
	}
	context->setActive(true);
	return *context;
}

GLuint ext::opengl::Device::createBuffer( enums::Buffer::type_t usage, GLsizeiptr size, void* data, bool aliased ) {
	GLuint index = 0;
// GPU-based buffer
#if !UF_USE_OPENGL_FIXED_FUNCTION
	GLenum usage;
	if ( usage & enums::Buffer::STREAM ) {
		if ( usage & enums::Buffer::DRAW ) usage = GL_STREAM_DRAW;
		if ( usage & enums::Buffer::READ ) usage = GL_STREAM_READ;
		if ( usage & enums::Buffer::COPY ) usage = GL_STREAM_COPY;
	}
	if ( usage & enums::Buffer::STATIC ) {
		if ( usage & enums::Buffer::DRAW ) usage = GL_STATIC_DRAW;
		if ( usage & enums::Buffer::READ ) usage = GL_STATIC_READ;
		if ( usage & enums::Buffer::COPY ) usage = GL_STATIC_COPY;
	}
	if ( usage & enums::Buffer::DYNAMIC ) {
		if ( usage & enums::Buffer::DRAW ) usage = GL_DYNAMIC_DRAW;
		if ( usage & enums::Buffer::READ ) usage = GL_DYNAMIC_READ;
		if ( usage & enums::Buffer::COPY ) usage = GL_DYNAMIC_COPY;
	}

//	GL_ERROR_CHECK(glGenBuffers(1, &index));
	buffers.alloc( index );
	GL_ERROR_CHECK(glBindBuffer(GL_COPY_WRITE_BUFFER, index));
	GL_ERROR_CHECK(glBufferData(GL_COPY_WRITE_BUFFER, size, data, usage));
	GL_ERROR_CHECK(glBindBuffer(GL_COPY_WRITE_BUFFER, 0));
#else
// CPU-based buffer
	index = ::buffers::type.size();
	::buffers::type.emplace_back( ::buffers::Pair{.aliased = aliased, .index = index} );
	
	if ( aliased ) {
		::buffers::aliases.emplace_back( data );
		return index;
	}

	auto& buffer = ::buffers::local.emplace_back();
	size_t requestedSize = (size_t) size;
#if UF_MEMORYPOOL_INVALID_MALLOC
	buffer = uf::memoryPool::global.alloc( requestedSize );
#else
	uf::MemoryPool* memoryPool = uf::memoryPool::global.size() > 0 ? &uf::memoryPool::global : NULL;
	if ( memoryPool ) buffer = memoryPool->alloc( requestedSize );
	else buffer = uf::allocator::malloc_m( requestedSize ); // allocate data for the userdata struct, and then some
#endif
	if ( data ) memcpy( buffer, data, requestedSize );
	else memset( buffer, 0, requestedSize );
#endif
	return index;
}
#if UF_USE_OPENGL_FIXED_FUNCTION
void* ext::opengl::Device::getBuffer( GLuint index ) {
	if ( !index || index >= ::buffers::type.size() ) return NULL;
	const auto& type = ::buffers::type.at(index);
	return type.aliased ? ::buffers::aliases[type.index] : ::buffers::local[type.index];
}
#endif
void ext::opengl::Device::destroyBuffer( GLuint& index ) {
#if !UF_USE_OPENGL_FIXED_FUNCTION
// glDeleteBuffers
	//if ( glIsBuffer(index) ) GL_ERROR_CHECK(glDeleteBuffers(1, index));
	buffers.free(index);
#else
// CPU-based buffer
	// invalid index
	if ( !index || index >= ::buffers::type.size() ) return;
	const auto& type = ::buffers::type.at(index);
	if ( type.aliased ) return;

	auto& buffer = ::buffers::local[type.index];
	if ( buffer ) {
	#if UF_MEMORYPOOL_INVALID_FREE
		uf::memoryPool::global.free( buffer );
	#else
		uf::MemoryPool* memoryPool = uf::memoryPool::global.size() > 0 ? &uf::memoryPool::global : NULL;	
		if ( memoryPool ) memoryPool->free( buffer );
		else free( buffer );
	#endif
	}
#endif

	index = GL_NULL_HANDLE;
}

size_t ext::opengl::Device::createTexture( GLuint& handle ) {
	return textures.alloc(handle);
}
void ext::opengl::Device::destroyTexture( GLuint& handle ) {
	textures.free(handle);
}

#endif