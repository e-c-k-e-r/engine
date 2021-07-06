#if UF_USE_OPENGL

#include <uf/ext/opengl/opengl.h>
#include <uf/ext/opengl/commands.h>
#include <uf/ext/opengl/graphic.h>

#include <uf/utils/memory/vector.h>
#include <bitset>

#include <uf/utils/mesh/mesh.h>
#include <uf/engine/graph/mesh.h>

#define VERBOSE false
#define VERBOSE_SUBMIT false

#define GL_DRAW_ARRAYS( vertexPointer, vertices )\
	GL_ERROR_CHECK(glBegin(GL_TRIANGLES)); {\
		for ( size_t i = 0; i < vertices; i++ ) {\
			uint8_t* vertex = vertexPointer + (i * vertexStride);\
			if ( vertexAttributeNormal.name != "" ) {\
				float* normal = (float*) (vertex + vertexAttributeNormal.offset);\
				GL_ERROR_CHECK(glNormal3f(normal[0], normal[1], normal[2]));\
			}\
			if ( vertexAttributeColor.name != "" ) {\
				float* color = (float*) (vertex + vertexAttributeColor.offset);\
				GL_ERROR_CHECK(glColor3f(color[0], color[1], color[2]));\
			}\
			if ( vertexAttributeUv.name != "" ) {\
				float* uv = (float*) (vertex + vertexAttributeUv.offset);\
				GL_ERROR_CHECK(glTexCoord2f(uv[0], uv[1]));\
			}\
			{\
				float* position = (float*) (vertex + vertexAttributePosition.offset);\
				GL_ERROR_CHECK(glVertex3f(position[0], position[1], position[2]));\
			}\
		}\
	} GL_ERROR_CHECK(glEnd());

#define GL_DRAW_ELEMENTS( indicesPointer, indices )\
	GL_ERROR_CHECK(glBegin(GL_TRIANGLES)); {\
		for ( size_t i = 0; i < indices; ++i ) {\
			uint32_t index = 0;\
			void* indexSrc = indexPointer + (currentIndex * indexStride);\
			switch ( indexStride ) {\
				case sizeof( uint8_t): index = *(( uint8_t*) indexSrc); break;\
				case sizeof(uint16_t): index = *((uint16_t*) indexSrc); break;\
				case sizeof(uint32_t): index = *((uint32_t*) indexSrc); break;\
			}\
			void* vertex = vertexPointer + (index * vertexStride);\
			if ( vertexAttributeNormal.name != "" ) {\
				float* normal = (float*) (vertex + vertexAttributeNormal.offset);\
				GL_ERROR_CHECK(glNormal3f(normal[0], normal[1], normal[2]));\
			}\
			if ( vertexAttributeColor.name != "" ) {\
				float* color = (float*) (vertex + vertexAttributeColor.offset);\
				GL_ERROR_CHECK(glColor3f(color[0], color[1], color[2]));\
			}\
			if ( vertexAttributeUv.name != "" ) {\
				float* uv = (float*) (vertex + vertexAttributeUv.offset);\
				GL_ERROR_CHECK(glTexCoord2f(uv[0], uv[1]));\
			}\
			{\
				float* position = (float*) (vertex + vertexAttributePosition.offset);\
				GL_ERROR_CHECK(glVertex3f(position[0], position[1], position[2]));\
			}\
		}\
	} GL_ERROR_CHECK(glEnd());

#define GL_BIND_POINTERS()\
	if ( vertexAttributeNormal.name != "" ) GL_ERROR_CHECK(glNormalPointer(GL_FLOAT, vertexStride, vertexPointer + vertexAttributeNormal.offset));\
	if ( vertexAttributeColor.name != "" ) GL_ERROR_CHECK(glColorPointer(3, GL_FLOAT, vertexStride, vertexPointer + vertexAttributeColor.offset));\
	if ( vertexAttributeUv.name != "" ) GL_ERROR_CHECK(glTexCoordPointer(2, GL_FLOAT, vertexStride, vertexPointer + vertexAttributeUv.offset));\
	if ( vertexAttributeSt.name != "" ) {\
		glClientActiveTexture(GL_TEXTURE1);\
		GL_ERROR_CHECK(glTexCoordPointer(2, GL_FLOAT, vertexStride, vertexPointer + vertexAttributeSt.offset));\
		glClientActiveTexture(GL_TEXTURE0);\
	}\
	GL_ERROR_CHECK(glVertexPointer(3, GL_FLOAT, vertexStride, vertexPointer + vertexAttributePosition.offset));

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
	if ( state == 1 ) {
		std::cout << "ERROR: COMMAND BUFFER " << this << " NOT READY FOR RECORDING" << std::endl;
		return;
	}
	flush();
	state = 1;
