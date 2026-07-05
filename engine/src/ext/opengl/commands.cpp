#if UF_USE_OPENGL

#include <uf/ext/opengl/opengl.h>
#include <uf/ext/opengl/commands.h>
#include <uf/ext/opengl/graphic.h>

#include <uf/utils/memory/vector.h>
#include <bitset>

#include <uf/utils/mesh/mesh.h>
#include <uf/engine/graph/graph.h>

namespace {
	struct State {
		bool initialized = false;

		// matricies
		const pod::Matrix4f* lastProjPtr = nullptr;
		const pod::Matrix4f* lastViewPtr = nullptr;
		const pod::Matrix4f* lastModelPtr = nullptr;

		pod::Matrix4f projection = uf::matrix::identity();
		pod::Matrix4f modelView = uf::matrix::identity();
		pod::Matrix4f ndc = uf::matrix::identity();

		bool modelViewDirty = true;
		bool projectionDirty = true;

		// blending
		bool blendEnabled = false;
		bool alphaTestEnabled = false;
		float alphaCutoff = -1.0f;
		
		pod::Vector4f color = {1,1,1,1};
		float lineWidth = 1.0f;

		// culling
		bool cullEnabled = false;
		GLenum cullFace = GL_BACK;
		GLenum frontFace = GL_CCW;

		// depth
		bool depthTestEnabled = false;
		bool depthWriteEnabled = true;

		// textures
		GLuint boundTexture0 = 0;
		GLuint boundTexture1 = 0;
		bool tex0Enabled = false;
		bool tex1Enabled = false;
		GLenum tex0Target = GL_TEXTURE_2D;
		GLenum tex1Target = GL_TEXTURE_2D;

		// client states
		bool normalArrayEnabled = false;
		bool colorArrayEnabled = false;
		bool texCoord0ArrayEnabled = false;
		bool texCoord1ArrayEnabled = false;

		void invalidate() {
			initialized = false;

			lastProjPtr = nullptr;
			lastViewPtr = nullptr;
			lastModelPtr = nullptr;
			modelViewDirty = true;
			projectionDirty = true;

			blendEnabled = false;
			alphaTestEnabled = false;
			alphaCutoff = -1.0f;
			
			color = {1,1,1,1};
			lineWidth = 1.0f;

			cullEnabled = false;
			cullFace = GL_BACK;
			frontFace = GL_CCW;

			depthTestEnabled = false;
			depthWriteEnabled = true;

			boundTexture0 = 0;
			boundTexture1 = 0;
			tex0Enabled = false;
			tex1Enabled = false;
			tex0Target = GL_TEXTURE_2D;
			tex1Target = GL_TEXTURE_2D;

			normalArrayEnabled = false;
			colorArrayEnabled = false;
			texCoord0ArrayEnabled = false;
			texCoord1ArrayEnabled = false;
		}

		void update( const ext::opengl::CommandBuffer::InfoDraw::Matrices& matrices ) {
			bool projectionChanged = lastProjPtr != matrices.projection;
			bool viewChanged = lastViewPtr != matrices.view;
			bool modelChanged = lastModelPtr != matrices.model;

			if ( projectionChanged || viewChanged || modelChanged ) {
				if ( projectionChanged ) {
					projectionDirty = true;
					projection = matrices.projection ? *matrices.projection : uf::matrix::identity();
					lastProjPtr = matrices.projection;
				}

				if ( viewChanged || modelChanged ) {
					modelViewDirty = true;

					pod::Matrix4f view = matrices.view ? *matrices.view : uf::matrix::identity();
					pod::Matrix4f model = matrices.model ? *matrices.model : uf::matrix::identity();

					modelView = view * model;

					lastViewPtr = matrices.view;
					lastModelPtr = matrices.model;
				}

				ndc = projection * modelView;
			}
		}
	} shadowState;

