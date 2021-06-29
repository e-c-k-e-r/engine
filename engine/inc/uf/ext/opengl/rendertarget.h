#pragma once

#include <uf/ext/opengl/ogl.h>
#include <uf/ext/opengl/enums.h>
#include <uf/utils/memory/vector.h>

namespace ext {
	namespace opengl {
		struct Device;

		struct UF_API RenderTarget {
			struct Attachment {
				struct Descriptor {
					GLhandle(VkFormat) format;
					GLhandle(VkImageLayout) layout;
					GLhandle(VkImageUsageFlags) usage;
					bool blend = false;
					uint8_t samples = 1;
					bool aliased = false;
				} descriptor;

				GLhandle(VkImage) image;
				GLhandle(VkDeviceMemory) mem;
				GLhandle(VkImageView) view;
			
				GLhandle(VmaAllocation) allocation;
				GLhandle(VmaAllocationInfo) allocationInfo;
			
				GLhandle(VkPipelineColorBlendAttachmentState) blendState;
			};
			uf::stl::vector<Attachment> attachments;

			struct Subpass {
				GLhandle(VkPipelineStageFlags) stage;
				GLhandle(VkAccessFlags) access;
				bool autoBuildPipeline;

				uf::stl::vector<GLhandle(VkAttachmentReference)> colors;
				uf::stl::vector<GLhandle(VkAttachmentReference)> inputs;
				uf::stl::vector<GLhandle(VkAttachmentReference)> resolves;
				GLhandle(VkAttachmentReference) depth;
			};
			uf::stl::vector<Subpass> passes;

			bool initialized = false;
			Device* device = GL_NULL_HANDLE;
			GLhandle(VkRenderPass) renderPass = GL_NULL_HANDLE;
			uf::stl::vector<GLhandle(VkFramebuffer)> framebuffers;
			uint32_t width = 0;
			uint32_t height = 0;
			uint8_t views = 1;
			// RAII
			void initialize( Device& device );
			void destroy();
			void addPass( GLhandle(VkPipelineStageFlags), GLhandle(VkAccessFlags), const uf::stl::vector<size_t>&, const uf::stl::vector<size_t>&, const uf::stl::vector<size_t>&, size_t, bool = true  );
			size_t attach( const Attachment::Descriptor& descriptor, Attachment* attachment = NULL );
		};
	}
}