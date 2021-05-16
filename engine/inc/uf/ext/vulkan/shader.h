#pragma once 	

#include <uf/ext/vulkan/device.h>
#include <uf/ext/vulkan/swapchain.h>
#include <uf/ext/vulkan/initializers.h>
#include <uf/ext/vulkan/texture.h>
#include <uf/utils/graphic/descriptor.h>
#include <uf/utils/graphic/mesh.h>

#define UF_GRAPHIC_POINTERED_USERDATA 1

namespace ext {
	namespace vulkan {
		#if UF_GRAPHIC_POINTERED_USERDATA
			typedef uf::PointeredUserdata userdata_t;
		#else
			typedef uf::Userdata userdata_t;
		#endif

		ext::json::Value definitionToJson(/*const*/ ext::json::Value& definition );
		ext::vulkan::userdata_t jsonToUserdata( const ext::json::Value& payload, const ext::json::Value& definition );

		struct UF_API Shader : public Buffers {
			bool aliased = false;

			Device* device = NULL;
			std::string filename = "";

			VkShaderModule module = VK_NULL_HANDLE;
			VkPipelineShaderStageCreateInfo descriptor;
			std::vector<VkDescriptorSetLayoutBinding> descriptorSetLayoutBindings;
			std::vector<VkSpecializationMapEntry> specializationMapEntries;
			VkSpecializationInfo specializationInfo;

			uf::Serializer metadata;

			ext::vulkan::userdata_t specializationConstants;
			std::vector<ext::vulkan::userdata_t> pushConstants;
			std::vector<ext::vulkan::userdata_t> uniforms;
		//	~Shader();
			void initialize( Device& device, const std::string&, VkShaderStageFlagBits );
			void destroy();
			bool validate();

			bool hasUniform( const std::string& name );

			Buffer* getUniformBuffer( const std::string& name );
			ext::vulkan::userdata_t& getUniform( const std::string& name );
			uf::Serializer getUniformJson( const std::string& name, bool cache = true );
			ext::vulkan::userdata_t getUniformUserdata( const std::string& name, const ext::json::Value& payload );
			bool updateUniform( const std::string& name );
			bool updateUniform( const std::string& name, const ext::vulkan::userdata_t& );
			bool updateUniform( const std::string& name, const ext::json::Value& payload );
			
			bool hasStorage( const std::string& name );
			Buffer* getStorageBuffer( const std::string& name );
			uf::Serializer getStorageJson( const std::string& name, bool cache = true );
			ext::vulkan::userdata_t getStorageUserdata( const std::string& name, const ext::json::Value& payload );
		};
	}
}