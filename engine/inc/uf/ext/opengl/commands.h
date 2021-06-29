#pragma once

#include <mutex>

#include <uf/ext/opengl/ogl.h>

#include <uf/ext/opengl/uniform.h>
#include <uf/ext/opengl/buffer.h>
#include <uf/ext/opengl/texture.h>

#include <uf/utils/thread/thread.h>
#include <uf/utils/thread/perthread.h>
#include <uf/utils/userdata/userdata.h>
#include <uf/utils/graphic/descriptor.h>

#define UF_COMMAND_BUFFER_POINTERED_USERDATA 1

namespace ext {
	namespace opengl {
		struct Device;
		struct Graphic;
		
		struct UF_API CommandBuffer {
		public:
			struct UF_API Info {
				ext::opengl::enums::Command::type_t type = 0;
				Info* next = NULL;
			};
			struct UF_API InfoClear : public Info {
			//	ext::opengl::enums::Command::type_t type = 0;
				pod::Vector4f color = {};
				float depth = 0;
				uint32_t bits = 0;
			};
			struct UF_API InfoViewport : public Info {
			//	ext::opengl::enums::Command::type_t type = 0;
				pod::Vector2ui corner = {};
				pod::Vector2ui size = {};
			};
			struct UF_API InfoVariant : public Info {
			//	ext::opengl::enums::Command::type_t type = 0;
				typedef std::function<void()> function_t;
				function_t lambda;
			};
			struct UF_API InfoDraw : public Info {
				GraphicDescriptor descriptor = {};

				ext::opengl::Buffer::Descriptor vertexBuffer = {};
				ext::opengl::Buffer::Descriptor indexBuffer = {};
				ext::opengl::Buffer::Descriptor uniformBuffer = {};
				ext::opengl::Texture::Descriptor texture = {};
				ext::opengl::Texture::Descriptor auxTexture = {};

				struct {
					uint8_t position = 0;
					uint8_t uv = 0;
					uint8_t st = 0;
					uint8_t normal = 0;
					uint8_t color = 0;
				} attributes;
			};
		/*
			struct UF_API InfoBuffer : public Info {
			//	ext::opengl::enums::Command::type_t type = 0;
				ext::opengl::Buffer::Descriptor descriptor = { GL_NULL_HANDLE, 0, 0 };
				enums::Buffer::type_t usage = 0;
			};
			struct UF_API InfoTexture : public Info {
			//	ext::opengl::enums::Command::type_t type = 0;
				ext::opengl::Texture::Descriptor descriptor = {};
			};
			struct UF_API InfoPipeline : public Info {
			//	ext::opengl::enums::Command::type_t type = 0;
				struct Descriptor {
					GLuint pipeline = GL_NULL_HANDLE;
					GLuint vertexArray = GL_NULL_HANDLE;
				} descriptor = { GL_NULL_HANDLE, GL_NULL_HANDLE };
			};
			struct UF_API InfoGraphicBuffer : public Info {
			//	ext::opengl::enums::Command::type_t type = 0;
				ext::opengl::Graphic* graphic = NULL;
				size_t bufferIndex = 0;
			};
			struct UF_API InfoDraw : public Info {
			//	ext::opengl::enums::Command::type_t type = 0;
				size_t length = 0;
				GraphicDescriptor descriptor = {};
			};
			struct UF_API InfoGenerateTexture : public Info {
			//	ext::opengl::enums::Command::type_t type = 0;
				Texture::Descriptor descriptor;
				void* data = NULL;
				size_t size = 0;
			};
		*/
			#if UF_COMMAND_BUFFER_POINTERED_USERDATA
				typedef uf::PointeredUserdata userdata_t;
			#else
				typedef uf::Userdata userdata_t;
			#endif
			typedef std::function<void()> function_t;
			static size_t preallocate;

			Device* device;
			size_t state = 0;
			uf::stl::vector<CommandBuffer::userdata_t> infos;
			std::mutex* mutex;
			
			void initialize( Device& device );
			void destroy();

			void record( const CommandBuffer::Info& );
			void record( const CommandBuffer::InfoVariant::function_t& );
			void record( const CommandBuffer& );

			void submit();
			void flush();
			void start();
			void end();
			size_t size() const;
		protected:
			pod::Matrix4f bindUniform( const Buffer::Descriptor& descriptor );
			void drawIndexed( const InfoDraw& );
		//	void draw( const InfoDraw&, const Info&, const uf::stl::vector<InfoTexture*>& = {} );
		//	void drawIndexed( const InfoDraw&, const Info&, const Info&, const uf::stl::vector<InfoTexture*>& = {} );
		};
	}
}