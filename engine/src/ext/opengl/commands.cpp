#if UF_USE_OPENGL

#include <uf/ext/opengl/opengl.h>
#include <uf/ext/opengl/commands.h>
#include <uf/ext/opengl/graphic.h>

#include <vector>
#include <bitset>

#include <uf/utils/graphic/mesh.h>
#include <uf/ext/gltf/mesh.h>

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
			size_t index = indicesPointer[i];\
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
	if ( state != 1 ) {
		std::cout << "ERROR: COMMAND BUFFER " << this << " NOT READY FOR RECORDING" << std::endl;
		return;
	}

	if ( infos.empty() ) {
		if ( VERBOSE ) std::cout << "==== ["<<this<<"] COMMAND BUFFER RECORDING START ==== " << std::endl;
	}

	switch ( header.type ) {
		case ext::opengl::enums::Command::CLEAR: {
			InfoClear* info = (InfoClear*) &header;
			info->next = NULL;
			auto& userdata = infos.emplace_back();
			userdata.autoDestruct = false;
			userdata.create<InfoClear>( *info );
			info = &userdata.get<InfoClear>();
			info->type = enums::Command::CLEAR;
			if ( VERBOSE ) std::cout << "[" << infos.size() << "] ["<<info<<"] COMMAND BUFFER " << this << " | CLEAR: " << uf::vector::toString(info->color) << " | " << info->depth << "\n";
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
			if ( VERBOSE ) std::cout << "[" << infos.size() << "] ["<<info<<"] COMMAND BUFFER " << this << " | VIEWPORT: " << uf::vector::toString(info->corner) << " | " << uf::vector::toString(info->size) << "\n";
		} break;
		case ext::opengl::enums::Command::VARIANT: {
			InfoVariant* info = (InfoVariant*) &header;
			info->next = NULL;
			auto& userdata = infos.emplace_back();
			userdata.autoDestruct = false;
			userdata.create<InfoVariant>( *info );
			info = &userdata.get<InfoVariant>();
			info->type = enums::Command::VARIANT;
			if ( VERBOSE ) std::cout << "[" << infos.size() << "] ["<<info<<"] COMMAND BUFFER " << this << " | VARIANT: " << "\n";
		} break;
		case ext::opengl::enums::Command::BIND_BUFFER: {
			InfoBuffer* info = (InfoBuffer*) &header;
			info->next = NULL;
			auto& userdata = infos.emplace_back();
			userdata.autoDestruct = false;
			userdata.create<InfoBuffer>( *info );
			info = &userdata.get<InfoBuffer>();
			info->type = enums::Command::BIND_BUFFER;
			if ( VERBOSE ) std::cout << "[" << infos.size() << "] ["<<info<<"] COMMAND BUFFER " << this << " | BIND_BUFFER: " << info->descriptor.buffer << "\n";
		} break;
		case ext::opengl::enums::Command::BIND_GRAPHIC_BUFFER: {
			InfoGraphicBuffer* info = (InfoGraphicBuffer*) &header;
			info->next = NULL;
			auto& userdata = infos.emplace_back();
			userdata.autoDestruct = false;
			userdata.create<InfoGraphicBuffer>( *info );
			info = &userdata.get<InfoGraphicBuffer>();
			info->type = enums::Command::BIND_GRAPHIC_BUFFER;
			if ( VERBOSE ) std::cout << "[" << infos.size() << "] ["<<info<<"] COMMAND BUFFER " << this << " | BIND_GRAPHIC_BUFFER: " << info->graphic << "[" << info->bufferIndex << "] = " << info->graphic->buffers[info->bufferIndex].descriptor.buffer << "\n";
		} break;
		case ext::opengl::enums::Command::BIND_TEXTURE: {
			InfoTexture* info = (InfoTexture*) &header;
			info->next = NULL;
			auto& userdata = infos.emplace_back();
			userdata.autoDestruct = false;
			userdata.create<InfoTexture>( *info );
			info = &userdata.get<InfoTexture>();
			info->type = enums::Command::BIND_TEXTURE;
			if ( VERBOSE ) std::cout << "[" << infos.size() << "] ["<<info<<"] COMMAND BUFFER " << this << " | BIND_TEXTURE: " << info->descriptor.image << "\n";
		} break;
		case ext::opengl::enums::Command::BIND_PIPELINE: {
			InfoPipeline* info = (InfoPipeline*) &header;
			info->next = NULL;
			auto& userdata = infos.emplace_back();
			userdata.autoDestruct = false;
			userdata.create<InfoPipeline>( *info );
			info = &userdata.get<InfoPipeline>();
			info->type = enums::Command::BIND_PIPELINE;
			if ( VERBOSE ) std::cout << "[" << infos.size() << "] ["<<info<<"] COMMAND BUFFER " << this << " | BIND PIPELINE: " << info->descriptor.pipeline << " | " << info->descriptor.vertexArray << "\n";
		} break;
		case ext::opengl::enums::Command::DRAW: {
			InfoDraw* info = (InfoDraw*) &header;
			info->next = NULL;
			auto& userdata = infos.emplace_back();
			userdata.autoDestruct = false;
			userdata.create<InfoDraw>( *info );
			info = &userdata.get<InfoDraw>();
			info->type = enums::Command::DRAW;
			if ( VERBOSE ) std::cout << "[" << infos.size() << "] ["<<info<<"] COMMAND BUFFER " << this << " | COUNT: " << info->length << "\n";
		} break;
		case ext::opengl::enums::Command::GENERATE_TEXTURE: {
			InfoGenerateTexture* info = (InfoGenerateTexture*) &header;
			auto& userdata = infos.emplace_back();
			userdata.autoDestruct = false;
			userdata.create( *info );
			info = &userdata.get<InfoGenerateTexture>();
			info->type = enums::Command::GENERATE_TEXTURE;
			if ( VERBOSE ) std::cout << "[" << infos.size() << "] ["<<info<<"] COMMAND BUFFER " << this << " | GENERATE_TEXTURE: " << info->descriptor.image << " | " << info->data << "[" << info->size << "]" << "\n";
		} break;
		default: {
			if ( VERBOSE ) std::cout << "UNKNOWN COMMAND TYPE: " << header.type << std::endl;
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
		std::cout << "MAX COMBINED TEXTURE UNITS: " << maxTextures << std::endl;
	}
#endif
	CommandBuffer::Info* vertexBufferInfo = GL_NULL_HANDLE;
	CommandBuffer::Info* indexBufferInfo = GL_NULL_HANDLE;
	std::vector<CommandBuffer::InfoTexture*> textureInfos;

	if ( state == 2 && VERBOSE_SUBMIT ) std::cout << "==== ["<<this<<"] COMMAND BUFFER SUBMIT START ====\n";
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
			/*
				if ( textureInfos.size() < maxTextures ) {
				#if !UF_ENV_DREAMCAST
					GL_ERROR_CHECK(glActiveTexture(GL_TEXTURE0 + textureInfos.size()));
				#endif
					GL_ERROR_CHECK(glEnable(info->descriptor.viewType));
					GL_ERROR_CHECK(glBindTexture(info->descriptor.viewType, info->descriptor.image));
					textureInfos.emplace_back(info);
				}
			*/
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
			/*
				#if !UF_ENV_DREAMCAST
					GL_ERROR_CHECK(glActiveTexture(GL_TEXTURE0));
				#endif
			*/
					if ( indexBufferInfo ) drawIndexed( *info, *vertexBufferInfo, *indexBufferInfo, textureInfos );
					else draw( *info, *vertexBufferInfo, textureInfos );
				}
			/*
				while ( !textureInfos.empty() ) {
					auto& info = textureInfos.back();
				#if !UF_ENV_DREAMCAST
					GL_ERROR_CHECK(glActiveTexture(GL_TEXTURE0 + textureInfos.size() - 1));
				#endif
					GL_ERROR_CHECK(glDisable(info->descriptor.viewType));
					GL_ERROR_CHECK(glBindTexture(info->descriptor.viewType, 0));
					textureInfos.pop_back();
				}
			*/
				textureInfos.clear();
				vertexBufferInfo = NULL;
				indexBufferInfo = NULL;
			} break;
		/*
			case ext::opengl::enums::Command::GENERATE_TEXTURE: {
				InfoGenerateTexture* info = (InfoGenerateTexture*) header;
				if ( state == 2 && VERBOSE_SUBMIT ) std::cout << "["<<info<<"] GENERATING TEXTURE | " << info->descriptor.image << " | " << info->data << "\n";

				GLenum format = GL_RGBA;
				GLenum type = GL_UNSIGNED_BYTE;
				switch ( info->descriptor.format ) {
				#if !UF_ENV_DREAMCAST
					case enums::Format::R8_UNORM: format = GL_RED; break;
					case enums::Format::R8G8_UNORM: format = GL_RG; break;
				#endif
					case enums::Format::R8G8B8_UNORM: format = GL_RGB; break;
					case enums::Format::R8G8B8A8_UNORM: format = GL_RGBA; break;
				}
				GL_ERROR_CHECK(glBindTexture(info->descriptor.viewType, info->descriptor.image));
				switch ( info->descriptor.viewType ) {
					case enums::Image::VIEW_TYPE_2D: GL_ERROR_CHECK(glTexImage2D(info->descriptor.viewType, 0, format, info->descriptor.width, info->descriptor.height, 0, format, type, info->data)); break;
				#if !UF_ENV_DREAMCAST
					case enums::Image::VIEW_TYPE_3D: GL_ERROR_CHECK(glTexImage3D(info->descriptor.viewType, 0, format, info->descriptor.width, info->descriptor.height, info->descriptor.depth, 0, format, type, info->data)); break;
				#endif
				}
				GL_ERROR_CHECK(glBindTexture(info->descriptor.viewType, 0));
			} break;
		*/
			default: {
				if ( state == 2 && VERBOSE_SUBMIT ) {
					std::cout << "["<<header<<"] UNKNOWN COMMAND TYPE: " << header->type << ": " << info.data().len << std::endl;;
				/*
					// dump contents
					std::cout << "["<<header<<"] UNKNOWN COMMAND TYPE: " << header->type << ": " << std::hex;
					uint8_t* start = (uint8_t*) header;
					for ( size_t i = 0; i < info.data().len; ++i ) {
						std::cout << (int) start[i] << " ";
					}
					std::cout << std::dec << std::endl;
				*/
				}
			} break;
		}
	}
	if ( state == 2 && VERBOSE_SUBMIT ) std::cout << "==== ["<<this<<"] COMMAND BUFFER SUBMIT END ==== " << std::endl;
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

