#pragma once 	

#include <uf/ext/vulkan/device.h>
#include <uf/ext/vulkan/swapchain.h>
#include <uf/ext/vulkan/initializers.h>
#include <uf/ext/vulkan/texture.h>
#include <uf/utils/mesh/mesh.h>

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
			VkPipelineShaderStageCreateInfo descriptor = {};
			uf::stl::vector<VkDescriptorSetLayoutBinding> descriptorSetLayoutBindings;
			uf::stl::vector<VkSpecializationMapEntry> specializationMapEntries;
			VkSpecializationInfo specializationInfo = {};

			struct Metadata {
				uf::Serializer json;
				
				uf::stl::string pipeline = "";
				uf::stl::string type = "";
				bool autoInitializeUniforms = true;

				struct Definition {
					struct InOut {
						uf::stl::string name = "";
						uint32_t index = 0;
						uint32_t binding = 0;
						uint32_t size = 0;
					//	 int32_t buffer = -1;
						ext::vulkan::enums::Image::viewType_t type{};
					};
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
					//	 int32_t buffer = -1;
					};
					struct Storage {
						uf::stl::string name = "";
						uint32_t index = 0;
						uint32_t binding = 0;
						uint32_t size = 0;
					//	 int32_t buffer = -1;
					};
					struct PushConstant {
						uf::stl::string name = "";
						uint32_t index = 0;
						uint32_t binding = 0;
						uint32_t size = 0;
					};
					struct SpecializationConstants {
						uf::stl::string name = "";
						uint32_t index = 0;
						uf::stl::string type = "";
						bool validate = false;
						union {
							int32_t i;
							uint32_t ui;
							float f;
						} value;
					};
					uf::stl::unordered_map<size_t, Texture> textures;
					uf::stl::unordered_map<uf::stl::string, InOut> inputs;
					uf::stl::unordered_map<uf::stl::string, InOut> outputs;
					uf::stl::unordered_map<uf::stl::string, Uniform> uniforms;
					uf::stl::unordered_map<uf::stl::string, Storage> storage;
					uf::stl::unordered_map<uf::stl::string, PushConstant> pushConstants;
					uf::stl::unordered_map<uf::stl::string, SpecializationConstants> specializationConstants;
				} definitions;
			} metadata;

			ext::vulkan::userdata_t specializationConstants;
			uf::stl::vector<ext::vulkan::userdata_t> pushConstants;
			uf::stl::vector<ext::vulkan::userdata_t> uniforms;

			// for per-shader texture allotment, needed for our bloom pipeline
			uf::stl::vector<Texture2D> textures;

		//	~Shader();
			void initialize( Device& device, const uf::stl::string&, VkShaderStageFlagBits );
			void destroy();
			bool validate();

			bool hasUniform( const uf::stl::string& name ) const;

			Buffer& getUniformBuffer( const uf::stl::string& name );
			const Buffer& getUniformBuffer( const uf::stl::string& name ) const;

			ext::vulkan::userdata_t& getUniform( const uf::stl::string& name );
			const ext::vulkan::userdata_t& getUniform( const uf::stl::string& name ) const ;
			
			bool updateUniform( const uf::stl::string& name, const void*, size_t ) const;
			inline bool updateUniform( const uf::stl::string& name ) const {
				if ( !hasUniform(name) ) return false;
				return updateUniform(name, getUniform(name));
			}
			inline bool updateUniform( const uf::stl::string& name, const ext::vulkan::userdata_t& userdata ) const {
				return updateUniform(name, (const void*) userdata, userdata.size() );
			}

			bool hasStorage( const uf::stl::string& name ) const;

			Buffer& getStorageBuffer( const uf::stl::string& name );
			const Buffer& getStorageBuffer( const uf::stl::string& name ) const;
			
			bool updateStorage( const uf::stl::string& name, const void*, size_t ) const;
			inline bool updateStorage( const uf::stl::string& name, const ext::vulkan::userdata_t& userdata ) const {
				return updateStorage(name, (const void*) userdata, userdata.size() );
			}

		/*
			uf::Serializer getUniformJson( const uf::stl::string& name, bool cache = true );
			bool updateUniform( const uf::stl::string& name, const ext::json::Value& payload );
			ext::vulkan::userdata_t getUniformUserdata( const uf::stl::string& name, const ext::json::Value& payload );

			uf::Serializer getStorageJson( const uf::stl::string& name, bool cache = true );
			ext::vulkan::userdata_t getStorageUserdata( const uf::stl::string& name, const ext::json::Value& payload );
		*/
		};
	}
}