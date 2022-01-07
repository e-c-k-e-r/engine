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

namespace {
	size_t culled = 0;
}
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
	//UF_TIMER_MULTITRACE_START("Starting command buffer submission: " << this);
	for ( auto& info : infos ) {
		CommandBuffer::Info* header = (CommandBuffer::Info*) (void*) info;
		switch ( header->type ) {
			case ext::opengl::enums::Command::CLEAR: {
				InfoClear* info = (InfoClear*) header;
				GL_ERROR_CHECK(glClearColor(info->color[0], info->color[1], info->color[2], info->color[3]));
				GL_ERROR_CHECK(glClearDepth(info->depth));
				GL_ERROR_CHECK(glClear(info->bits));
			//	GL_ERROR_CHECK(glLightModelfv(GL_LIGHT_MODEL_AMBIENT, &info->color[0]));
				//UF_TIMER_MULTITRACE("Command::CLEAR");
			} break;
			case ext::opengl::enums::Command::VIEWPORT: {
				InfoViewport* info = (InfoViewport*) header;
				GL_ERROR_CHECK(glViewport(info->corner[0], info->corner[1], info->size[0], info->size[1]));
				//UF_TIMER_MULTITRACE("Command::VIEWPORT");
			} break;
			case ext::opengl::enums::Command::VARIANT: {
				InfoVariant* info = (InfoVariant*) header;
				if ( info->lambda ) info->lambda();
				//UF_TIMER_MULTITRACE("Command::VARIANT");
			} break;
			case ext::opengl::enums::Command::DRAW: {
				InfoDraw* info = (InfoDraw*) header;
				drawIndexed( *info );
				//UF_TIMER_MULTITRACE("Command::DRAW");
			} break;
			default: {
			} break;
		}
	}
	if ( ::culled ) {
	//	UF_MSG_DEBUG("Culled " << ::culled << " meshes.");
		::culled = 0;
	}
	//UF_TIMER_MULTITRACE_END("Finished command buffer submission: " << this);
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

