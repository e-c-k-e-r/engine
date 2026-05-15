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

		struct UF_API Pipeline : public Buffers {
			static uf::stl::unordered_map<GraphicDescriptor, Pipeline> pipelines;

			bool aliased = false;

			Device* device = NULL;

			VkPipeline pipeline = VK_NULL_HANDLE;
			VkPipelineLayout pipelineLayout = VK_NULL_HANDLE;
			uf::stl::vector<VkDescriptorSetLayout> descriptorSetLayouts;
			GraphicDescriptor descriptor = {};

			uf::stl::vector<VkStridedDeviceAddressRegionKHR> sbtEntries;

			void initialize( const Graphic& graphic );
			void initialize( const Graphic& graphic, const GraphicDescriptor& descriptor );
			void record( const Graphic& graphic, VkCommandBuffer, size_t = 0, size_t = 0, size_t = 0 ) const;
			void destroy();
		};

		struct UF_API DescriptorSets : public Buffers {
			bool aliased = false;

			Device* device = NULL;

			uf::stl::vector<VkDescriptorSet> descriptorSets;
			GraphicDescriptor descriptor = {};

			struct {
				bool built = false;
				bool process = true;
			} metadata;

			void initialize( const Graphic& graphic );
			void initialize( const Graphic& graphic, const GraphicDescriptor& descriptor );
			void update( const Graphic& graphic );
			void update( const Graphic& graphic, const GraphicDescriptor& descriptor, const uf::stl::vector<Shader::Metadata::Definition>& = {} );
			void record( const Graphic& graphic, VkCommandBuffer, size_t = 0, size_t = 0, size_t = 0 ) const;
			void record( const Graphic& graphic, const GraphicDescriptor& descriptor, VkCommandBuffer, size_t = 0, size_t = 0, size_t = 0 ) const;
			void destroy();
			
			void collectBuffers( const Shader& shader, const RenderMode& renderMode, const Graphic& graphic, const std::function<void(const Buffer&)>& lambda ) const;
		};

		struct UF_API Material {
			bool aliased = false;
			Device* device = NULL;

			uf::stl::vector<Sampler> samplers;
			uf::stl::vector<Texture2D> textures;
			uf::stl::vector<Shader> shaders;

			struct Metadata {
				uf::Serializer json;
				bool autoInitializeUniformBuffers = true;
				bool autoInitializeUniformUserdatas = false;
				uf::stl::unordered_map<uf::stl::string, size_t> shaders;
				uf::stl::unordered_map<uf::stl::string, size_t> hashes;
			} metadata;

			void initialize( Device& device );
			void destroy();

			void attachShader( const uf::stl::string&, VkShaderStageFlagBits, const uf::stl::string& pipeline = "" );
			void initializeShaders( const uf::stl::vector<std::pair<uf::stl::string, VkShaderStageFlagBits>>&, const uf::stl::string& pipeline = "" );

			bool hasShader( const uf::stl::string& type, const uf::stl::string& pipeline = "" ) const;
			Shader& getShader( const uf::stl::string& type, const uf::stl::string& pipeline = "" );
			const Shader& getShader( const uf::stl::string& type, const uf::stl::string& pipeline = "" ) const;

			uf::stl::vector<const Shader*> getShaders( const uf::stl::string& = "" ) const;

			bool validate();
		};

		struct UF_API Graphic : public Buffers {
			GraphicDescriptor descriptor = {};

			bool initialized = false;
			bool process = true;
			Material material = {};
			
			uf::stl::unordered_map<GraphicDescriptor, Pipeline> pipelines;
			uf::stl::unordered_map<GraphicDescriptor, DescriptorSets> descriptorSets;

			struct {
				uf::stl::unordered_map<uf::stl::string, size_t> buffers;
			} metadata;

			struct {
				uf::stl::vector<uf::renderer::AccelerationStructure> bottoms;
				uf::stl::vector<uf::renderer::AccelerationStructure> tops;
			} accelerationStructures;

			~Graphic();
			void initialize( const uf::stl::string& = "" );
			void initialize( const GraphicDescriptor& );
			void update();
			void update( const GraphicDescriptor& );
			void destroy();

			// raster
			void initializeMesh( uf::Mesh& mesh, bool buffer = true );
			bool updateMesh( uf::Mesh& mesh );
			// raytrace
			void generateBottomAccelerationStructures();
			void generateTopAccelerationStructure( const uf::stl::vector<uf::renderer::Graphic*>&, const uf::stl::vector<pod::Instance>&, const uf::stl::vector<pod::Matrix4f>&  );

			void initializePipeline();
			Pipeline& initializePipeline( const GraphicDescriptor& descriptor );

			bool hasPipeline( const GraphicDescriptor& descriptor ) const;
			Pipeline& getPipeline();
			const Pipeline& getPipeline() const;
			Pipeline& getPipeline( const GraphicDescriptor& descriptor );
			const Pipeline& getPipeline( const GraphicDescriptor& descriptor ) const;

			void initializeDescriptorSet();
			DescriptorSets& initializeDescriptorSet( const GraphicDescriptor& descriptor );
			bool hasDescriptorSet( const GraphicDescriptor& descriptor ) const;
			DescriptorSets& getDescriptorSet();
			const DescriptorSets& getDescriptorSet() const;
			DescriptorSets& getDescriptorSet( const GraphicDescriptor& descriptor );
			const DescriptorSets& getDescriptorSet( const GraphicDescriptor& descriptor ) const;
			
			void record( VkCommandBuffer commandBuffer, size_t pass = 0, size_t draw = 0, size_t offset = 0 ) const;
			void record( VkCommandBuffer commandBuffer, const GraphicDescriptor& descriptor, size_t pass = 0, size_t draw = 0, size_t offset = 0 ) const;

		};
	}
}