//	if ( VERBOSE ) std::cout << "==== ["<<this<<"] COMMAND BUFFER RECORDING START ====\n";
	mutex->lock();
}
void ext::opengl::CommandBuffer::end() {
	if ( state != 1 ) {
		std::cout << "ERROR: COMMAND BUFFER " << this << " WAS NOT RECORDING" << std::endl;
		return;
	}
	if ( !infos.empty() ) {
		if ( VERBOSE ) std::cout << "==== ["<<this<<"] COMMAND BUFFER RECORDING END ==== " << std::endl;
	}
	state = 2;
	mutex->unlock();

	// traverse list chain
/*
	Info* currentInfo = (Info*) (void*) startInfo;
	while ( currentInfo ) {
		std::cout << "COMMAND: " << currentInfo << "[" << currentInfo->type << "]";
		currentInfo = currentInfo->next;
		if ( currentInfo ) std::cout << " -> ";
	}
	std::cout << std::endl;
*/
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
	/*
		case ext::opengl::enums::Command::BIND_BUFFER: {
			InfoBuffer* info = (InfoBuffer*) &header;
			info->next = NULL;
			auto& userdata = infos.emplace_back();
			userdata.autoDestruct = false;
			userdata.create<InfoBuffer>( *info );
			info = &userdata.get<InfoBuffer>();
			info->type = enums::Command::BIND_BUFFER;
		} break;
		case ext::opengl::enums::Command::BIND_GRAPHIC_BUFFER: {
			InfoGraphicBuffer* info = (InfoGraphicBuffer*) &header;
			info->next = NULL;
			auto& userdata = infos.emplace_back();
			userdata.autoDestruct = false;
			userdata.create<InfoGraphicBuffer>( *info );
			info = &userdata.get<InfoGraphicBuffer>();
			info->type = enums::Command::BIND_GRAPHIC_BUFFER;
		} break;
		case ext::opengl::enums::Command::BIND_TEXTURE: {
			InfoTexture* info = (InfoTexture*) &header;
			info->next = NULL;
			auto& userdata = infos.emplace_back();
			userdata.autoDestruct = false;
			userdata.create<InfoTexture>( *info );
			info = &userdata.get<InfoTexture>();
			info->type = enums::Command::BIND_TEXTURE;
		} break;
		case ext::opengl::enums::Command::BIND_PIPELINE: {
			InfoPipeline* info = (InfoPipeline*) &header;
			info->next = NULL;
			auto& userdata = infos.emplace_back();
			userdata.autoDestruct = false;
			userdata.create<InfoPipeline>( *info );
			info = &userdata.get<InfoPipeline>();
			info->type = enums::Command::BIND_PIPELINE;
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
		case ext::opengl::enums::Command::GENERATE_TEXTURE: {
			InfoGenerateTexture* info = (InfoGenerateTexture*) &header;
			auto& userdata = infos.emplace_back();
			userdata.autoDestruct = false;
			userdata.create( *info );
			info = &userdata.get<InfoGenerateTexture>();
			info->type = enums::Command::GENERATE_TEXTURE;
		} break;
	*/
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
			//	if ( info->indexBuffer.buffer ) drawIndexed( *info );
			//	else draw( *info );
			} break;
			default: {
			} break;
		}
	}
#if 0
	CommandBuffer::Info* vertexBufferInfo = GL_NULL_HANDLE;
	CommandBuffer::Info* indexBufferInfo = GL_NULL_HANDLE;
	uf::stl::vector<CommandBuffer::InfoTexture*> textureInfos;
	textureInfos.reserve(maxTextures);
	if ( state == 2 && VERBOSE_SUBMIT ) std::cout << "==== ["<<this<<"] COMMAND BUFFER SUBMIT START ====\n";
	// 50us each process
	for ( auto& info : infos ) {
		CommandBuffer::Info* header = (CommandBuffer::Info*) (void*) info;
		switch ( header->type ) {
			case ext::opengl::enums::Command::CLEAR: {
				InfoClear* info = (InfoClear*) header;
				if ( state == 2 && VERBOSE_SUBMIT ) std::cout << "["<<info<<"] CLEARING SCREEN TO " << uf::vector::toString( info->color ) << " | DEPTH: " << info->depth << " | BITS: " << std::bitset<32>(info->bits) << "\n";
				GL_ERROR_CHECK(glClearColor(info->color[0], info->color[1], info->color[2], info->color[3]));
				GL_ERROR_CHECK(glClearDepth(info->depth));
				GL_ERROR_CHECK(glClear(info->bits));
				GL_ERROR_CHECK(glLightModelfv(GL_LIGHT_MODEL_AMBIENT, &info->color[0]));
			} break;
			case ext::opengl::enums::Command::VIEWPORT: {
				InfoViewport* info = (InfoViewport*) header;
				if ( state == 2 && VERBOSE_SUBMIT ) std::cout << "["<<info<<"] SETTING VIEWPORT TO " << uf::vector::toString(info->corner) << " | " << uf::vector::toString(info->size) << "\n";
				GL_ERROR_CHECK(glViewport(info->corner[0], info->corner[1], info->size[0], info->size[1]));
			} break;
			case ext::opengl::enums::Command::VARIANT: {
				InfoVariant* info = (InfoVariant*) header;
				if ( state == 2 && VERBOSE_SUBMIT ) std::cout << "["<<info<<"] CALLING LAMBDA\n";
				if ( info->lambda ) info->lambda();
			} break;
			case ext::opengl::enums::Command::BIND_BUFFER: {
				InfoBuffer* info = (InfoBuffer*) header;
				if ( state == 2 && VERBOSE_SUBMIT ) std::cout << "["<<info<<"] BINDING BUFFER: " << info->descriptor.buffer << " | FLAGS: " << /*std::bitset<16>*/(info->usage) << "\n";
				if ( info->usage & enums::Buffer::UNIFORM ) bindUniform( info->descriptor );
				if ( info->usage & enums::Buffer::VERTEX ) vertexBufferInfo = header;
				if ( info->usage & enums::Buffer::INDEX ) indexBufferInfo = header;
			} break;
			case ext::opengl::enums::Command::BIND_GRAPHIC_BUFFER: {
				InfoGraphicBuffer* info = (InfoGraphicBuffer*) header;
				if ( state == 2 && VERBOSE_SUBMIT ) std::cout << "["<<info<<"] BINDING BUFFER: ";
				auto& buffer = info->graphic->buffers[info->bufferIndex];
				if ( state == 2 && VERBOSE_SUBMIT ) std::cout << buffer.descriptor.buffer << " | FLAGS: " << /*std::bitset<16>*/(buffer.usage) << "\n";
				if ( buffer.usage & enums::Buffer::UNIFORM ) bindUniform( buffer.descriptor );
				if ( buffer.usage & enums::Buffer::VERTEX ) vertexBufferInfo = header;
				if ( buffer.usage & enums::Buffer::INDEX ) indexBufferInfo = header;
			} break;
			case ext::opengl::enums::Command::BIND_TEXTURE: {
				InfoTexture* info = (InfoTexture*) header;
				if ( state == 2 && VERBOSE_SUBMIT ) std::cout << "["<<info<<"] BINDING TEXTURE: " << info->descriptor.image << " | " << info->descriptor.viewType << "\n";
				textureInfos.emplace_back(info);
			} break;
			case ext::opengl::enums::Command::BIND_PIPELINE: {
				InfoPipeline* info = (InfoPipeline*) header;
				if ( state == 2 && VERBOSE_SUBMIT ) std::cout << "["<<info<<"] BINDING PIPELINE: " << info->descriptor.pipeline << " | " << info->descriptor.vertexArray << "\n";
			#if !UF_USE_OPENGL_FIXED_FUNCTION
				GL_ERROR_CHECK(glUseProgram(info->descriptor.pipeline));
			#endif
			} break;
			case ext::opengl::enums::Command::DRAW: {
				InfoDraw* info = (InfoDraw*) header;
				if ( state == 2 && VERBOSE_SUBMIT ) std::cout << "["<<info<<"] DRAW CALL | COUNT: " << info->length << " | VERTEX INFO: " << vertexBufferInfo << " | INDEX INFO: " << indexBufferInfo << "\n";
				if ( vertexBufferInfo ) {
					if ( indexBufferInfo ) drawIndexed( *info, *vertexBufferInfo, *indexBufferInfo, textureInfos );
					else draw( *info, *vertexBufferInfo, textureInfos );
				}
				textureInfos.clear();
				vertexBufferInfo = NULL;
				indexBufferInfo = NULL;
			} break;
			default: {
				if ( state == 2 && VERBOSE_SUBMIT ) {
					std::cout << "["<<header<<"] UNKNOWN COMMAND TYPE: " << header->type << ": " << info.data().len << std::endl;;
				}
			} break;
		}
	}
	if ( state == 2 && VERBOSE_SUBMIT ) std::cout << "==== ["<<this<<"] COMMAND BUFFER SUBMIT END ==== " << std::endl;