	bool inside( const pod::Instance& instance, const pod::Matrix4f& mat ) {
		#pragma unroll
		for ( auto p = 0; p < 4; ++p ) {
			int i = p / 2;
			int j = p % 2;
			int s = 1 - (j * 2);

			pod::Vector3f normal = {
				mat(3,0) + s * mat(i,0),
				mat(3,1) + s * mat(i,1),
				mat(3,2) + s * mat(i,2)
			};
			float w = mat(3,3) + s * mat(i,3);
			float r = uf::vector::dot( instance.bounds.extent, uf::vector::abs( normal ) );
			float d = uf::vector::dot( instance.bounds.center, normal ) + w;
			if ( d < -r ) return false;
		}

		return true;
	}
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
		#if UF_COMMAND_BUFFER_USERDATA
			auto& userdata = infos.emplace_back();
			userdata.create<InfoClear>( *info );
			info = &userdata.get<InfoClear>();
		#else
			info = new InfoClear(*info);
			infos.emplace_back(info);
		#endif
			info->type = enums::Command::CLEAR;
			info->next = NULL;
		} break;
		case ext::opengl::enums::Command::VIEWPORT: {
			InfoViewport* info = (InfoViewport*) &header;
			if ( info->size.x == 0 ) info->size.x = ext::opengl::settings::width;
			if ( info->size.y == 0 ) info->size.y = ext::opengl::settings::height;
		#if UF_COMMAND_BUFFER_USERDATA
			auto& userdata = infos.emplace_back();
			userdata.create<InfoViewport>( *info );
			info = &userdata.get<InfoViewport>();
		#else
			info = new InfoViewport(*info);
			infos.emplace_back(info);
		#endif
			info->type = enums::Command::VIEWPORT;
			info->next = NULL;
		} break;
		case ext::opengl::enums::Command::VARIANT: {
			InfoVariant* info = (InfoVariant*) &header;
		#if UF_COMMAND_BUFFER_USERDATA
			auto& userdata = infos.emplace_back();
			userdata.create<InfoVariant>( *info );
			info = &userdata.get<InfoVariant>();
		#else
			info = new InfoVariant(*info);
			infos.emplace_back(info);
		#endif
			info->type = enums::Command::VARIANT;
			info->next = NULL;
		} break;
		case ext::opengl::enums::Command::DRAW: {
			InfoDraw* info = (InfoDraw*) &header;
		#if UF_COMMAND_BUFFER_USERDATA
			auto& userdata = infos.emplace_back();
			userdata.create<InfoDraw>( *info );
			info = &userdata.get<InfoDraw>();
		#else
			info = new InfoDraw(*info);
			infos.emplace_back(info);
		#endif
			info->type = enums::Command::DRAW;
			info->next = NULL;
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

	::shadowState.invalidate();

	for ( auto& info : infos ) {
	#if UF_COMMAND_BUFFER_USERDATA
		CommandBuffer::Info* header = (CommandBuffer::Info*) (void*) info;
	#else
		CommandBuffer::Info* header = (CommandBuffer::Info*) info;
	#endif
		switch ( header->type ) {
			case ext::opengl::enums::Command::CLEAR: {
				InfoClear* info = (InfoClear*) header;
				GL_ERROR_CHECK(glClearColor(info->color[0], info->color[1], info->color[2], info->color[3]));
				GL_ERROR_CHECK(glClearDepth(info->depth));
				GL_ERROR_CHECK(glClear(info->bits));
				GL_ERROR_CHECK(glLightModelfv(GL_LIGHT_MODEL_AMBIENT, &info->color[0]));
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
				::shadowState.update( info->matrices );

				if ( ext::opengl::settings::pipelines::culling && info->attributes.instance.pointer && info->attributes.instance.length == sizeof(pod::Instance) ) {
					const pod::Instance& instance = *(pod::Instance*) info->attributes.instance.pointer;

					if ( !::inside( instance, ::shadowState.ndc ) ) continue;
				}

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
	#if !UF_COMMAND_BUFFER_USERDATA
	for ( auto& info : infos ) {
		CommandBuffer::Info* header = (CommandBuffer::Info*) info;
		switch ( header->type ) {
			case ext::opengl::enums::Command::CLEAR: {
				InfoClear* info = (InfoClear*) header;
				delete info;
			} break;
			case ext::opengl::enums::Command::VIEWPORT: {
				InfoViewport* info = (InfoViewport*) header;
				delete info;
			} break;
			case ext::opengl::enums::Command::VARIANT: {
				InfoVariant* info = (InfoVariant*) header;
				delete info;
			} break;
			case ext::opengl::enums::Command::DRAW: {
				InfoDraw* info = (InfoDraw*) header;
				delete info;
			} break;
			default: {
			} break;
		}
	}
	#endif
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
	if ( ::shadowState.modelViewDirty ) {
		GL_ERROR_CHECK(glMatrixMode(GL_MODELVIEW));
		GL_ERROR_CHECK(glLoadMatrixf( &::shadowState.modelView[0] ));
		::shadowState.modelViewDirty = false;
	}

	if ( ::shadowState.projectionDirty ) {
		GL_ERROR_CHECK(glMatrixMode(GL_PROJECTION));
		GL_ERROR_CHECK(glLoadMatrixf( &::shadowState.projection[0] ));
		::shadowState.projectionDirty = false;
	}

#if 0 && UF_ENV_DREAMCAST
	// washingtondc has a regression where non-alpha-tested polys do not render
	// more convenient to just work around the regression since later builds have working opengl backends
	GL_ERROR_CHECK(glEnable(GL_ALPHA_TEST));
	GL_ERROR_CHECK(glAlphaFunc(GL_GREATER, drawInfo.blend.alphaCutoff));

	GL_ERROR_CHECK(glEnable(GL_BLEND));
	GL_ERROR_CHECK(glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA));
#else
	if ( drawInfo.blend.modeAlpha > 0 ) {
		if ( !::shadowState.alphaTestEnabled ) {
			GL_ERROR_CHECK(glEnable(GL_ALPHA_TEST));
			::shadowState.alphaTestEnabled = true;
		}
		if ( ::shadowState.alphaCutoff != drawInfo.blend.alphaCutoff ) {
			GL_ERROR_CHECK(glAlphaFunc(GL_GREATER, drawInfo.blend.alphaCutoff));
			::shadowState.alphaCutoff = drawInfo.blend.alphaCutoff;
		}
		if ( !::shadowState.blendEnabled ) {
			GL_ERROR_CHECK(glEnable(GL_BLEND));
			GL_ERROR_CHECK(glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA));
			::shadowState.blendEnabled = true;
		}
	} else {
		if ( ::shadowState.alphaTestEnabled ) {
			GL_ERROR_CHECK(glDisable(GL_ALPHA_TEST));
			::shadowState.alphaTestEnabled = false;
		}
		if ( ::shadowState.blendEnabled ) {
			GL_ERROR_CHECK(glDisable(GL_BLEND));
			::shadowState.blendEnabled = false;
		}
	}
#endif

	if ( drawInfo.descriptor.cullMode != GL_NONE ) {
		if ( !::shadowState.cullEnabled ) {
			GL_ERROR_CHECK(glEnable(GL_CULL_FACE));
			::shadowState.cullEnabled = true;
		}
		if ( ::shadowState.frontFace != drawInfo.descriptor.frontFace ) {
			GL_ERROR_CHECK(glFrontFace(drawInfo.descriptor.frontFace));
			::shadowState.frontFace = drawInfo.descriptor.frontFace;
		}
		if ( ::shadowState.cullFace != drawInfo.descriptor.cullMode ) {
			GL_ERROR_CHECK(glCullFace(drawInfo.descriptor.cullMode));
			::shadowState.cullFace = drawInfo.descriptor.cullMode;
		}
	} else {
		if ( ::shadowState.cullEnabled ) {
			GL_ERROR_CHECK(glDisable(GL_CULL_FACE));
			::shadowState.cullEnabled = false;
		}
	}

	if ( drawInfo.descriptor.depth.test != ::shadowState.depthTestEnabled ) {
		if ( drawInfo.descriptor.depth.test ) {
			GL_ERROR_CHECK(glEnable(GL_DEPTH_TEST));
		} else {
			GL_ERROR_CHECK(glDisable(GL_DEPTH_TEST));
		}
		::shadowState.depthTestEnabled = drawInfo.descriptor.depth.test;
	}

	if ( drawInfo.descriptor.depth.write != ::shadowState.depthWriteEnabled ) {
		GL_ERROR_CHECK(glDepthMask(drawInfo.descriptor.depth.write ? GL_TRUE : GL_FALSE));
		::shadowState.depthWriteEnabled = drawInfo.descriptor.depth.write;
	}

	pod::Vector4f color = {1,1,1,1};
	if ( drawInfo.color.enabled ) {
		color = drawInfo.color.pointer ? *drawInfo.color.pointer : drawInfo.color.value;
	}
	if ( color != ::shadowState.color ) {
		GL_ERROR_CHECK(glColor4f( color[0], color[1], color[2], color[3] ));
		::shadowState.color = color;
	}

	
	GLenum vertexType = GL_FLOAT;
	switch ( drawInfo.attributes.position.descriptor.type ) {
		case uf::renderer::enums::Type::SHORT: vertexType = GL_SHORT; break;
	#if GL_QUANTIZED_SHORT
		case uf::renderer::enums::Type::USHORT: vertexType = GL_QUANTIZED_SHORT; break;
	#else
		case uf::renderer::enums::Type::USHORT: vertexType = GL_UNSIGNED_SHORT; break;
	#endif

	#if GL_HALF_FLOAT
		case uf::renderer::enums::Type::HALF: vertexType = GL_HALF_FLOAT; break;
	#endif
	//	case uf::renderer::enums::Type::BFLOAT: vertexType = GL_HALF_FLOAT; break; // 
		case uf::renderer::enums::Type::FLOAT: vertexType = GL_FLOAT; break;
	}

	GLenum normalType = GL_FLOAT;
	switch ( drawInfo.attributes.normal.descriptor.type ) {
		case uf::renderer::enums::Type::SHORT: normalType = GL_SHORT; break;
	#if GL_QUANTIZED_SHORT
		case uf::renderer::enums::Type::USHORT: normalType = GL_QUANTIZED_SHORT; break;
	#else
		case uf::renderer::enums::Type::USHORT: normalType = GL_UNSIGNED_SHORT; break;
	#endif
	#if GL_HALF_FLOAT
		case uf::renderer::enums::Type::HALF: normalType = GL_HALF_FLOAT; break;
	#endif
	//	case uf::renderer::enums::Type::BFLOAT: normalType = GL_HALF_FLOAT; break; // 
		case uf::renderer::enums::Type::FLOAT: normalType = GL_FLOAT; break;
	}

	GLenum uvType = GL_FLOAT;
	switch ( drawInfo.attributes.uv.descriptor.type ) {
		case uf::renderer::enums::Type::SHORT: uvType = GL_SHORT; break;
	#if GL_QUANTIZED_SHORT
		case uf::renderer::enums::Type::USHORT: uvType = GL_QUANTIZED_SHORT; break;
	#else
		case uf::renderer::enums::Type::USHORT: uvType = GL_UNSIGNED_SHORT; break;
	#endif

	#if GL_HALF_FLOAT
		case uf::renderer::enums::Type::HALF: uvType = GL_HALF_FLOAT; break;
	#endif
	//	case uf::renderer::enums::Type::BFLOAT: uvType = GL_HALF_FLOAT; break; // 
		case uf::renderer::enums::Type::FLOAT: uvType = GL_FLOAT; break;
	}

	GLenum stType = GL_FLOAT;
	switch ( drawInfo.attributes.st.descriptor.type ) {
		case uf::renderer::enums::Type::SHORT: stType = GL_SHORT; break;
	#if GL_QUANTIZED_SHORT
		case uf::renderer::enums::Type::USHORT: stType = GL_QUANTIZED_SHORT; break;
	#else
		case uf::renderer::enums::Type::USHORT: stType = GL_UNSIGNED_SHORT; break;
	#endif
	#if GL_HALF_FLOAT
		case uf::renderer::enums::Type::HALF: stType = GL_HALF_FLOAT; break;
	#endif
	//	case uf::renderer::enums::Type::BFLOAT: stType = GL_HALF_FLOAT; break; // 
		case uf::renderer::enums::Type::FLOAT: stType = GL_FLOAT; break;
	}

	GLenum indicesType = GL_UNSIGNED_INT;
	switch ( drawInfo.attributes.index.stride ) {
		case sizeof(uint32_t): indicesType = GL_UNSIGNED_INT; break;
		case sizeof(uint16_t): indicesType = GL_UNSIGNED_SHORT; break;
		case sizeof(uint8_t): indicesType = GL_UNSIGNED_BYTE; break;
	}
	
	// defined in case attributes need to be filled / converted on the fly
	uint8_t* normalPtr = drawInfo.attributes.normal.pointer ? (static_cast<uint8_t*>(drawInfo.attributes.normal.pointer) + drawInfo.attributes.normal.stride * drawInfo.descriptor.inputs.vertex.first) : NULL;
	uint8_t* colorPtr = drawInfo.attributes.color.pointer ? (static_cast<uint8_t*>(drawInfo.attributes.color.pointer) + drawInfo.attributes.color.stride * drawInfo.descriptor.inputs.vertex.first) : NULL;
	uint8_t* uvPtr = drawInfo.attributes.uv.pointer ? (static_cast<uint8_t*>(drawInfo.attributes.uv.pointer) + drawInfo.attributes.uv.stride * drawInfo.descriptor.inputs.vertex.first) : NULL;
	uint8_t* stPtr = drawInfo.attributes.st.pointer ? (static_cast<uint8_t*>(drawInfo.attributes.st.pointer) + drawInfo.attributes.st.stride * drawInfo.descriptor.inputs.vertex.first) : NULL;
	uint8_t* vertexPtr = drawInfo.attributes.position.pointer ? (static_cast<uint8_t*>(drawInfo.attributes.position.pointer) + drawInfo.attributes.position.stride * drawInfo.descriptor.inputs.vertex.first) : NULL;

	auto vertexStride = drawInfo.attributes.position.stride;
	STATIC_THREAD_LOCAL(uf::stl::vector<float>, vertexBufferRemap);

#if !UF_ENV_DREAMCAST
	if ( vertexType != GL_FLOAT ) {
		vertexBufferRemap.resize( drawInfo.descriptor.inputs.vertex.count * 3 );
		for ( size_t i = 0; i < drawInfo.descriptor.inputs.vertex.count * 3; ++i ) {
			switch ( drawInfo.attributes.position.descriptor.type ) {
				case uf::renderer::enums::Type::SHORT:
				case uf::renderer::enums::Type::USHORT:
					vertexBufferRemap[i] = uf::quant::dequantize( ((uint16_t*) vertexPtr)[i] );
				break;
			#if UF_USE_FLOAT16
				case uf::renderer::enums::Type::HALF:
					vertexBufferRemap[i] = ((float16*) vertexPtr)[i];
				break;
			#endif
			#if UF_USE_BFLOAT16
				case uf::renderer::enums::Type::BFLOAT:
					vertexBufferRemap[i] = ((bfloat16*) vertexPtr)[i];
				break;
			#endif
				default:
					vertexBufferRemap[i] = vertexPtr[i];
				break;
			}
		}

		vertexPtr = (uint8_t*) vertexBufferRemap.data();
		vertexType = GL_FLOAT;
		vertexStride = 0;
	}
#endif

	if ( drawInfo.attributes.normal.pointer ) {
		if ( !::shadowState.normalArrayEnabled ) {
			GL_ERROR_CHECK(glEnableClientState(GL_NORMAL_ARRAY));
			::shadowState.normalArrayEnabled = true;
		}
		GL_ERROR_CHECK(glNormalPointer(normalType, drawInfo.attributes.normal.stride, normalPtr));
	} else if ( ::shadowState.normalArrayEnabled ) {
		GL_ERROR_CHECK(glDisableClientState(GL_NORMAL_ARRAY));
		::shadowState.normalArrayEnabled = false;
	}

	if ( drawInfo.attributes.color.pointer ) {
		if ( !::shadowState.colorArrayEnabled ) {
			GL_ERROR_CHECK(glEnableClientState(GL_COLOR_ARRAY));
			::shadowState.colorArrayEnabled = true;
		}
		GLenum colorType = (drawInfo.attributes.color.descriptor.size / drawInfo.attributes.color.descriptor.components == sizeof(uint8_t)) ? GL_UNSIGNED_BYTE : GL_FLOAT;
		GL_ERROR_CHECK(glColorPointer(drawInfo.attributes.color.descriptor.components, colorType, drawInfo.attributes.color.stride, colorPtr));
	} else if ( ::shadowState.colorArrayEnabled ) {
		GL_ERROR_CHECK(glDisableClientState(GL_COLOR_ARRAY));
		::shadowState.colorArrayEnabled = false;
	}

	if ( drawInfo.textures.primary.image && drawInfo.attributes.uv.pointer ) {
		GL_ERROR_CHECK(glClientActiveTexture(GL_TEXTURE0));
		GL_ERROR_CHECK(glActiveTexture(GL_TEXTURE0));

		if ( !::shadowState.tex0Enabled || ::shadowState.tex0Target != drawInfo.textures.primary.viewType ) {
			if ( ::shadowState.tex0Enabled ) {
				GL_ERROR_CHECK(glDisable(::shadowState.tex0Target));
			}
			GL_ERROR_CHECK(glEnable(drawInfo.textures.primary.viewType));
			::shadowState.tex0Enabled = true;
			::shadowState.tex0Target = drawInfo.textures.primary.viewType;
		}

		if ( !::shadowState.texCoord0ArrayEnabled ) {
			GL_ERROR_CHECK(glEnableClientState(GL_TEXTURE_COORD_ARRAY));
			::shadowState.texCoord0ArrayEnabled = true;
		}
		if ( ::shadowState.boundTexture0 != drawInfo.textures.primary.image ) {
			GL_ERROR_CHECK(glBindTexture(drawInfo.textures.primary.viewType, drawInfo.textures.primary.image));
			::shadowState.boundTexture0 = drawInfo.textures.primary.image;
		}

		GL_ERROR_CHECK(glTexCoordPointer(2, uvType, drawInfo.attributes.uv.stride, uvPtr));
		GL_ERROR_CHECK(glTexEnvi(GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, drawInfo.attributes.color.pointer ? GL_MODULATE : GL_REPLACE));
	} else {
		if ( ::shadowState.tex0Enabled ) {
			GL_ERROR_CHECK(glClientActiveTexture(GL_TEXTURE0));
			GL_ERROR_CHECK(glActiveTexture(GL_TEXTURE0));
			GL_ERROR_CHECK(glDisable(::shadowState.tex0Target));
			::shadowState.tex0Enabled = false;
		}
		if ( ::shadowState.texCoord0ArrayEnabled ) {
			GL_ERROR_CHECK(glClientActiveTexture(GL_TEXTURE0));
			GL_ERROR_CHECK(glDisableClientState(GL_TEXTURE_COORD_ARRAY));
			::shadowState.texCoord0ArrayEnabled = false;
		}
	}

	if ( drawInfo.textures.primary.image && drawInfo.textures.secondary.image && drawInfo.attributes.st.pointer ) {
		GL_ERROR_CHECK(glClientActiveTexture(GL_TEXTURE1));
		GL_ERROR_CHECK(glActiveTexture(GL_TEXTURE1));

		if ( !::shadowState.tex1Enabled || ::shadowState.tex1Target != drawInfo.textures.secondary.viewType ) {
			if ( ::shadowState.tex1Enabled ) {
				GL_ERROR_CHECK(glDisable(::shadowState.tex1Target));
			}
			GL_ERROR_CHECK(glEnable(drawInfo.textures.secondary.viewType));
			::shadowState.tex1Enabled = true;
			::shadowState.tex1Target = drawInfo.textures.secondary.viewType;
		}

		if ( !::shadowState.texCoord1ArrayEnabled ) {
			GL_ERROR_CHECK(glEnableClientState(GL_TEXTURE_COORD_ARRAY));
			::shadowState.texCoord1ArrayEnabled = true;
		}
		if ( ::shadowState.boundTexture1 != drawInfo.textures.secondary.image ) {
			GL_ERROR_CHECK(glBindTexture(drawInfo.textures.secondary.viewType, drawInfo.textures.secondary.image));
			::shadowState.boundTexture1 = drawInfo.textures.secondary.image;
		}

		GL_ERROR_CHECK(glTexEnvi(GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_MODULATE));
		GL_ERROR_CHECK(glTexCoordPointer(2, stType, drawInfo.attributes.st.stride, stPtr));
	} else {
		if ( ::shadowState.tex1Enabled ) {
			GL_ERROR_CHECK(glClientActiveTexture(GL_TEXTURE1));
			GL_ERROR_CHECK(glActiveTexture(GL_TEXTURE1));
			GL_ERROR_CHECK(glDisable(::shadowState.tex1Target));
			::shadowState.tex1Enabled = false;
		}
		if ( ::shadowState.texCoord1ArrayEnabled ) {
			GL_ERROR_CHECK(glClientActiveTexture(GL_TEXTURE1));
			GL_ERROR_CHECK(glDisableClientState(GL_TEXTURE_COORD_ARRAY));
			::shadowState.texCoord1ArrayEnabled = false;
		}
	}

	{
		GL_ERROR_CHECK(glEnableClientState(GL_VERTEX_ARRAY));
		GL_ERROR_CHECK(glVertexPointer(3, vertexType, vertexStride, vertexPtr));
	}

	GLenum mode = GL_TRIANGLES;
	float lineWidth = 1.0f;
	
	if ( drawInfo.descriptor.fill == uf::renderer::enums::PolygonMode::LINE ) {
		mode = GL_LINES;
		lineWidth = drawInfo.descriptor.lineWidth;
	}

	if ( ::shadowState.lineWidth != lineWidth ) {
		GL_ERROR_CHECK(glLineWidth(lineWidth));
		::shadowState.lineWidth = lineWidth;
	}

	if ( drawInfo.descriptor.inputs.index.count ) {
		GL_ERROR_CHECK(glDrawElements(mode, drawInfo.descriptor.inputs.index.count, indicesType, (static_cast<uint8_t*>(drawInfo.attributes.index.pointer) + drawInfo.attributes.index.stride * drawInfo.descriptor.inputs.index.first)));
	} else {
		GL_ERROR_CHECK(glDrawArrays(mode, drawInfo.descriptor.inputs.vertex.first, drawInfo.descriptor.inputs.vertex.count));
	}

	if ( drawInfo.descriptor.inputs.index.count ) {
		GL_ERROR_CHECK(glDrawElements(mode, drawInfo.descriptor.inputs.index.count, indicesType, (static_cast<uint8_t*>(drawInfo.attributes.index.pointer) + drawInfo.attributes.index.stride * drawInfo.descriptor.inputs.index.first)));
	} else {
		GL_ERROR_CHECK(glDrawArrays(mode, drawInfo.descriptor.inputs.vertex.first, drawInfo.descriptor.inputs.vertex.count));
	}

	GL_ERROR_CHECK(glDisableClientState(GL_VERTEX_ARRAY));
}
#endif