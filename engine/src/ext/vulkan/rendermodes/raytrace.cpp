#if UF_USE_VULKAN

#include <uf/ext/vulkan/vulkan.h>
#include <uf/ext/vulkan/rendermodes/raytrace.h>
#include <uf/ext/vulkan/initializers.h>
#include <uf/utils/window/window.h>
#include <uf/utils/math/physics.h>
#include <uf/utils/graphic/graphic.h>
#include <uf/ext/vulkan/graphic.h>
#include <uf/engine/graph/graph.h>
#include <uf/utils/camera/camera.h>

namespace {
	//
	struct UniformDescriptor {
		struct Matrices {
			alignas(16) pod::Matrix4f view;
			alignas(16) pod::Matrix4f projection;
			alignas(16) pod::Matrix4f iView;
			alignas(16) pod::Matrix4f iProjection;
			alignas(16) pod::Matrix4f iProjectionView;
			alignas(16) pod::Vector4f eyePos;
		} matrices[2];
	};

	void initializeGraphic( uf::renderer::RayTraceRenderMode& renderMode ) {
		auto& device = *renderMode.device;
		auto& blitter = *renderMode.getBlitter();

		// setup buffers
		uf::Mesh mesh;
		mesh.bind<pod::Vertex_3F, uint16_t>();
		mesh.insertVertices<pod::Vertex_3F>({
			{ { -0.5f, -0.5f, 0.0f } },
     		{ { 0.5f, -0.5f, 0.0f } },
     		{ { 0.0f,  0.5f, 0.0f } },
		});
		mesh.insertIndices<uint16_t>({
			0, 1, 2
		});
		mesh.updateDescriptor();

		blitter.initialize( renderMode.getName() );
		blitter.initializeMesh( mesh );
		blitter.process = false;
		
		{
			auto& scene = uf::scene::getCurrentScene();
			auto& controller = scene.getController();
			auto& camera = controller.getComponent<uf::Camera>();

			UniformDescriptor uniforms;
			for ( auto i = 0; i < 2; ++i ) {
				uniforms.matrices[i] = UniformDescriptor::Matrices{
					.view = camera.getView(i),
					.projection = camera.getProjection(i),
					.iView = uf::matrix::inverse( camera.getView(i) ),
					.iProjection = uf::matrix::inverse( camera.getProjection(i) ),
					.iProjectionView = uf::matrix::inverse( camera.getProjection(i) * camera.getView(i) ),
					.eyePos = camera.getEye( i ),
				};
			}
			blitter.initializeBuffer( (const void*) &uniforms, sizeof(UniformDescriptor), uf::renderer::enums::Buffer::UNIFORM );
		}
		
		auto& image = blitter.material.textures.emplace_back();
		image.fromBuffers( NULL, 0, uf::renderer::enums::Format::R8G8B8A8_UNORM, uf::renderer::settings::width, uf::renderer::settings::height, 1, 1, VK_IMAGE_USAGE_SAMPLED_BIT | VK_IMAGE_USAGE_STORAGE_BIT, VK_IMAGE_LAYOUT_GENERAL );
	
		pod::Matrix4f transformMatrix = uf::matrix::identity();
		auto vertexBufferIndex = blitter.initializeBuffer( (const void*) mesh.vertex.attributes.front().pointer, mesh.vertex.count * mesh.vertex.size, uf::renderer::enums::Buffer::ADDRESS | VK_BUFFER_USAGE_ACCELERATION_STRUCTURE_BUILD_INPUT_READ_ONLY_BIT_KHR );
		auto indexBufferIndex = blitter.initializeBuffer( (const void*) mesh.index.attributes.front().pointer, mesh.index.count * mesh.index.size, uf::renderer::enums::Buffer::ADDRESS | VK_BUFFER_USAGE_ACCELERATION_STRUCTURE_BUILD_INPUT_READ_ONLY_BIT_KHR );

		blitter.requestedAlignment = 16;
		auto transformBufferIndex = blitter.initializeBuffer( &transformMatrix[0], sizeof(transformMatrix), uf::renderer::enums::Buffer::ADDRESS | VK_BUFFER_USAGE_ACCELERATION_STRUCTURE_BUILD_INPUT_READ_ONLY_BIT_KHR );
		blitter.requestedAlignment = 0;

		uf::renderer::Buffer vertexBuffer;
		uf::renderer::Buffer indexBuffer;
		uf::renderer::Buffer transformBuffer;
		uf::renderer::Buffer blasBuffer;
		uf::renderer::Buffer instancesBuffer;
		uf::renderer::Buffer tlasBuffer;
		uf::renderer::Buffer scratchBuffer;

		// build SBT
		{
			uf::stl::string rayGenShaderFilename = uf::io::root+"/shaders/raytrace/shader.gen.spv";
			uf::stl::string rayMissShaderFilename = uf::io::root+"/shaders/raytrace/shader.miss.spv";
			uf::stl::string rayHitShaderFilename = uf::io::root+"/shaders/raytrace/shader.hit.spv";
			blitter.material.initializeShaders({
				{uf::io::resolveURI(rayGenShaderFilename), VK_SHADER_STAGE_RAYGEN_BIT_KHR},
				{uf::io::resolveURI(rayMissShaderFilename), VK_SHADER_STAGE_MISS_BIT_KHR},
				{uf::io::resolveURI(rayHitShaderFilename), VK_SHADER_STAGE_CLOSEST_HIT_BIT_KHR},
			});
		}

		blitter.material.getShader("ray:gen").textures.emplace_back().aliasTexture( image );

		VkPhysicalDeviceAccelerationStructurePropertiesKHR acclerationStructureProperties{};
		VkPhysicalDeviceRayTracingPipelinePropertiesKHR rayTracingPipelineProperties{};
		VkPhysicalDeviceProperties2 deviceProperties2{};

		{
			acclerationStructureProperties.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_ACCELERATION_STRUCTURE_PROPERTIES_KHR;

			rayTracingPipelineProperties.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_RAY_TRACING_PIPELINE_PROPERTIES_KHR;
			rayTracingPipelineProperties.pNext = &acclerationStructureProperties;

			deviceProperties2.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_PROPERTIES_2;
			deviceProperties2.pNext = &rayTracingPipelineProperties;

			vkGetPhysicalDeviceProperties2(device.physicalDevice, &deviceProperties2);
		}

		// build blas
		{
			// get address
			VkDeviceOrHostAddressConstKHR vertexBufferDeviceAddress{};
			VkDeviceOrHostAddressConstKHR indexBufferDeviceAddress{};
			VkDeviceOrHostAddressConstKHR transformBufferDeviceAddress{};

			vertexBuffer.aliasBuffer( blitter.buffers[vertexBufferIndex] );
			indexBuffer.aliasBuffer( blitter.buffers[indexBufferIndex] );
			transformBuffer.aliasBuffer( blitter.buffers[transformBufferIndex] );

			UF_ASSERT( vertexBuffer.buffer );
			UF_ASSERT( indexBuffer.buffer );
			UF_ASSERT( transformBuffer.buffer );

			vertexBufferDeviceAddress.deviceAddress = vertexBuffer.getAddress();
			indexBufferDeviceAddress.deviceAddress = indexBuffer.getAddress();
			transformBufferDeviceAddress.deviceAddress = transformBuffer.getAddress();

			// attribute info
			uf::Mesh::Attribute vertexAttribute;
			for ( auto& attribute : mesh.vertex.attributes ) if ( attribute.descriptor.name == "position" ) vertexAttribute = attribute;

			UF_ASSERT( vertexAttribute.descriptor.name == "position" );

			// blas info
			VkAccelerationStructureGeometryKHR accelerationStructureGeometry{};
			accelerationStructureGeometry.sType = VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_GEOMETRY_KHR;
			accelerationStructureGeometry.flags = VK_GEOMETRY_OPAQUE_BIT_KHR;
			accelerationStructureGeometry.geometryType = VK_GEOMETRY_TYPE_TRIANGLES_KHR;
			accelerationStructureGeometry.geometry.triangles.sType = VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_GEOMETRY_TRIANGLES_DATA_KHR;

			accelerationStructureGeometry.geometry.triangles.vertexFormat = vertexAttribute.descriptor.format;
			accelerationStructureGeometry.geometry.triangles.vertexData = vertexBufferDeviceAddress;
			accelerationStructureGeometry.geometry.triangles.maxVertex = mesh.vertex.count;
			accelerationStructureGeometry.geometry.triangles.vertexStride = vertexAttribute.stride;

			accelerationStructureGeometry.geometry.triangles.indexType = VK_INDEX_TYPE_UINT16;
			accelerationStructureGeometry.geometry.triangles.indexData = indexBufferDeviceAddress;

			accelerationStructureGeometry.geometry.triangles.transformData.deviceAddress = 0;
			accelerationStructureGeometry.geometry.triangles.transformData.hostAddress = nullptr;
			accelerationStructureGeometry.geometry.triangles.transformData = transformBufferDeviceAddress;

			// size info
			VkAccelerationStructureBuildGeometryInfoKHR accelerationStructureBuildGeometryInfo{};
			accelerationStructureBuildGeometryInfo.sType = VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_BUILD_GEOMETRY_INFO_KHR;
			accelerationStructureBuildGeometryInfo.type = VK_ACCELERATION_STRUCTURE_TYPE_BOTTOM_LEVEL_KHR;
			accelerationStructureBuildGeometryInfo.flags = VK_BUILD_ACCELERATION_STRUCTURE_PREFER_FAST_TRACE_BIT_KHR;
			accelerationStructureBuildGeometryInfo.geometryCount = 1;
			accelerationStructureBuildGeometryInfo.pGeometries = &accelerationStructureGeometry;

			const uint32_t numTriangles = 1; // (mesh.index.count ? mesh.index.count : mesh.vertex.count) / 3;
			VkAccelerationStructureBuildSizesInfoKHR accelerationStructureBuildSizesInfo{};
			accelerationStructureBuildSizesInfo.sType = VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_BUILD_SIZES_INFO_KHR;

			UF_MSG_DEBUG("vkGetAccelerationStructureBuildSizesKHR");
			uf::renderer::vkGetAccelerationStructureBuildSizesKHR(
				device,
				VK_ACCELERATION_STRUCTURE_BUILD_TYPE_DEVICE_KHR,
				&accelerationStructureBuildGeometryInfo,
				&numTriangles,
				&accelerationStructureBuildSizesInfo
			);

			// build blas buffer
			size_t blasBufferIndex = blitter.initializeBuffer( NULL, accelerationStructureBuildSizesInfo.accelerationStructureSize, uf::renderer::enums::Buffer::ACCELERATION_STRUCTURE | uf::renderer::enums::Buffer::ADDRESS );
			blitter.accelerationStructures.bottom.buffer.aliasBuffer( blitter.buffers[blasBufferIndex] );

			// build blas handle
			VkAccelerationStructureCreateInfoKHR accelerationStructureCreateInfo{};
			accelerationStructureCreateInfo.sType = VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_CREATE_INFO_KHR;
			accelerationStructureCreateInfo.buffer = blitter.accelerationStructures.bottom.buffer.buffer;
			accelerationStructureCreateInfo.size = accelerationStructureBuildSizesInfo.accelerationStructureSize;
			accelerationStructureCreateInfo.type = VK_ACCELERATION_STRUCTURE_TYPE_BOTTOM_LEVEL_KHR;

			UF_MSG_DEBUG("vkCreateAccelerationStructureKHR");
			VK_CHECK_RESULT(uf::renderer::vkCreateAccelerationStructureKHR(device, &accelerationStructureCreateInfo, nullptr, &blitter.accelerationStructures.bottom.handle));

			// build scratch buffer
			scratchBuffer.alignment = acclerationStructureProperties.minAccelerationStructureScratchOffsetAlignment;
			scratchBuffer.initialize( NULL, accelerationStructureBuildSizesInfo.buildScratchSize, uf::renderer::enums::Buffer::STORAGE | uf::renderer::enums::Buffer::ADDRESS );

			VkAccelerationStructureBuildGeometryInfoKHR accelerationBuildGeometryInfo{};
			accelerationBuildGeometryInfo.sType = VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_BUILD_GEOMETRY_INFO_KHR;
			accelerationBuildGeometryInfo.type = VK_ACCELERATION_STRUCTURE_TYPE_BOTTOM_LEVEL_KHR;
			accelerationBuildGeometryInfo.flags = VK_BUILD_ACCELERATION_STRUCTURE_PREFER_FAST_TRACE_BIT_KHR;
			accelerationBuildGeometryInfo.mode = VK_BUILD_ACCELERATION_STRUCTURE_MODE_BUILD_KHR;
			accelerationBuildGeometryInfo.dstAccelerationStructure = blitter.accelerationStructures.bottom.handle;
			accelerationBuildGeometryInfo.geometryCount = 1; // numTriangles;
			accelerationBuildGeometryInfo.pGeometries = &accelerationStructureGeometry;
			accelerationBuildGeometryInfo.scratchData.deviceAddress = scratchBuffer.getAddress();

			VkAccelerationStructureBuildRangeInfoKHR accelerationStructureBuildRangeInfo{};
			accelerationStructureBuildRangeInfo.primitiveCount = numTriangles;
			accelerationStructureBuildRangeInfo.primitiveOffset = 0;
			accelerationStructureBuildRangeInfo.firstVertex = 0;
			accelerationStructureBuildRangeInfo.transformOffset = 0;
			std::vector<VkAccelerationStructureBuildRangeInfoKHR*> accelerationBuildStructureRangeInfos = { &accelerationStructureBuildRangeInfo };

			// build blas
			VkCommandBuffer commandBuffer = device.createCommandBuffer(VK_COMMAND_BUFFER_LEVEL_PRIMARY, uf::renderer::Device::QueueEnum::COMPUTE);

			UF_MSG_DEBUG("vkCmdBuildAccelerationStructuresKHR");
			uf::renderer::vkCmdBuildAccelerationStructuresKHR(
				commandBuffer,
				1,
				&accelerationBuildGeometryInfo,
				accelerationBuildStructureRangeInfos.data()
			);
			device.flushCommandBuffer(commandBuffer, uf::renderer::Device::QueueEnum::COMPUTE);

			VkAccelerationStructureDeviceAddressInfoKHR accelerationDeviceAddressInfo{};
			accelerationDeviceAddressInfo.sType = VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_DEVICE_ADDRESS_INFO_KHR;
			accelerationDeviceAddressInfo.accelerationStructure = blitter.accelerationStructures.bottom.handle;
			blitter.accelerationStructures.bottom.deviceAddress = uf::renderer::vkGetAccelerationStructureDeviceAddressKHR(device, &accelerationDeviceAddressInfo);

			scratchBuffer.destroy();
		}
		// build tlas
		{
			// setup instances buffer
			VkAccelerationStructureInstanceKHR instance{};
			instance.transform = {
				1.0f, 0.0f, 0.0f, 0.0f,
				0.0f, 1.0f, 0.0f, 0.0f,
				0.0f, 0.0f, 1.0f, 0.0f
			};
			instance.instanceCustomIndex = 0;
			instance.mask = 0xFF;
			instance.instanceShaderBindingTableRecordOffset = 0;
			instance.flags = VK_GEOMETRY_INSTANCE_TRIANGLE_FACING_CULL_DISABLE_BIT_KHR;
			instance.accelerationStructureReference = blitter.accelerationStructures.bottom.deviceAddress;

			auto instancesBufferIndex = blitter.initializeBuffer( NULL, sizeof(instance), uf::renderer::enums::Buffer::ADDRESS | VK_BUFFER_USAGE_ACCELERATION_STRUCTURE_BUILD_INPUT_READ_ONLY_BIT_KHR );
			instancesBuffer.aliasBuffer( blitter.buffers[instancesBufferIndex] );
			blitter.updateBuffer( (const void*) &instance, sizeof(instance), instancesBuffer );

			VkDeviceOrHostAddressConstKHR instanceDataDeviceAddress{};
			instanceDataDeviceAddress.deviceAddress = instancesBuffer.getAddress();

			VkAccelerationStructureGeometryKHR accelerationStructureGeometry{};
			accelerationStructureGeometry.sType = VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_GEOMETRY_KHR;
			accelerationStructureGeometry.geometryType = VK_GEOMETRY_TYPE_INSTANCES_KHR;
			accelerationStructureGeometry.flags = VK_GEOMETRY_OPAQUE_BIT_KHR;
			accelerationStructureGeometry.geometry.instances.sType = VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_GEOMETRY_INSTANCES_DATA_KHR;
			accelerationStructureGeometry.geometry.instances.arrayOfPointers = VK_FALSE;
			accelerationStructureGeometry.geometry.instances.data = instanceDataDeviceAddress;

			VkAccelerationStructureBuildGeometryInfoKHR accelerationStructureBuildGeometryInfo{};
			accelerationStructureBuildGeometryInfo.sType = VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_BUILD_GEOMETRY_INFO_KHR;
			accelerationStructureBuildGeometryInfo.type = VK_ACCELERATION_STRUCTURE_TYPE_TOP_LEVEL_KHR;
			accelerationStructureBuildGeometryInfo.flags = VK_BUILD_ACCELERATION_STRUCTURE_PREFER_FAST_TRACE_BIT_KHR;
			accelerationStructureBuildGeometryInfo.geometryCount = 1;
			accelerationStructureBuildGeometryInfo.pGeometries = &accelerationStructureGeometry;

			const uint32_t primitive_count = 1;
			VkAccelerationStructureBuildSizesInfoKHR accelerationStructureBuildSizesInfo{};
			accelerationStructureBuildSizesInfo.sType = VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_BUILD_SIZES_INFO_KHR;
			UF_MSG_DEBUG("vkGetAccelerationStructureBuildSizesKHR");
			uf::renderer::vkGetAccelerationStructureBuildSizesKHR(
				device,
				VK_ACCELERATION_STRUCTURE_BUILD_TYPE_DEVICE_KHR,
				&accelerationStructureBuildGeometryInfo,
				&primitive_count,
				&accelerationStructureBuildSizesInfo
			);

			// build tlas buffer
			size_t tlasBufferIndex = blitter.initializeBuffer( NULL, accelerationStructureBuildSizesInfo.accelerationStructureSize, uf::renderer::enums::Buffer::ACCELERATION_STRUCTURE | uf::renderer::enums::Buffer::ADDRESS );
			blitter.accelerationStructures.top.buffer.aliasBuffer( blitter.buffers[tlasBufferIndex] );
			blitter.material.getShader("ray:gen").buffers.emplace_back().aliasBuffer( blitter.buffers[tlasBufferIndex] );

			// build tlas handle
			VkAccelerationStructureCreateInfoKHR accelerationStructureCreateInfo{};
			accelerationStructureCreateInfo.sType = VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_CREATE_INFO_KHR;
			accelerationStructureCreateInfo.buffer = blitter.accelerationStructures.top.buffer.buffer;
			accelerationStructureCreateInfo.size = accelerationStructureBuildSizesInfo.accelerationStructureSize;
			accelerationStructureCreateInfo.type = VK_ACCELERATION_STRUCTURE_TYPE_TOP_LEVEL_KHR;

			UF_MSG_DEBUG("vkCreateAccelerationStructureKHR");
			VK_CHECK_RESULT(uf::renderer::vkCreateAccelerationStructureKHR(device, &accelerationStructureCreateInfo, nullptr, &blitter.accelerationStructures.top.handle));

			// build scratch buffer
			scratchBuffer.alignment = acclerationStructureProperties.minAccelerationStructureScratchOffsetAlignment;
			scratchBuffer.initialize( NULL, accelerationStructureBuildSizesInfo.buildScratchSize, uf::renderer::enums::Buffer::STORAGE | uf::renderer::enums::Buffer::ADDRESS );
			UF_MSG_DEBUG(accelerationStructureBuildSizesInfo.buildScratchSize);

			VkAccelerationStructureBuildGeometryInfoKHR accelerationBuildGeometryInfo{};
			accelerationBuildGeometryInfo.sType = VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_BUILD_GEOMETRY_INFO_KHR;
			accelerationBuildGeometryInfo.type = VK_ACCELERATION_STRUCTURE_TYPE_TOP_LEVEL_KHR;
			accelerationBuildGeometryInfo.flags = VK_BUILD_ACCELERATION_STRUCTURE_PREFER_FAST_TRACE_BIT_KHR;
			accelerationBuildGeometryInfo.mode = VK_BUILD_ACCELERATION_STRUCTURE_MODE_BUILD_KHR;
			accelerationBuildGeometryInfo.dstAccelerationStructure = blitter.accelerationStructures.top.handle;
			accelerationBuildGeometryInfo.geometryCount = 1;
			accelerationBuildGeometryInfo.pGeometries = &accelerationStructureGeometry;
			accelerationBuildGeometryInfo.scratchData.deviceAddress = scratchBuffer.getAddress();

			VkAccelerationStructureBuildRangeInfoKHR accelerationStructureBuildRangeInfo{};
			accelerationStructureBuildRangeInfo.primitiveCount = 1;
			accelerationStructureBuildRangeInfo.primitiveOffset = 0;
			accelerationStructureBuildRangeInfo.firstVertex = 0;
			accelerationStructureBuildRangeInfo.transformOffset = 0;
			std::vector<VkAccelerationStructureBuildRangeInfoKHR*> accelerationBuildStructureRangeInfos = { &accelerationStructureBuildRangeInfo };

			VkCommandBuffer commandBuffer = device.createCommandBuffer(VK_COMMAND_BUFFER_LEVEL_PRIMARY, uf::renderer::Device::QueueEnum::COMPUTE);
			UF_MSG_DEBUG("vkCmdBuildAccelerationStructuresKHR");
			uf::renderer::vkCmdBuildAccelerationStructuresKHR(
				commandBuffer,
				1,
				&accelerationBuildGeometryInfo,
				accelerationBuildStructureRangeInfos.data()
			);
			device.flushCommandBuffer(commandBuffer, uf::renderer::Device::QueueEnum::COMPUTE);

			VkAccelerationStructureDeviceAddressInfoKHR accelerationDeviceAddressInfo{};
			accelerationDeviceAddressInfo.sType = VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_DEVICE_ADDRESS_INFO_KHR;
			accelerationDeviceAddressInfo.accelerationStructure = blitter.accelerationStructures.top.handle;
			blitter.accelerationStructures.top.deviceAddress = uf::renderer::vkGetAccelerationStructureDeviceAddressKHR(device, &accelerationDeviceAddressInfo);

			scratchBuffer.destroy();
		}
		blitter.descriptor.inputs.width = image.width;
		blitter.descriptor.inputs.height = image.height;

		blitter.initializePipeline();

		blitter.initialized = true;
		blitter.process = true;
		UF_MSG_DEBUG("Initialized ray tracer");
	}
}

