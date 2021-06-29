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
			uf::stl::string filename = "";

			VkShaderModule module = VK_NULL_HANDLE;
			VkPipelineShaderStageCreateInfo descriptor;
			uf::stl::vector<VkDescriptorSetLayoutBinding> descriptorSetLayoutBindings;
			uf::stl::vector<VkSpecializationMapEntry> specializationMapEntries;
			VkSpecializationInfo specializationInfo;

			struct Metadata {
				uf::Serializer json;
				
				uf::stl::string pipeline = "";
				uf::stl::string type = "";

				struct Definition {
					struct Texture {
						uf::stl::string name = "";
						uint32_t index = 0;
						uint32_t binding = 0;
						uint32_t size = 0;
						ext::vulkan::enums::Image::viewType_t type{};
					};
					struct Uniform {
						uf::stl::string name = "";
						uint32_t index = 0;
						uint32_t binding = 0;
						uint32_t size = 0;
					};
					struct Storage {
						uf::stl::string name = "";
						uint32_t index = 0;
						uint32_t binding = 0;
						uint32_t size = 0;
					};
					struct PushConstant {
						uf::stl::string name = "";
						uint32_t index = 0;
						uint32_t binding = 0;
						uint32_t size = 0;
					};
					uf::stl::unordered_map<size_t, Texture> textures;
					uf::stl::unordered_map<uf::stl::string, Uniform> uniforms;
					uf::stl::unordered_map<uf::stl::string, Storage> storage;
					uf::stl::unordered_map<uf::stl::string, PushConstant> pushConstants;
				} definitions;
			} metadata;

			ext::vulkan::userdata_t specializationConstants;
			uf::stl::vector<ext::vulkan::userdata_t> pushConstants;
			uf::stl::vector<ext::vulkan::userdata_t> uniforms;
		//	~Shader();
			void initialize( Device& device, const uf::stl::string&, VkShaderStageFlagBits );
			void destroy();
			bool validate();

			bool hasUniform( const uf::stl::string& name );

			Buffer* getUniformBuffer( const uf::stl::string& name );
			ext::vulkan::userdata_t& getUniform( const uf::stl::string& name );
			uf::Serializer getUniformJson( const uf::stl::string& name, bool cache = true );
			ext::vulkan::userdata_t getUniformUserdata( const uf::stl::string& name, const ext::json::Value& payload );
			bool updateUniform( const uf::stl::string& name );
			bool updateUniform( const uf::stl::string& name, const ext::vulkan::userdata_t& );
			bool updateUniform( const uf::stl::string& name, const ext::json::Value& payload );
			
			bool hasStorage( const uf::stl::string& name );
			Buffer* getStorageBuffer( const uf::stl::string& name );
			uf::Serializer getStorageJson( const uf::stl::string& name, bool cache = true );
			ext::vulkan::userdata_t getStorageUserdata( const uf::stl::string& name, const ext::json::Value& payload );
		};
	}
}