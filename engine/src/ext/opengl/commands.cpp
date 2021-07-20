#if UF_USE_OPENGL

#include <uf/ext/opengl/opengl.h>
#include <uf/ext/opengl/commands.h>
#include <uf/ext/opengl/graphic.h>

#include <uf/utils/memory/vector.h>
#include <bitset>

#include <uf/utils/mesh/mesh.h>
#include <uf/engine/graph/graph.h>

#define VERBOSE false
#define VERBOSE_SUBMIT false

size_t ext::opengl::CommandBuffer::preallocate = 8;

void ext::opengl::CommandBuffer::initialize( Device& device ) {
	this->device = &device;
	if ( !this->mutex ) this->mutex = new std::mutex;
	infos.reserve(CommandBuffer::preallocate);
}
void ext::opengl::CommandBuffer::destroy() {
	this->flush();
	
	if ( this->mutex ) delete this->mutex;
	this->mutex = NULL;
}
void ext::opengl::CommandBuffer::start() {
	if ( state == 1 ) return;
	flush();
	state = 1;
	mutex->lock();
}
void ext::opengl::CommandBuffer::end() {
	if ( state != 1 ) return;
	state = 2;
	mutex->unlock();
}
void ext::opengl::CommandBuffer::record( const CommandBuffer::Info& header ) {
	if ( state != 1 ) return;
	switch ( header.type ) {
		case ext::opengl::enums::Command::CLEAR: {
			InfoClear* info = (InfoClear*) &header;
			info->next = NULL;
			auto& userdata = infos.emplace_back();
			userdata.autoDestruct = false;
			userdata.create<InfoClear>( *info );
			info = &userdata.get<InfoClear>();
			info->type = enums::Command::CLEAR;
		} break;
		case ext::opengl::enums::Command::VIEWPORT: {
			InfoViewport* info = (InfoViewport*) &header;
			info->next = NULL;
			if ( info->size.x == 0 ) info->size.x = ext::opengl::settings::width;
			if ( info->size.y == 0 ) info->size.y = ext::opengl::settings::height;
			auto& userdata = infos.emplace_back();
			userdata.autoDestruct = false;
			userdata.create<InfoViewport>( *info );
			info = &userdata.get<InfoViewport>();
			info->type = enums::Command::VIEWPORT;
		} break;
		case ext::opengl::enums::Command::VARIANT: {
			InfoVariant* info = (InfoVariant*) &header;
			info->next = NULL;
			auto& userdata = infos.emplace_back();
			userdata.autoDestruct = false;
			userdata.create<InfoVariant>( *info );
			info = &userdata.get<InfoVariant>();
			info->type = enums::Command::VARIANT;
		} break;
		case ext::opengl::enums::Command::DRAW: {
			InfoDraw* info = (InfoDraw*) &header;
			info->next = NULL;
			auto& userdata = infos.emplace_back();
			userdata.autoDestruct = false;
			userdata.create<InfoDraw>( *info );
			info = &userdata.get<InfoDraw>();
			info->type = enums::Command::DRAW;
		} break;
		default: {
		} break;
	}
}
void ext::opengl::CommandBuffer::record( const InfoVariant::function_t& lambda ) {
	InfoVariant lambdaCommandInfo = {};
	lambdaCommandInfo.type = ext::opengl::enums::Command::VARIANT;
	lambdaCommandInfo.lambda = lambda;
	record(lambdaCommandInfo);
}
void ext::opengl::CommandBuffer::record( const CommandBuffer& commandBuffer ) {
	infos.insert( infos.end(), commandBuffer.infos.begin(), commandBuffer.infos.end() );
}
void ext::opengl::CommandBuffer::submit() {
	if ( infos.empty() ) return;

	mutex->lock();
#if UF_ENV_DREAMCAST
	static GLint maxTextures = 80;
#else
	static GLint maxTextures = 0;
	if ( maxTextures <= 0 ) {
		glGetIntegerv(GL_MAX_COMBINED_TEXTURE_IMAGE_UNITS, &maxTextures);
	}
#endif
	for ( auto& info : infos ) {
		CommandBuffer::Info* header = (CommandBuffer::Info*) (void*) info;
		switch ( header->type ) {
			case ext::opengl::enums::Command::CLEAR: {
				InfoClear* info = (InfoClear*) header;
				GL_ERROR_CHECK(glClearColor(info->color[0], info->color[1], info->color[2], info->color[3]));
				GL_ERROR_CHECK(glClearDepth(info->depth));
				GL_ERROR_CHECK(glClear(info->bits));
			//	GL_ERROR_CHECK(glLightModelfv(GL_LIGHT_MODEL_AMBIENT, &info->color[0]));
			} break;
			case ext::opengl::enums::Command::VIEWPORT: {
				InfoViewport* info = (InfoViewport*) header;
				GL_ERROR_CHECK(glViewport(info->corner[0], info->corner[1], info->size[0], info->size[1]));
			} break;
			case ext::opengl::enums::Command::VARIANT: {
				InfoVariant* info = (InfoVariant*) header;
				if ( info->lambda ) info->lambda();
			} break;
			case ext::opengl::enums::Command::DRAW: {
				InfoDraw* info = (InfoDraw*) header;
				drawIndexed( *info );
			} break;
			default: {
			} break;
		}
	}
	state = 3;
	mutex->unlock();
}
void ext::opengl::CommandBuffer::flush() {
	mutex->lock();
	for ( auto& userdata : infos ) userdata.destroy();
	infos.clear();
	state = 0;
	mutex->unlock();
}
size_t ext::opengl::CommandBuffer::size() const {
	return infos.size();
}