const uf::stl::string ext::vulkan::RayTraceRenderMode::getTarget() const {
//	auto& metadata = *const_cast<uf::Serializer*>(&this->metadata);
//	return metadata["target"].as<uf::stl::string>();
	return metadata.target;
}
void ext::vulkan::RayTraceRenderMode::setTarget( const uf::stl::string& target ) {
//	this->metadata["target"] = target;
	metadata.target = target;
}

const uf::stl::string ext::vulkan::RayTraceRenderMode::getType() const {
	return "Compute";
}
const size_t ext::vulkan::RayTraceRenderMode::blitters() const {
	return 1;
}
ext::vulkan::Graphic* ext::vulkan::RayTraceRenderMode::getBlitter( size_t i ) {
	return &this->blitter;
}
uf::stl::vector<ext::vulkan::Graphic*> ext::vulkan::RayTraceRenderMode::getBlitters() {
	return { &this->blitter };
}

ext::vulkan::GraphicDescriptor ext::vulkan::RayTraceRenderMode::bindGraphicDescriptor( const ext::vulkan::GraphicDescriptor& reference, size_t pass ) {
	ext::vulkan::GraphicDescriptor descriptor = ext::vulkan::RenderMode::bindGraphicDescriptor(reference, pass);
	descriptor.parse(metadata.json["descriptor"]);

	// invalidate
	if ( metadata.target != "" && descriptor.renderMode != this->getName() && descriptor.renderMode != metadata.target ) {
		descriptor.invalidated = true;
	} else {
		descriptor.renderMode = this->getName();
	}
	return descriptor;
}

