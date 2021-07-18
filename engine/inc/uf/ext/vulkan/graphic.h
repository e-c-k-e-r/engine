#pragma once 	

#include <uf/ext/vulkan/device.h>
#include <uf/ext/vulkan/swapchain.h>
#include <uf/ext/vulkan/initializers.h>
#include <uf/ext/vulkan/texture.h>
#include <uf/ext/vulkan/shader.h>
#include <uf/utils/mesh/mesh.h>

namespace ext {
	namespace vulkan {
		struct Graphic;

		struct UF_API Pipeline {
			bool aliased = false;

			Device* device = NULL;

			VkPipeline pipeline = VK_NULL_HANDLE;
			VkPipelineLayout pipelineLayout = VK_NULL_HANDLE;
			VkDescriptorSetLayout descriptorSetLayout = VK_NULL_HANDLE;
			VkDescriptorPool descriptorPool = VK_NULL_HANDLE;
			VkDescriptorSet descriptorSet = VK_NULL_HANDLE;
			GraphicDescriptor descriptor = {};

			struct {
				uf::Serializer json;
				
				uf::stl::string type = "";
				bool process = true;
				bool rebuild = false;
			} metadata;

			void initialize( const Graphic& graphic );
			void initialize( const Graphic& graphic, const GraphicDescriptor& descriptor );
			void update( const Graphic& graphic );
			void update( const Graphic& graphic, const GraphicDescriptor& descriptor );
			void record( const Graphic& graphic, VkCommandBuffer, size_t = 0, size_t = 0 ) const;
			void record( const Graphic& graphic, const GraphicDescriptor& descriptor, VkCommandBuffer, size_t = 0, size_t = 0 ) const;
			void destroy();

			uf::stl::vector<Shader*> getShaders( uf::stl::vector<Shader>& );
			uf::stl::vector<const Shader*> getShaders( const uf::stl::vector<Shader>& ) const;
		};

		struct UF_API Material {
			bool aliased = false;
			Device* device = NULL;

			uf::stl::vector<Sampler> samplers;
			uf::stl::vector<Texture2D> textures;
			uf::stl::vector<Shader> shaders;

			struct Metadata {
				uf::Serializer json;
				bool autoInitializeUniforms = true;
				uf::stl::unordered_map<uf::stl::string, size_t> shaders;
			} metadata;

			void initialize( Device& device );
			void destroy();

			void attachShader( const uf::stl::string&, VkShaderStageFlagBits, const uf::stl::string& pipeline = "" );
			void initializeShaders( const uf::stl::vector<std::pair<uf::stl::string, VkShaderStageFlagBits>>&, const uf::stl::string& pipeline = "" );

			bool hasShader( const uf::stl::string& type, const uf::stl::string& pipeline = "" ) const;
			Shader& getShader( const uf::stl::string& type, const uf::stl::string& pipeline = "" );
			const Shader& getShader( const uf::stl::string& type, const uf::stl::string& pipeline = "" ) const;

			bool validate();
		};

		struct UF_API Graphic : public Buffers {
			GraphicDescriptor descriptor = {};

			bool initialized = false;
			bool process = true;
			Material material = {};
			uf::stl::unordered_map<GraphicDescriptor::hash_t, Pipeline> pipelines;

			struct {
				uf::stl::unordered_map<uf::stl::string, size_t> buffers;
			} metadata;

			~Graphic();
			void initialize( const uf::stl::string& = "" );
			void destroy();
			
		//	template<typename T, typename U>
		//	void initializeMesh( uf::Mesh<T, U>& mesh );
		//	void initializeAttributes( const uf::Mesh::Attributes& mesh );

			void initializeMesh( uf::Mesh& mesh, bool buffer = true );
			void updateMesh( uf::Mesh& mesh );

			bool hasPipeline( const GraphicDescriptor& descriptor ) const;
			void initializePipeline();
			Pipeline& initializePipeline( const GraphicDescriptor& descriptor, bool update = true );
			
			Pipeline& getPipeline();
			const Pipeline& getPipeline() const;

			Pipeline& getPipeline( const GraphicDescriptor& descriptor );
			const Pipeline& getPipeline( const GraphicDescriptor& descriptor ) const;

			void updatePipelines();
			
			void record( VkCommandBuffer commandBuffer, size_t pass = 0, size_t draw = 0 ) const;
			void record( VkCommandBuffer commandBuffer, const GraphicDescriptor& descriptor, size_t pass = 0, size_t draw = 0 ) const;
		};
	}
}


#include "graphic.inl"