namespace {
	bool inside( const pod::Instance& instance, const pod::Matrix4f& mat ) {
		bool visible = false;
		#if 0
			pod::Vector4f corners[8] = {
				pod::Vector4f{ instance.bounds.min.x, instance.bounds.min.y, instance.bounds.min.z, 1.0f },
				pod::Vector4f{ instance.bounds.max.x, instance.bounds.min.y, instance.bounds.min.z, 1.0f },
				pod::Vector4f{ instance.bounds.max.x, instance.bounds.max.y, instance.bounds.min.z, 1.0f },
				pod::Vector4f{ instance.bounds.min.x, instance.bounds.max.y, instance.bounds.min.z, 1.0f },

				pod::Vector4f{ instance.bounds.min.x, instance.bounds.min.y, instance.bounds.max.z, 1.0f },
				pod::Vector4f{ instance.bounds.max.x, instance.bounds.min.y, instance.bounds.max.z, 1.0f },
				pod::Vector4f{ instance.bounds.max.x, instance.bounds.max.y, instance.bounds.max.z, 1.0f },
				pod::Vector4f{ instance.bounds.min.x, instance.bounds.max.y, instance.bounds.max.z, 1.0f },
			};
			#pragma unroll
			for ( uint p = 0; p < 8; ++p ) {
				pod::Vector4f t = uf::matrix::multiply( mat, corners[p] );
				float w = t.w * 1.25f;
				if ( -w <= t.x && t.x <= w && -w <= t.y && t.y <= w && -w <= t.z && t.z <= w ) return true;
			}
		#else
			pod::Vector4f planes[6]; {
				#pragma unroll
				for ( auto i = 0; i < 3; ++i )
				#pragma unroll
				for ( auto j = 0; j < 2; ++j) {
					float x = mat[4*0+3] + (j == 0 ? mat[4*0+i] : -mat[4*0+i]);
					float y = mat[4*1+3] + (j == 0 ? mat[4*1+i] : -mat[4*1+i]);
					float z = mat[4*2+3] + (j == 0 ? mat[4*2+i] : -mat[4*2+i]);
					float w = mat[4*3+3] + (j == 0 ? mat[4*3+i] : -mat[4*3+i]);
					float length = 1.0f / sqrt( x * x + y * y + z * z );
					
					planes[i*2+j] = pod::Vector4f{ x * length, y * length, z * length, w * length };
				}
			}
		#if 1
			#pragma unroll
			for ( auto p = 0; p < 6; ++p ) {
				float d = std::max(instance.bounds.min.x * planes[p].x, instance.bounds.max.x * planes[p].x)
						+ std::max(instance.bounds.min.y * planes[p].y, instance.bounds.max.y * planes[p].y)
						+ std::max(instance.bounds.min.z * planes[p].z, instance.bounds.max.z * planes[p].z);
				if ( d > -planes[p].w ) return true;
			}
		#else
			pod::Vector4f corners[8] = {
				pod::Vector4f{ instance.bounds.min.x, instance.bounds.min.y, instance.bounds.min.z, 1.0f },
				pod::Vector4f{ instance.bounds.max.x, instance.bounds.min.y, instance.bounds.min.z, 1.0f },
				pod::Vector4f{ instance.bounds.max.x, instance.bounds.max.y, instance.bounds.min.z, 1.0f },
				pod::Vector4f{ instance.bounds.min.x, instance.bounds.max.y, instance.bounds.min.z, 1.0f },

				pod::Vector4f{ instance.bounds.min.x, instance.bounds.min.y, instance.bounds.max.z, 1.0f },
				pod::Vector4f{ instance.bounds.max.x, instance.bounds.min.y, instance.bounds.max.z, 1.0f },
				pod::Vector4f{ instance.bounds.max.x, instance.bounds.max.y, instance.bounds.max.z, 1.0f },
				pod::Vector4f{ instance.bounds.min.x, instance.bounds.max.y, instance.bounds.max.z, 1.0f },
			};
			#pragma unroll
			for ( uint p = 0; p < 8; ++p ) corners[p] = uf::matrix::multiply( mat, corners[p] );
			#pragma unroll
			for ( uint p = 0; p < 6; ++p ) {
				#pragma unroll
				for ( uint q = 0; q < 8; ++q ) if ( uf::vector::dot( corners[q], planes[p] ) > 0 ) return true;
				return false;
			}
		#endif
		#endif
		return visible;
	}
}