void ext::vulkan::RayTraceRenderMode::initialize( Device& device ) {
	ext::vulkan::RenderMode::initialize( device );

	blitter.process = false;
	initializeGraphic(*this);
}

void ext::vulkan::RayTraceRenderMode::tick() {
	ext::vulkan::RenderMode::tick();

	bool resized = this->width == 0 && this->height == 0 && ext::vulkan::states::resized;
	bool rebuild = resized || ext::vulkan::states::rebuild || this->rebuild;

	if ( resized ) {
		auto& image = blitter.material.textures.front();

		image.destroy();
		image.fromBuffers( NULL, 0, uf::renderer::enums::Format::R8G8B8A8_UNORM, uf::renderer::settings::width, uf::renderer::settings::height, 1, 1, VK_IMAGE_USAGE_SAMPLED_BIT | VK_IMAGE_USAGE_STORAGE_BIT, VK_IMAGE_LAYOUT_GENERAL );

		auto& shader = blitter.material.getShader("ray:gen");
		shader.textures.clear();
		shader.textures.emplace_back().aliasTexture( image );

		blitter.descriptor.inputs.width = image.width;
		blitter.descriptor.inputs.height = image.height;

		blitter.getPipeline().update( blitter );
	}

	{
		auto& scene = uf::scene::getCurrentScene();
		auto& controller = scene.getController();
		auto& camera = controller.getComponent<uf::Camera>();

		UniformDescriptor uniforms;
		for ( auto i = 0; i < 2; ++i ) {
			uniforms.matrices[i] = UniformDescriptor::Matrices{
				.view = camera.getView(i),
				.projection = camera.getProjection(i),
				.iView = uf::matrix::inverse( camera.getView(i) ),
				.iProjection = uf::matrix::inverse( camera.getProjection(i) ),
				.iProjectionView = uf::matrix::inverse( camera.getProjection(i) * camera.getView(i) ),
				.eyePos = camera.getEye( i ),
			};
		}

		for ( auto& buffer : blitter.buffers ) {
			if ( !(buffer.usage & uf::renderer::enums::Buffer::UNIFORM) ) continue;
			if ( buffer.allocationInfo.size != sizeof(UniformDescriptor) ) continue;
			
			buffer.update( (const void*) &uniforms, sizeof(UniformDescriptor) );
			break;
		}
		
	}

	if ( metadata.limiter.frequency > 0 ) {
		if ( metadata.limiter.timer > metadata.limiter.frequency ) {
			metadata.limiter.timer = 0;
			metadata.limiter.execute = true;
		} else {
			metadata.limiter.timer = metadata.limiter.timer + uf::physics::time::delta;
			metadata.limiter.execute = false;
		}
	}

//	if ( !graphic.initialized ) initializeGraphic(*this);
}
void ext::vulkan::RayTraceRenderMode::destroy() {
	auto& image = blitter.material.textures.front();
	image.screenshot().save("./data/rt.png");
	image.destroy();

	blitter.destroy();

	ext::vulkan::RenderMode::destroy();
}