void ext::opengl::CommandBuffer::bindUniform( const ext::opengl::Buffer::Descriptor& descriptor ) {
#if UF_USE_OPENGL_FIXED_FUNCTION
	pod::Uniform* uniform = (pod::Uniform*) (device->getBuffer( descriptor.buffer ) + descriptor.offset);

	GL_ERROR_CHECK(glMatrixMode(GL_PROJECTION));
	GL_ERROR_CHECK(glLoadMatrixf( &uniform->projection[0] ));

	GL_ERROR_CHECK(glMatrixMode(GL_MODELVIEW));
	GL_ERROR_CHECK(glLoadMatrixf( &uniform->modelView[0] ));
#endif
}
void ext::opengl::CommandBuffer::draw( const ext::opengl::CommandBuffer::InfoDraw& drawInfo, const ext::opengl::CommandBuffer::Info& vertexBufferInfo, const std::vector<InfoTexture*>& textureInfos ) {
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

	size_t vertexStride = drawInfo.descriptor.geometry.attributes.vertex.size;
	size_t vertices = vertexBuffer.range / vertexStride;
	
	uf::renderer::VertexDescriptor 	vertexAttributePosition, 
									vertexAttributeNormal,
									vertexAttributeColor,
									vertexAttributeUv,
									vertexAttributeId;

	for ( auto& attribute : drawInfo.descriptor.geometry.attributes.descriptor ) {
		if ( attribute.name == "position" ) vertexAttributePosition = attribute;
		else if ( attribute.name == "normal" ) vertexAttributeNormal = attribute;
		else if ( attribute.name == "color" ) vertexAttributeColor = attribute;
		else if ( attribute.name == "uv" ) vertexAttributeUv = attribute;
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
			std::vector<std::vector<uf::renderer::index_t>> sorted( textureInfos.size() );
			for ( uf::renderer::index_t index = 0; index < vertices; ++index ) {
				uint8_t* vertex = vertexPointer + (index * vertexStride);

				const pod::Vector2ui& id = *((pod::Vector2ui*) (vertex + vertexAttributeId.offset));
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
			std::vector<std::vector<uf::renderer::index_t>> sorted( textureInfos.size() );
			for ( uf::renderer::index_t index = 0; index < vertices; ++index ) {
				uint8_t* vertex = vertexPointer + (index * vertexStride);

				const pod::Vector2ui& id = *((pod::Vector2ui*) (vertex + vertexAttributeId.offset));
				size_t textureId = id.y;
				if ( textureInfos.size() < textureId ) continue;
				sorted[textureId].emplace_back(index);
			}
			for ( size_t textureId = 0; textureId < sorted.size(); ++textureId ) {
				auto& indices = sorted[textureId];
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
void ext::opengl::CommandBuffer::drawIndexed( const ext::opengl::CommandBuffer::InfoDraw& drawInfo, const ext::opengl::CommandBuffer::Info& vertexBufferInfo, const ext::opengl::CommandBuffer::Info& indexBufferInfo, const std::vector<InfoTexture*>& textureInfos ) {
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
	size_t indicesStride = drawInfo.descriptor.geometry.attributes.index.size;
	size_t vertexStride = drawInfo.descriptor.geometry.attributes.vertex.size;
	size_t vertices = vertexBuffer.range / vertexStride;
	
	uf::renderer::VertexDescriptor 	vertexAttributePosition, 
									vertexAttributeNormal,
									vertexAttributeColor,
									vertexAttributeUv,
									vertexAttributeId;

	for ( auto& attribute : drawInfo.descriptor.geometry.attributes.descriptor ) {
		if ( attribute.name == "position" ) vertexAttributePosition = attribute;
		else if ( attribute.name == "normal" ) vertexAttributeNormal = attribute;
		else if ( attribute.name == "color" ) vertexAttributeColor = attribute;
		else if ( attribute.name == "uv" ) vertexAttributeUv = attribute;
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
			std::vector<std::vector<uf::renderer::index_t>> sorted( textureInfos.size() );
			for ( size_t currentIndex = 0; currentIndex < indices; ++currentIndex ) {
				auto index = indicesPointer[currentIndex];
				void* vertices = vertexPointer + (index * vertexStride);

				const pod::Vector2ui& id = *((pod::Vector2ui*) (vertices + vertexAttributeId.offset));
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
			std::vector<std::vector<uf::renderer::index_t>> sorted( textureInfos.size() );
			for ( size_t currentIndex = 0; currentIndex < indices; ++currentIndex ) {
				uf::renderer::index_t index = indicesPointer[currentIndex];
				void* vertices = vertexPointer + (index * vertexStride);

				const pod::Vector2ui& id = *((pod::Vector2ui*) (vertices + vertexAttributeId.offset));
				size_t textureId = id.y;
				if ( textureInfos.size() < textureId ) continue;
				sorted[textureId].emplace_back(index);
			}
			for ( size_t textureId = 0; textureId < sorted.size(); ++textureId ) {
				auto& indices = sorted[textureId];
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