void ext::opengl::CommandBuffer::drawIndexed( const ext::opengl::CommandBuffer::InfoDraw& drawInfo ) {
	pod::Matrix4f modelView = uf::matrix::identity(), projection = uf::matrix::identity();
	if ( drawInfo.matrices.model && drawInfo.matrices.view ) modelView = uf::matrix::multiply( *drawInfo.matrices.view, *drawInfo.matrices.model );
	else if ( drawInfo.matrices.model ) modelView = *drawInfo.matrices.model;
	else if ( drawInfo.matrices.view ) modelView = *drawInfo.matrices.view;
	if ( drawInfo.matrices.projection ) projection = *drawInfo.matrices.projection;

	if ( drawInfo.attributes.indirect.pointer && drawInfo.attributes.indirect.length == sizeof(pod::DrawCommand) ) {
		pod::DrawCommand& drawCommand = *(pod::DrawCommand*) drawInfo.attributes.indirect.pointer;
		if ( ext::opengl::settings::experimental::culling && drawInfo.attributes.instance.pointer && drawInfo.attributes.instance.length == sizeof(pod::Instance) ) {
			pod::Instance& instance = *(pod::Instance*) drawInfo.attributes.instance.pointer;
			pod::Matrix4f mat = (*drawInfo.matrices.projection) * (*drawInfo.matrices.view) * (*drawInfo.matrices.model);
		//	pod::Matrix4f mat = (*drawInfo.matrices.projection) * (*drawInfo.matrices.view) * (instance.model);
		//	pod::Matrix4f mat = uf::matrix::multiply( projection, modelView );
			
			bool visible = inside( instance, mat );
			drawCommand.instances = visible ? 1 : 0;
			if ( !visible ) ++::culled;
		}
		if ( drawCommand.instances == 0 ) return;
	}

	GL_ERROR_CHECK(glMatrixMode(GL_MODELVIEW));
	GL_ERROR_CHECK(glLoadMatrixf( &modelView[0] ));

	GL_ERROR_CHECK(glMatrixMode(GL_PROJECTION));
	GL_ERROR_CHECK(glLoadMatrixf( &projection[0] ));

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
	GL_ERROR_CHECK(glEnableClientState(GL_VERTEX_ARRAY));

	GLenum indicesType = GL_UNSIGNED_INT;
	switch ( drawInfo.attributes.index.stride ) {
		case sizeof(uint32_t): indicesType = GL_UNSIGNED_INT; break;
		case sizeof(uint16_t): indicesType = GL_UNSIGNED_SHORT; break;
		case sizeof(uint8_t): indicesType = GL_UNSIGNED_BYTE; break;
	}

	GLboolean blending = glIsEnabled(GL_BLEND);

	if ( drawInfo.textures.primary.image && drawInfo.attributes.uv.pointer ) {
	//	static GLuint previous = 0;
	//	if ( previous != drawInfo.textures.primary.image ) previous = drawInfo.textures.primary.image;
		
		GL_ERROR_CHECK(glEnableClientState(GL_TEXTURE_COORD_ARRAY));

		GL_ERROR_CHECK(glClientActiveTexture(GL_TEXTURE0));
		GL_ERROR_CHECK(glActiveTexture(GL_TEXTURE0));
		GL_ERROR_CHECK(glEnable(drawInfo.textures.primary.viewType));
		GL_ERROR_CHECK(glEnableClientState(GL_TEXTURE_COORD_ARRAY));
		GL_ERROR_CHECK(glBindTexture(drawInfo.textures.primary.viewType, drawInfo.textures.primary.image));
		GL_ERROR_CHECK(glTexEnvi(GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_REPLACE));
		GL_ERROR_CHECK(glTexCoordPointer(2, GL_FLOAT, drawInfo.attributes.uv.stride, drawInfo.attributes.uv.pointer));
		
		if ( drawInfo.textures.secondary.image && drawInfo.attributes.st.pointer ) {
		//	static GLuint previous = 0;
		//	if ( previous != drawInfo.textures.secondary.image ) previous = drawInfo.textures.secondary.image;
			
			GL_ERROR_CHECK(glClientActiveTexture(GL_TEXTURE1));
			GL_ERROR_CHECK(glActiveTexture(GL_TEXTURE1));
			GL_ERROR_CHECK(glEnable(drawInfo.textures.secondary.viewType));
			GL_ERROR_CHECK(glEnableClientState(GL_TEXTURE_COORD_ARRAY));
			GL_ERROR_CHECK(glBindTexture(drawInfo.textures.secondary.viewType, drawInfo.textures.secondary.image));
			GL_ERROR_CHECK(glTexEnvi(GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_MODULATE));
		#if UF_ENV_DREAMCAST
			if ( blending ) GL_ERROR_CHECK(glDisable(GL_BLEND));
		#endif
			GL_ERROR_CHECK(glTexCoordPointer(2, GL_FLOAT, drawInfo.attributes.st.stride, drawInfo.attributes.st.pointer));
		}
	}

	
	if ( drawInfo.attributes.normal.pointer ) GL_ERROR_CHECK(glNormalPointer(GL_FLOAT, drawInfo.attributes.normal.stride, drawInfo.attributes.normal.pointer));
	if ( drawInfo.attributes.color.pointer ) GL_ERROR_CHECK(glColorPointer(4, GL_UNSIGNED_BYTE, drawInfo.attributes.color.stride, drawInfo.attributes.color.pointer));
	GL_ERROR_CHECK(glVertexPointer(3, GL_FLOAT, drawInfo.attributes.position.stride, drawInfo.attributes.position.pointer));
	GL_ERROR_CHECK(glDrawElements(GL_TRIANGLES, drawInfo.attributes.index.length / drawInfo.attributes.index.stride, indicesType, drawInfo.attributes.index.pointer));

	if ( drawInfo.textures.secondary.image ) {
	#if UF_ENV_DREAMCAST
		if ( blending ) GL_ERROR_CHECK(glEnable(GL_BLEND));
	#endif
	}

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