#endif
	state = 3;
	mutex->unlock();
}
void ext::opengl::CommandBuffer::flush() {
	mutex->lock();
	for ( auto& userdata : infos ) userdata.destroy();
	infos.clear();
	state = 0;
/*
	startInfo = NULL;
	endInfo = NULL;
*/
	mutex->unlock();
}
size_t ext::opengl::CommandBuffer::size() const {
	return infos.size();
}

pod::Matrix4f ext::opengl::CommandBuffer::bindUniform( const ext::opengl::Buffer::Descriptor& descriptor ) {
#if UF_USE_OPENGL_FIXED_FUNCTION
	pod::Uniform* uniform = (pod::Uniform*) (device->getBuffer( descriptor.buffer ) + descriptor.offset);

	GL_ERROR_CHECK(glMatrixMode(GL_MODELVIEW));
	GL_ERROR_CHECK(glLoadMatrixf( &uniform->modelView[0] ));

	GL_ERROR_CHECK(glMatrixMode(GL_PROJECTION));
	GL_ERROR_CHECK(glLoadMatrixf( &uniform->projection[0] ));

	return uniform->projection * uniform->modelView;
#endif
}
#if 0
void ext::opengl::CommandBuffer::draw( const ext::opengl::CommandBuffer::InfoDraw& drawInfo, const ext::opengl::CommandBuffer::Info& vertexBufferInfo, const uf::stl::vector<InfoTexture*>& textureInfos ) {
	ext::opengl::Buffer::Descriptor vertexBuffer = {};

	switch ( vertexBufferInfo.type ) {
		case ext::opengl::enums::Command::BIND_BUFFER: {
			ext::opengl::CommandBuffer::InfoBuffer* info = (ext::opengl::CommandBuffer::InfoBuffer*) &vertexBufferInfo;
			vertexBuffer = info->descriptor;
		} break;
		case ext::opengl::enums::Command::BIND_GRAPHIC_BUFFER: {
			ext::opengl::CommandBuffer::InfoGraphicBuffer* info = (ext::opengl::CommandBuffer::InfoGraphicBuffer*) &vertexBufferInfo;
			vertexBuffer = info->graphic->buffers[info->bufferIndex].descriptor;
		} break;
	}
	if ( !vertexBuffer.buffer ) return;

	size_t vertexStride = drawInfo.descriptor.attributes.vertex.size;
	size_t vertices = vertexBuffer.range / vertexStride;
	
	uf::renderer::VertexDescriptor 	vertexAttributePosition, 
									vertexAttributeNormal,
									vertexAttributeColor,
									vertexAttributeUv,
									vertexAttributeSt,
									vertexAttributeId;

	for ( auto& attribute : drawInfo.descriptor.attributes.descriptor ) {
		if ( attribute.name == "position" ) vertexAttributePosition = attribute;
		else if ( attribute.name == "normal" ) vertexAttributeNormal = attribute;
		else if ( attribute.name == "color" ) vertexAttributeColor = attribute;
		else if ( attribute.name == "uv" ) vertexAttributeUv = attribute;
		else if ( attribute.name == "st" ) vertexAttributeSt = attribute;
		else if ( attribute.name == "id" ) vertexAttributeId = attribute;
	}
	if ( vertexAttributePosition.name == "" ) return;

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

	// GPU-buffer based command dispatching
#if !UF_USE_OPENGL_FIXED_FUNCTION

#else
	// CPU-buffer based command dispatching
	uint8_t* vertexPointer = (uint8_t*) ( device->getBuffer( vertexBuffer.buffer ) + vertexBuffer.offset );

	// vertices do not need to be transformed
	//if ( vertexAttributeId.name == "" )
#if !UF_USE_OPENGL_IMMEDIATE_MODE
	if ( vertexAttributeNormal.name != "" )	GL_ERROR_CHECK(glEnableClientState(GL_NORMAL_ARRAY));
	if ( vertexAttributeColor.name != "" )GL_ERROR_CHECK(glEnableClientState(GL_COLOR_ARRAY));
	if ( vertexAttributeUv.name != "" )	GL_ERROR_CHECK(glEnableClientState(GL_TEXTURE_COORD_ARRAY));
	GL_ERROR_CHECK(glEnableClientState(GL_VERTEX_ARRAY));
	
	
	// no ID attribute found, fallback
	if ( vertexAttributeId.name == "" ) {
		if ( !textureInfos.empty() ) {
			InfoTexture* info = textureInfos.front();
			GL_ERROR_CHECK(glEnable(info->descriptor.viewType));
			GL_ERROR_CHECK(glBindTexture(info->descriptor.viewType, info->descriptor.image));
		}
		GL_BIND_POINTERS();
		GL_ERROR_CHECK(glDrawArrays(GL_TRIANGLES, 0, vertices));
	} else {
		if ( textureInfos.empty() ) {
			GL_BIND_POINTERS();
			GL_ERROR_CHECK(glDrawArrays(GL_TRIANGLES, 0, vertices));
		} else if ( textureInfos.size() == 1 ) {
			InfoTexture* info = textureInfos.front();
			GL_ERROR_CHECK(glEnable(info->descriptor.viewType));
			GL_ERROR_CHECK(glBindTexture(info->descriptor.viewType, info->descriptor.image));
			GL_BIND_POINTERS();
			GL_ERROR_CHECK(glDrawArrays(GL_TRIANGLES, 0, vertices));
		} else {
			uf::stl::vector<uf::stl::vector<uf::renderer::index_t>> sorted( textureInfos.size() );
			for ( uf::renderer::index_t index = 0; index < vertices; ++index ) {
				uint8_t* vertex = vertexPointer + (index * vertexStride);

				const pod::Vector<uf::graph::id_t,2>& id = *((pod::Vector<uf::graph::id_t,2>*) (vertex + vertexAttributeId.offset));
				size_t textureId = id.y;
				if ( textureInfos.size() < textureId ) continue;
				sorted[textureId].emplace_back(index);
			}
			GLenum indicesType = GL_UNSIGNED_INT;
			switch ( sizeof(uf::renderer::index_t) ) {
				case sizeof(uint32_t): indicesType = GL_UNSIGNED_INT; break;
				case sizeof(uint16_t): indicesType = GL_UNSIGNED_SHORT; break;
				case sizeof(uint8_t): indicesType = GL_UNSIGNED_BYTE; break;
			}
			for ( size_t textureId = 0; textureId < sorted.size(); ++textureId ) {
				auto& indices = sorted[textureId];
				if ( indices.empty() ) continue;
				InfoTexture* info = textureInfos[textureId];
				GL_ERROR_CHECK(glEnable(info->descriptor.viewType));
				GL_ERROR_CHECK(glBindTexture(info->descriptor.viewType, info->descriptor.image));
				GL_BIND_POINTERS();
				GL_ERROR_CHECK(glDrawElements(GL_TRIANGLES, indices.size(), indicesType, &indices[0]));
			}
		}
	}
	
	if ( vertexAttributeNormal.name != "" )	GL_ERROR_CHECK(glDisableClientState(GL_NORMAL_ARRAY));
	if ( vertexAttributeColor.name != "" )	GL_ERROR_CHECK(glDisableClientState(GL_COLOR_ARRAY));
	if ( vertexAttributeUv.name != "" )	GL_ERROR_CHECK(glDisableClientState(GL_TEXTURE_COORD_ARRAY));
	GL_ERROR_CHECK(glDisableClientState(GL_VERTEX_ARRAY));
#else
	// no ID attribute found, fallback
	if ( vertexAttributeId.name == "" ) {
		if ( !textureInfos.empty() ) {
			InfoTexture* info = textureInfos.front();
			GL_ERROR_CHECK(glEnable(info->descriptor.viewType));
			GL_ERROR_CHECK(glBindTexture(info->descriptor.viewType, info->descriptor.image));
		}
		GL_DRAW_ARRAYS( vertexPointer, vertices );
	} else {
		if ( textureInfos.empty() ) {
			GL_DRAW_ARRAYS( vertexPointer, vertices );
		} else if ( textureInfos.size() == 1 ) {
			InfoTexture* info = textureInfos.front();
			GL_ERROR_CHECK(glEnable(info->descriptor.viewType));
			GL_ERROR_CHECK(glBindTexture(info->descriptor.viewType, info->descriptor.image));
			GL_DRAW_ARRAYS( vertexPointer, vertices );
		} else {
			uf::stl::vector<uf::stl::vector<uf::renderer::index_t>> sorted( textureInfos.size() );
			for ( uf::renderer::index_t index = 0; index < vertices; ++index ) {
				uint8_t* vertex = vertexPointer + (index * vertexStride);

				const pod::Vector<uf::graph::id_t,2>& id = *((pod::Vector<uf::graph::id_t,2>*) (vertex + vertexAttributeId.offset));
				size_t textureId = id.y;
				if ( textureInfos.size() < textureId ) continue;
				sorted[textureId].emplace_back(index);
			}
			for ( size_t textureId = 0; textureId < sorted.size(); ++textureId ) {
				auto& indices = sorted[textureId];
				if ( indices.empty() ) continue;
				InfoTexture* info = textureInfos[textureId];
				GL_ERROR_CHECK(glEnable(info->descriptor.viewType));
				GL_ERROR_CHECK(glBindTexture(info->descriptor.viewType, info->descriptor.image));
				GL_DRAW_ELEMENTS( (&indices[0]) , (indices.size()) );
			}
		}
	}
#endif
#endif
}
#endif
void ext::opengl::CommandBuffer::drawIndexed( const ext::opengl::CommandBuffer::InfoDraw& drawInfo ) {
	auto projectionViewMatrix = bindUniform( drawInfo.uniformBuffer );

	size_t indices = drawInfo.descriptor.indices;
	size_t indicesStride = drawInfo.descriptor.attributes.index.size;
	size_t vertexStride = drawInfo.descriptor.attributes.vertex.size;
	size_t vertices = drawInfo.vertexBuffer.range / vertexStride;

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
	void* vertexPointer = (void*) ( device->getBuffer( drawInfo.vertexBuffer.buffer ) + drawInfo.vertexBuffer.offset );
	uf::renderer::index_t* indicesPointer = (uf::renderer::index_t*) ( device->getBuffer( drawInfo.indexBuffer.buffer ) + drawInfo.indexBuffer.offset );

	if ( drawInfo.attributes.normal ) GL_ERROR_CHECK(glEnableClientState(GL_NORMAL_ARRAY));
	if ( drawInfo.attributes.color ) GL_ERROR_CHECK(glEnableClientState(GL_COLOR_ARRAY));
	if ( drawInfo.attributes.uv ) GL_ERROR_CHECK(glEnableClientState(GL_TEXTURE_COORD_ARRAY));
	GL_ERROR_CHECK(glEnableClientState(GL_VERTEX_ARRAY));

	GLenum indicesType = GL_UNSIGNED_INT;
	switch ( indicesStride ) {
		case sizeof(uint32_t): indicesType = GL_UNSIGNED_INT; break;
		case sizeof(uint16_t): indicesType = GL_UNSIGNED_SHORT; break;
		case sizeof(uint8_t): indicesType = GL_UNSIGNED_BYTE; break;
	}

	if ( drawInfo.texture.image && drawInfo.attributes.uv ) {
		GL_ERROR_CHECK(glClientActiveTexture(GL_TEXTURE0));
		GL_ERROR_CHECK(glActiveTexture(GL_TEXTURE0));
		GL_ERROR_CHECK(glEnable(drawInfo.texture.viewType));
		GL_ERROR_CHECK(glEnableClientState(GL_TEXTURE_COORD_ARRAY));
		GL_ERROR_CHECK(glBindTexture(drawInfo.texture.viewType, drawInfo.texture.image));
		GL_ERROR_CHECK(glTexEnvi(GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_REPLACE));
		GL_ERROR_CHECK(glTexCoordPointer(2, GL_FLOAT, vertexStride, vertexPointer + drawInfo.attributes.uv));
	}
	if ( drawInfo.auxTexture.image && drawInfo.attributes.st ) {
		GL_ERROR_CHECK(glClientActiveTexture(GL_TEXTURE1));
		GL_ERROR_CHECK(glActiveTexture(GL_TEXTURE1));
		GL_ERROR_CHECK(glEnable(drawInfo.auxTexture.viewType));
		GL_ERROR_CHECK(glEnableClientState(GL_TEXTURE_COORD_ARRAY));
		GL_ERROR_CHECK(glBindTexture(drawInfo.auxTexture.viewType, drawInfo.auxTexture.image));
		GL_ERROR_CHECK(glTexEnvi(GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_MODULATE));
		GL_ERROR_CHECK(glTexCoordPointer(2, GL_FLOAT, vertexStride, vertexPointer + drawInfo.attributes.st));
	#if UF_ENV_DREAMCAST
		GL_ERROR_CHECK(glDisable(GL_BLEND));
	#endif
	}
	
	if ( drawInfo.attributes.normal ) GL_ERROR_CHECK(glNormalPointer(GL_FLOAT, vertexStride, vertexPointer + drawInfo.attributes.normal));
	if ( drawInfo.attributes.color ) GL_ERROR_CHECK(glColorPointer(3, GL_FLOAT, vertexStride, vertexPointer + drawInfo.attributes.color));
	GL_ERROR_CHECK(glVertexPointer(3, GL_FLOAT, vertexStride, vertexPointer + drawInfo.attributes.position));

	// frustrum culling
	if ( ext::opengl::settings::experimental::frustrumCull ) {
		uf::stl::vector<uf::renderer::index_t> unculled;
		unculled.reserve(indices);
		for ( size_t currentIndex = 0; currentIndex < indices; currentIndex += 3 ) {
			const pod::Vector3f& positionA = *((pod::Vector3f*) ((vertexPointer + (indicesPointer[currentIndex+0] * vertexStride)) + drawInfo.attributes.position));
			const pod::Vector3f& positionB = *((pod::Vector3f*) ((vertexPointer + (indicesPointer[currentIndex+2] * vertexStride)) + drawInfo.attributes.position));
			const pod::Vector3f& positionC = *((pod::Vector3f*) ((vertexPointer + (indicesPointer[currentIndex+1] * vertexStride)) + drawInfo.attributes.position));

			bool inside = false;
			pod::Vector3f translated = uf::matrix::multiply<float>( projectionViewMatrix, positionA, 1, true );
			static constexpr pod::Vector3f min{-1.5,-1.5,-1.5};
			static constexpr pod::Vector3f max{ 1.5, 1.5, 1.5};
			if ( min.x <= translated.x && min.y <= translated.y && min.z <= translated.z && translated.x <= max.x && translated.y <= max.y && translated.z <= max.z ) inside = true;
		//	if ( min <= translated && translated <= max ) inside = true;
			else {
				pod::Vector3f translated = uf::matrix::multiply<float>( projectionViewMatrix, positionB, 1, true );
				if ( min.x <= translated.x && min.y <= translated.y && min.z <= translated.z && translated.x <= max.x && translated.y <= max.y && translated.z <= max.z ) inside = true;
			//	if ( min <= translated && translated <= max ) inside = true;
				else {
					pod::Vector3f translated = uf::matrix::multiply<float>( projectionViewMatrix, positionC, 1, true );
					if ( min.x <= translated.x && min.y <= translated.y && min.z <= translated.z && translated.x <= max.x && translated.y <= max.y && translated.z <= max.z ) inside = true;
				//	if ( min <= translated && translated <= max ) inside = true;
				}
			}
			if ( inside ) {
				unculled.emplace_back(indicesPointer[currentIndex+0]);
				unculled.emplace_back(indicesPointer[currentIndex+1]);
				unculled.emplace_back(indicesPointer[currentIndex+2]);
			}
		}
		GLenum indicesType = GL_UNSIGNED_INT;
		switch ( sizeof(uf::renderer::index_t) ) {
			case sizeof(uint32_t): indicesType = GL_UNSIGNED_INT; break;
			case sizeof(uint16_t): indicesType = GL_UNSIGNED_SHORT; break;
			case sizeof(uint8_t): indicesType = GL_UNSIGNED_BYTE; break;
		}
		GL_ERROR_CHECK(glDrawElements(GL_TRIANGLES, unculled.size(), indicesType, &unculled[0]));
	} else {
		GL_ERROR_CHECK(glDrawElements(GL_TRIANGLES, indices, indicesType, indicesPointer));
	}

	if ( drawInfo.auxTexture.image ) {
	#if UF_ENV_DREAMCAST
		GL_ERROR_CHECK(glEnable(GL_BLEND));
	#endif
		GL_ERROR_CHECK(glClientActiveTexture(GL_TEXTURE1));
		GL_ERROR_CHECK(glActiveTexture(GL_TEXTURE1));
		GL_ERROR_CHECK(glBindTexture(drawInfo.auxTexture.viewType, 0));
		GL_ERROR_CHECK(glDisable(drawInfo.auxTexture.viewType));
	}
	if ( drawInfo.texture.image ) {
		GL_ERROR_CHECK(glClientActiveTexture(GL_TEXTURE0));
		GL_ERROR_CHECK(glActiveTexture(GL_TEXTURE0));
		GL_ERROR_CHECK(glBindTexture(drawInfo.texture.viewType, 0));
		GL_ERROR_CHECK(glDisable(drawInfo.texture.viewType));
	}

	if ( drawInfo.attributes.normal ) GL_ERROR_CHECK(glDisableClientState(GL_NORMAL_ARRAY));
	if ( drawInfo.attributes.color ) GL_ERROR_CHECK(glDisableClientState(GL_COLOR_ARRAY));
	if ( drawInfo.attributes.uv ) GL_ERROR_CHECK(glDisableClientState(GL_TEXTURE_COORD_ARRAY));
	GL_ERROR_CHECK(glDisableClientState(GL_VERTEX_ARRAY));
}
#if 0
void ext::opengl::CommandBuffer::drawIndexed( const ext::opengl::CommandBuffer::InfoDraw& drawInfo, const ext::opengl::CommandBuffer::Info& vertexBufferInfo, const ext::opengl::CommandBuffer::Info& indexBufferInfo, const uf::stl::vector<InfoTexture*>& textureInfos ) {
	uf::Timer<long long> TIMER_TRACE;
	long long prevTime = 0;
	long long curTime = 0;
	UF_MSG_DEBUG("==== START RENDER ====");

	ext::opengl::Buffer::Descriptor vertexBuffer = {};
	ext::opengl::Buffer::Descriptor indexBuffer = {};

	switch ( vertexBufferInfo.type ) {
		case ext::opengl::enums::Command::BIND_BUFFER: {
			ext::opengl::CommandBuffer::InfoBuffer* info = (ext::opengl::CommandBuffer::InfoBuffer*) &vertexBufferInfo;
			vertexBuffer = info->descriptor;
		} break;
		case ext::opengl::enums::Command::BIND_GRAPHIC_BUFFER: {
			ext::opengl::CommandBuffer::InfoGraphicBuffer* info = (ext::opengl::CommandBuffer::InfoGraphicBuffer*) &vertexBufferInfo;
			vertexBuffer = info->graphic->buffers[info->bufferIndex].descriptor;
		} break;
	}
	switch ( indexBufferInfo.type ) {
		case ext::opengl::enums::Command::BIND_BUFFER: {
			ext::opengl::CommandBuffer::InfoBuffer* info = (ext::opengl::CommandBuffer::InfoBuffer*) &indexBufferInfo;
			indexBuffer = info->descriptor;
		} break;
		case ext::opengl::enums::Command::BIND_GRAPHIC_BUFFER: {
			ext::opengl::CommandBuffer::InfoGraphicBuffer* info = (ext::opengl::CommandBuffer::InfoGraphicBuffer*) &indexBufferInfo;
			indexBuffer = info->graphic->buffers[info->bufferIndex].descriptor;
		} break;
	}

	if ( !vertexBuffer.buffer || !indexBuffer.buffer ) return;

	size_t indices = drawInfo.length;
	size_t indicesStride = drawInfo.descriptor.attributes.index.size;
	size_t vertexStride = drawInfo.descriptor.attributes.vertex.size;
	size_t vertices = vertexBuffer.range / vertexStride;
	
	uf::renderer::VertexDescriptor 	vertexAttributePosition, 
									vertexAttributeNormal,
									vertexAttributeColor,
									vertexAttributeUv,
									vertexAttributeSt,
									vertexAttributeId;

	for ( auto& attribute : drawInfo.descriptor.attributes.descriptor ) {
		if ( attribute.name == "position" ) vertexAttributePosition = attribute;
		else if ( attribute.name == "normal" ) vertexAttributeNormal = attribute;
		else if ( attribute.name == "color" ) vertexAttributeColor = attribute;
		else if ( attribute.name == "uv" ) vertexAttributeUv = attribute;
		else if ( attribute.name == "st" ) vertexAttributeSt = attribute;
		else if ( attribute.name == "id" ) vertexAttributeId = attribute;
	}
	if ( vertexAttributePosition.name == "" ) return;

	if ( drawInfo.descriptor.cullMode == GL_NONE ) {
		GL_ERROR_CHECK(glDisable(GL_CULL_FACE));
	} else {
		GL_ERROR_CHECK(glEnable(GL_CULL_FACE));
		GL_ERROR_CHECK(glFrontFace(drawInfo.descriptor.frontFace));
		GL_ERROR_CHECK(glCullFace(drawInfo.descriptor.cullMode));
	}
#if !UF_USE_OPENGL_FIXED_FUNCTION
	// GPU-buffer based command dispatching
	{

	}
#else
	// CPU-buffer based command dispatching
	void* vertexPointer = (void*) ( device->getBuffer( vertexBuffer.buffer ) + vertexBuffer.offset );
	uf::renderer::index_t* indicesPointer = (uf::renderer::index_t*) ( device->getBuffer( indexBuffer.buffer ) + indexBuffer.offset );

	// vertices do not need to be transformed
	//if ( vertexAttributeId.name == "" )
#if !UF_USE_OPENGL_IMMEDIATE_MODE
	if ( vertexAttributeNormal.name != "" )	GL_ERROR_CHECK(glEnableClientState(GL_NORMAL_ARRAY));
	if ( vertexAttributeColor.name != "" )GL_ERROR_CHECK(glEnableClientState(GL_COLOR_ARRAY));
	if ( vertexAttributeUv.name != "" )	GL_ERROR_CHECK(glEnableClientState(GL_TEXTURE_COORD_ARRAY));
	GL_ERROR_CHECK(glEnableClientState(GL_VERTEX_ARRAY));

	GLenum indicesType = GL_UNSIGNED_INT;
	switch ( indicesStride ) {
		case sizeof(uint32_t): indicesType = GL_UNSIGNED_INT; break;
		case sizeof(uint16_t): indicesType = GL_UNSIGNED_SHORT; break;
		case sizeof(uint8_t): indicesType = GL_UNSIGNED_BYTE; break;
	}
	// no ID attribute found, fallback
	if ( vertexAttributeId.name == "" ) {
		if ( !textureInfos.empty() ) {
			InfoTexture* info = textureInfos.front();
			GL_ERROR_CHECK(glEnable(info->descriptor.viewType));
			GL_ERROR_CHECK(glBindTexture(info->descriptor.viewType, info->descriptor.image));
		}
		GL_BIND_POINTERS();
		GL_ERROR_CHECK(glDrawElements(GL_TRIANGLES, indices, indicesType, indicesPointer));
	} else {
		if ( textureInfos.empty() ) {
			GL_BIND_POINTERS();
			GL_ERROR_CHECK(glDrawElements(GL_TRIANGLES, indices, indicesType, indicesPointer));
		} else if ( textureInfos.size() == 1 ) {
			InfoTexture* info = textureInfos.front();
			GL_ERROR_CHECK(glEnable(info->descriptor.viewType));
			GL_ERROR_CHECK(glBindTexture(info->descriptor.viewType, info->descriptor.image));
			GL_BIND_POINTERS();
			GL_ERROR_CHECK(glDrawElements(GL_TRIANGLES, indices, indicesType, indicesPointer));
		} else {
			bool lightMapped = false;
			uf::stl::vector<uf::stl::vector<uf::renderer::index_t>> sorted( textureInfos.size() );
			for ( size_t currentIndex = 0; currentIndex < indices; ++currentIndex ) {
				auto index = indicesPointer[currentIndex];
				void* vertices = vertexPointer + (index * vertexStride);

				const pod::Vector2f& st = *((pod::Vector2f*) (vertices + vertexAttributeSt.offset));
				const pod::Vector<uf::graph::id_t,2>& id = *((pod::Vector<uf::graph::id_t,2>*) (vertices + vertexAttributeId.offset));

				if ( st > pod::Vector2f{0,0} ) lightMapped = true; 

				size_t textureId = id.y;
				if ( textureInfos.size() < textureId ) continue;
				sorted[textureId].emplace_back(index);
			}
			GLenum indicesType = GL_UNSIGNED_INT;
			switch ( sizeof(uf::renderer::index_t) ) {
				case sizeof(uint32_t): indicesType = GL_UNSIGNED_INT; break;
				case sizeof(uint16_t): indicesType = GL_UNSIGNED_SHORT; break;
				case sizeof(uint8_t): indicesType = GL_UNSIGNED_BYTE; break;
			}
			// assume if the last texture is unused, and we have ST's, it's lightmapped
			InfoTexture* textureLightmapInfo = NULL;
			if ( lightMapped && sorted.back().empty() ) {
				textureLightmapInfo = textureInfos.back();
				sorted.pop_back();
			} else vertexAttributeSt.name = "";
			for ( size_t textureId = 0; textureId < sorted.size(); ++textureId ) {
				auto& indices = sorted[textureId];
				if ( indices.empty() ) continue;
				InfoTexture* info = textureInfos[textureId];
				if ( textureLightmapInfo ) {
				#if UF_ENV_DREAMCAST
					GL_ERROR_CHECK(glDisable(GL_BLEND));
				#endif
					if ( vertexAttributeUv.name != "" ) {
						GL_ERROR_CHECK(glClientActiveTexture(GL_TEXTURE0));
						GL_ERROR_CHECK(glActiveTexture(GL_TEXTURE0));
						GL_ERROR_CHECK(glEnable(info->descriptor.viewType));
						GL_ERROR_CHECK(glEnableClientState(GL_TEXTURE_COORD_ARRAY));
						GL_ERROR_CHECK(glBindTexture(info->descriptor.viewType, info->descriptor.image));
						GL_ERROR_CHECK(glTexEnvi(GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_REPLACE));
						GL_ERROR_CHECK(glTexCoordPointer(2, GL_FLOAT, vertexStride, vertexPointer + vertexAttributeUv.offset));
					}

					if ( vertexAttributeSt.name != "" ) {
						GL_ERROR_CHECK(glClientActiveTexture(GL_TEXTURE1));
						GL_ERROR_CHECK(glActiveTexture(GL_TEXTURE1));
						GL_ERROR_CHECK(glEnable(textureLightmapInfo->descriptor.viewType));
						GL_ERROR_CHECK(glEnableClientState(GL_TEXTURE_COORD_ARRAY));
						GL_ERROR_CHECK(glBindTexture(textureLightmapInfo->descriptor.viewType, textureLightmapInfo->descriptor.image));
						GL_ERROR_CHECK(glTexEnvi(GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_MODULATE));
						GL_ERROR_CHECK(glTexCoordPointer(2, GL_FLOAT, vertexStride, vertexPointer + vertexAttributeSt.offset));
					}
					
					GL_ERROR_CHECK(glClientActiveTexture(GL_TEXTURE0));

					if ( vertexAttributeNormal.name != "" ) GL_ERROR_CHECK(glNormalPointer(GL_FLOAT, vertexStride, vertexPointer + vertexAttributeNormal.offset));
					if ( vertexAttributeColor.name != "" ) GL_ERROR_CHECK(glColorPointer(3, GL_FLOAT, vertexStride, vertexPointer + vertexAttributeColor.offset));
					GL_ERROR_CHECK(glVertexPointer(3, GL_FLOAT, vertexStride, vertexPointer + vertexAttributePosition.offset));
				
					GL_ERROR_CHECK(glDrawElements(GL_TRIANGLES, indices.size(), indicesType, &indices[0]));

					GL_ERROR_CHECK(glActiveTexture(GL_TEXTURE1));
					GL_ERROR_CHECK(glBindTexture(textureLightmapInfo->descriptor.viewType, 0));
					GL_ERROR_CHECK(glDisable(textureLightmapInfo->descriptor.viewType));

					GL_ERROR_CHECK(glActiveTexture(GL_TEXTURE0));
					GL_ERROR_CHECK(glBindTexture(info->descriptor.viewType, 0));
					GL_ERROR_CHECK(glDisable(info->descriptor.viewType));
				#if UF_ENV_DREAMCAST
					GL_ERROR_CHECK(glEnable(GL_BLEND));
				#endif
				} else {
					GL_ERROR_CHECK(glActiveTexture(GL_TEXTURE0));
					GL_ERROR_CHECK(glEnable(info->descriptor.viewType));
					GL_ERROR_CHECK(glBindTexture(info->descriptor.viewType, info->descriptor.image));
					GL_ERROR_CHECK(glTexEnvi(GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_REPLACE));
				
					GL_BIND_POINTERS();
					
					GL_ERROR_CHECK(glDrawElements(GL_TRIANGLES, indices.size(), indicesType, &indices[0]));

					GL_ERROR_CHECK(glActiveTexture(GL_TEXTURE0));
					GL_ERROR_CHECK(glBindTexture(info->descriptor.viewType, 0));
					GL_ERROR_CHECK(glDisable(info->descriptor.viewType));
				}
				curTime = TIMER_TRACE.elapsed().asMicroseconds();
				UF_MSG_DEBUG(curTime << " us\t" << (curTime - prevTime) << " us\t" << textureId);
				prevTime = curTime;
			}
		}
	}

    if ( vertexAttributeNormal.name != "" )	GL_ERROR_CHECK(glDisableClientState(GL_NORMAL_ARRAY));
	if ( vertexAttributeColor.name != "" )	GL_ERROR_CHECK(glDisableClientState(GL_COLOR_ARRAY));
	if ( vertexAttributeUv.name != "" )	GL_ERROR_CHECK(glDisableClientState(GL_TEXTURE_COORD_ARRAY));
	GL_ERROR_CHECK(glDisableClientState(GL_VERTEX_ARRAY));
#else
	// no ID attribute found, fallback
	if ( vertexAttributeId.name == "" ) {
		if ( !textureInfos.empty() ) {
			InfoTexture* info = textureInfos.front();
			GL_ERROR_CHECK(glEnable(info->descriptor.viewType));
			GL_ERROR_CHECK(glBindTexture(info->descriptor.viewType, info->descriptor.image));
		}
		GL_DRAW_ELEMENTS( indicesPointer, indices );
	} else {
		if ( textureInfos.empty() ) {
			GL_DRAW_ELEMENTS( indicesPointer, indices );
		} else if ( textureInfos.size() == 1 ) {
			InfoTexture* info = textureInfos.front();
			GL_ERROR_CHECK(glEnable(info->descriptor.viewType));
			GL_ERROR_CHECK(glBindTexture(info->descriptor.viewType, info->descriptor.image));
			GL_DRAW_ELEMENTS( indicesPointer, indices );
		} else {
			uf::stl::vector<uf::stl::vector<uf::renderer::index_t>> sorted( textureInfos.size() );
			for ( size_t currentIndex = 0; currentIndex < indices; ++currentIndex ) {
				uf::renderer::index_t index = indicesPointer[currentIndex];
				void* vertices = vertexPointer + (index * vertexStride);

				const pod::Vector<uf::graph::id_t,2>& id = *((pod::Vector<uf::graph::id_t,2>*) (vertices + vertexAttributeId.offset));
				size_t textureId = id.y;
				if ( textureInfos.size() < textureId ) continue;
				sorted[textureId].emplace_back(index);
			}
			for ( size_t textureId = 0; textureId < sorted.size(); ++textureId ) {
				auto& indices = sorted[textureId];
				if ( indices.empty() ) continue;
				InfoTexture* info = textureInfos[textureId];
				GL_ERROR_CHECK(glEnable(info->descriptor.viewType));
				GL_ERROR_CHECK(glBindTexture(info->descriptor.viewType, info->descriptor.image));
				GL_DRAW_ELEMENTS( (&indices[0]) , (indices.size()) );
			}
		}
	}
#endif
#if 0
	GL_ERROR_CHECK(glBegin(GL_TRIANGLES)); {
		for ( size_t i = 0; i < indices; ++i ) {
			size_t index = indicesPointer[i];
			void* vertex = vertexPointer + (index * vertexStride);
			if ( vertexAttributeNormal.name != "" ) {
				float* normal = (float*) (vertex + vertexAttributeNormal.offset);
				GL_ERROR_CHECK(glNormal3f(normal[0], normal[1], normal[2]));
			}
			if ( vertexAttributeColor.name != "" ) {
				float* color = (float*) (vertex + vertexAttributeColor.offset);
				GL_ERROR_CHECK(glColor3f(color[0], color[1], color[2]));
			}
			if ( vertexAttributeUv.name != "" ) {
				float* uv = (float*) (vertex + vertexAttributeUv.offset);
				GL_ERROR_CHECK(glTexCoord2f(uv[0], uv[1]));
			}
			{
				float* position = (float*) (vertex + vertexAttributePosition.offset);
				GL_ERROR_CHECK(glVertex3f(position[0], position[1], position[2]));
			}
		}
	} GL_ERROR_CHECK(glEnd());
#endif
#endif
}
#endif

#endif