void ext::vulkan::RayTraceRenderMode::render() {	
	if ( commandBufferCallbacks.count(EXECUTE_BEGIN) > 0 ) commandBufferCallbacks[EXECUTE_BEGIN]( VkCommandBuffer{} );

	//lockMutex( this->mostRecentCommandPoolId );
	auto& commands = getCommands( this->mostRecentCommandPoolId );

	// Submit commands
	// Use a fence to ensure that command buffer has finished executing before using it again
	VK_CHECK_RESULT(vkWaitForFences( *device, 1, &fences[states::currentBuffer], VK_TRUE, UINT64_MAX ));
	VK_CHECK_RESULT(vkResetFences( *device, 1, &fences[states::currentBuffer] ));

	VkSubmitInfo submitInfo = {};
	submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
	submitInfo.pWaitDstStageMask = NULL; 								// Pointer to the list of pipeline stages that the semaphore waits will occur at
	submitInfo.pWaitSemaphores = NULL;									// Semaphore(s) to wait upon before the submitted command buffer starts executing
	submitInfo.waitSemaphoreCount = 0;									// One wait semaphore																				
	submitInfo.pSignalSemaphores = NULL;								// Semaphore(s) to be signaled when command buffers have completed
	submitInfo.signalSemaphoreCount = 0;								// One signal semaphore
	submitInfo.pCommandBuffers = &commands[states::currentBuffer];		// Command buffers(s) to execute in this batch (submission)
	submitInfo.commandBufferCount = 1;

	VK_CHECK_RESULT(vkQueueSubmit(device->getQueue( uf::renderer::Device::QueueEnum::COMPUTE ), 1, &submitInfo, fences[states::currentBuffer]));

	if ( commandBufferCallbacks.count(EXECUTE_END) > 0 ) commandBufferCallbacks[EXECUTE_END]( VkCommandBuffer{} );

	this->executed = true;
	//unlockMutex( this->mostRecentCommandPoolId );
}
void ext::vulkan::RayTraceRenderMode::pipelineBarrier( VkCommandBuffer commandBuffer, uint8_t state ) {
}
void ext::vulkan::RayTraceRenderMode::createCommandBuffers( const uf::stl::vector<ext::vulkan::Graphic*>& graphics ) {
	// destroy if exists
	float width = this->width > 0 ? this->width : ext::vulkan::settings::width;
	float height = this->height > 0 ? this->height : ext::vulkan::settings::height;

	VkCommandBufferBeginInfo cmdBufInfo = {};
	cmdBufInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
	cmdBufInfo.pNext = nullptr;
	auto& commands = getCommands();
	for (size_t i = 0; i < commands.size(); ++i) {
		VK_CHECK_RESULT(vkBeginCommandBuffer(commands[i], &cmdBufInfo));
		
		// pre-renderpass commands
		if ( commandBufferCallbacks.count(CALLBACK_BEGIN) > 0 ) commandBufferCallbacks[CALLBACK_BEGIN]( commands[i] );

		if ( blitter.process ) {
			auto& image = blitter.material.textures.front();
			auto descriptor = blitter.descriptor;
			descriptor.inputs.width = image.width;
			descriptor.inputs.height = image.height;
			blitter.getPipeline().record( blitter, descriptor, commands[i] );
		}
		
		// post-renderpass commands
		if ( commandBufferCallbacks.count(CALLBACK_END) > 0 ) commandBufferCallbacks[CALLBACK_END]( commands[i] );

		VK_CHECK_RESULT(vkEndCommandBuffer(commands[i]));
	}
}

#endif