pod::Matrix4f ext::opengl::CommandBuffer::bindUniform( const ext::opengl::Buffer::Descriptor& descriptor ) {
#if UF_USE_OPENGL_FIXED_FUNCTION
	pod::Uniform* uniform = (pod::Uniform*) ((uint8_t*) device->getBuffer( descriptor.buffer ) + descriptor.offset);

	GL_ERROR_CHECK(glMatrixMode(GL_MODELVIEW));
	GL_ERROR_CHECK(glLoadMatrixf( &uniform->modelView[0] ));

	GL_ERROR_CHECK(glMatrixMode(GL_PROJECTION));
	GL_ERROR_CHECK(glLoadMatrixf( &uniform->projection[0] ));

	return uniform->projection * uniform->modelView;
#endif
}
void ext::opengl::CommandBuffer::drawIndexed( const ext::opengl::CommandBuffer::InfoDraw& drawInfo ) {
	if ( drawInfo.matrices.model && drawInfo.matrices.view && drawInfo.matrices.projection ) {
		pod::Matrix4f modelView = uf::matrix::multiply( *drawInfo.matrices.view, *drawInfo.matrices.model );

		GL_ERROR_CHECK(glMatrixMode(GL_MODELVIEW));
		GL_ERROR_CHECK(glLoadMatrixf( &modelView[0] ));

		GL_ERROR_CHECK(glMatrixMode(GL_PROJECTION));
		GL_ERROR_CHECK(glLoadMatrixf( (float*) drawInfo.matrices.projection ));
	} else if ( drawInfo.matrices.model && drawInfo.matrices.projection ) {
		GL_ERROR_CHECK(glMatrixMode(GL_MODELVIEW));
		GL_ERROR_CHECK(glLoadMatrixf( (float*) drawInfo.matrices.model ));

		GL_ERROR_CHECK(glMatrixMode(GL_PROJECTION));
		GL_ERROR_CHECK(glLoadMatrixf( (float*) drawInfo.matrices.projection ));
	} else if ( drawInfo.matrices.view && drawInfo.matrices.projection ) {
		GL_ERROR_CHECK(glMatrixMode(GL_MODELVIEW));
		GL_ERROR_CHECK(glLoadMatrixf( (float*) drawInfo.matrices.view ));

		GL_ERROR_CHECK(glMatrixMode(GL_PROJECTION));
		GL_ERROR_CHECK(glLoadMatrixf( (float*) drawInfo.matrices.projection ));
	}

	if ( drawInfo.descriptor.cullMode == GL_NONE ) {
		GL_ERROR_CHECK(glDisable(GL_CULL_FACE));
	} else {
		GL_ERROR_CHECK(glEnable(GL_CULL_FACE));
		GL_ERROR_CHECK(glFrontFace(drawInfo.descriptor.frontFace));
		GL_ERROR_CHECK(glCullFace(drawInfo.descriptor.cullMode));
	}
	if ( drawInfo.descriptor.depth.test ) {
		GL_ERROR_CHECK(glEnable(GL_DEPTH_TEST));
	} else {
		GL_ERROR_CHECK(glDisable(GL_DEPTH_TEST));
	}
	GL_ERROR_CHECK(glDepthMask(drawInfo.descriptor.depth.write ? GL_TRUE : GL_FALSE));

	// CPU-buffer based command dispatching
	if ( drawInfo.attributes.normal.pointer ) GL_ERROR_CHECK(glEnableClientState(GL_NORMAL_ARRAY));
	if ( drawInfo.attributes.color.pointer ) GL_ERROR_CHECK(glEnableClientState(GL_COLOR_ARRAY));
	if ( drawInfo.attributes.uv.pointer ) GL_ERROR_CHECK(glEnableClientState(GL_TEXTURE_COORD_ARRAY));
	GL_ERROR_CHECK(glEnableClientState(GL_VERTEX_ARRAY));

	GLenum indicesType = GL_UNSIGNED_INT;
	switch ( drawInfo.attributes.index.stride ) {
		case sizeof(uint32_t): indicesType = GL_UNSIGNED_INT; break;
		case sizeof(uint16_t): indicesType = GL_UNSIGNED_SHORT; break;
		case sizeof(uint8_t): indicesType = GL_UNSIGNED_BYTE; break;
	}

	if ( drawInfo.textures.primary.image && drawInfo.attributes.uv.pointer ) {
		static GLuint previous = 0;
		if ( previous != drawInfo.textures.primary.image ) {
			previous = drawInfo.textures.primary.image;
			GL_ERROR_CHECK(glClientActiveTexture(GL_TEXTURE0));
			GL_ERROR_CHECK(glActiveTexture(GL_TEXTURE0));
			GL_ERROR_CHECK(glEnable(drawInfo.textures.primary.viewType));
			GL_ERROR_CHECK(glEnableClientState(GL_TEXTURE_COORD_ARRAY));
			GL_ERROR_CHECK(glBindTexture(drawInfo.textures.primary.viewType, drawInfo.textures.primary.image));
			GL_ERROR_CHECK(glTexEnvi(GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_REPLACE));
		}
		GL_ERROR_CHECK(glTexCoordPointer(2, GL_FLOAT, drawInfo.attributes.uv.stride, drawInfo.attributes.uv.pointer));
	}
	if ( drawInfo.textures.secondary.image && drawInfo.attributes.st.pointer ) {
		static GLuint previous = 0;
		if ( previous != drawInfo.textures.secondary.image ) {
			previous = drawInfo.textures.secondary.image;
			GL_ERROR_CHECK(glClientActiveTexture(GL_TEXTURE1));
			GL_ERROR_CHECK(glActiveTexture(GL_TEXTURE1));
			GL_ERROR_CHECK(glEnable(drawInfo.textures.secondary.viewType));
			GL_ERROR_CHECK(glEnableClientState(GL_TEXTURE_COORD_ARRAY));
			GL_ERROR_CHECK(glBindTexture(drawInfo.textures.secondary.viewType, drawInfo.textures.secondary.image));
			GL_ERROR_CHECK(glTexEnvi(GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_MODULATE));
			GL_ERROR_CHECK(glTexCoordPointer(2, GL_FLOAT, drawInfo.attributes.st.stride, drawInfo.attributes.st.pointer));
		}
	#if UF_ENV_DREAMCAST
		GL_ERROR_CHECK(glDisable(GL_BLEND));
	#endif
	}
	
	if ( drawInfo.attributes.normal.pointer ) GL_ERROR_CHECK(glNormalPointer(GL_FLOAT, drawInfo.attributes.normal.stride, drawInfo.attributes.normal.pointer));
	if ( drawInfo.attributes.color.pointer ) GL_ERROR_CHECK(glColorPointer(3, GL_FLOAT, drawInfo.attributes.color.stride, drawInfo.attributes.color.pointer));
	GL_ERROR_CHECK(glVertexPointer(3, GL_FLOAT, drawInfo.attributes.position.stride, drawInfo.attributes.position.pointer));
	GL_ERROR_CHECK(glDrawElements(GL_TRIANGLES, drawInfo.attributes.index.length, indicesType, drawInfo.attributes.index.pointer));

#if 0
	if ( drawInfo.textures.secondary.image ) {
	#if UF_ENV_DREAMCAST
		GL_ERROR_CHECK(glEnable(GL_BLEND));
	#endif
		GL_ERROR_CHECK(glClientActiveTexture(GL_TEXTURE1));
		GL_ERROR_CHECK(glActiveTexture(GL_TEXTURE1));
		GL_ERROR_CHECK(glBindTexture(drawInfo.textures.secondary.viewType, 0));
		GL_ERROR_CHECK(glDisable(drawInfo.textures.secondary.viewType));
	}
	if ( drawInfo.textures.primary.image ) {
		GL_ERROR_CHECK(glClientActiveTexture(GL_TEXTURE0));
		GL_ERROR_CHECK(glActiveTexture(GL_TEXTURE0));
		GL_ERROR_CHECK(glBindTexture(drawInfo.textures.primary.viewType, 0));
		GL_ERROR_CHECK(glDisable(drawInfo.textures.primary.viewType));
	}
#endif

	if ( drawInfo.attributes.normal.pointer ) GL_ERROR_CHECK(glDisableClientState(GL_NORMAL_ARRAY));
	if ( drawInfo.attributes.color.pointer ) GL_ERROR_CHECK(glDisableClientState(GL_COLOR_ARRAY));
	if ( drawInfo.attributes.uv.pointer ) GL_ERROR_CHECK(glDisableClientState(GL_TEXTURE_COORD_ARRAY));
	GL_ERROR_CHECK(glDisableClientState(GL_VERTEX_ARRAY));
}
#endif