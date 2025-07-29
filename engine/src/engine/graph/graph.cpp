#include <uf/engine/graph/graph.h>
#include <uf/ext/gltf/gltf.h>
#include <uf/utils/math/physics.h>
#include <uf/utils/mesh/grid.h>
#include <uf/utils/thread/thread.h>
#include <uf/utils/string/base64.h>
#include <uf/utils/graphic/graphic.h>
#include <uf/utils/camera/camera.h>
#include <uf/utils/math/physics.h>
#include <uf/utils/memory/map.h>
#include <uf/ext/xatlas/xatlas.h>
#include <uf/ext/ffx/fsr.h>
#include <uf/ext/ext.h>

#if UF_ENV_DREAMCAST
	#define UF_DEBUG_TIMER_MULTITRACE_START(...) UF_TIMER_MULTITRACE_START(__VA_ARGS__)
	#define UF_DEBUG_TIMER_MULTITRACE(...) UF_TIMER_MULTITRACE(__VA_ARGS__)
	#define UF_DEBUG_TIMER_MULTITRACE_END(...) UF_TIMER_MULTITRACE_END(__VA_ARGS__)
#else
	#define UF_DEBUG_TIMER_MULTITRACE_START(...)
	#define UF_DEBUG_TIMER_MULTITRACE(...)
	#define UF_DEBUG_TIMER_MULTITRACE_END(...)
#endif

namespace {
	bool newGraphAdded = true;
}

#if UF_ENV_DREAMCAST
	size_t uf::graph::initialBufferElements = 256;
#else
	size_t uf::graph::initialBufferElements = 1024;
#endif
bool uf::graph::globalStorage;
pod::Graph::Storage uf::graph::storage;

UF_VERTEX_DESCRIPTOR(uf::graph::mesh::Base,
	UF_VERTEX_DESCRIPTION(uf::graph::mesh::Base, R32G32B32_SFLOAT, position)
	UF_VERTEX_DESCRIPTION(uf::graph::mesh::Base, R32G32_SFLOAT, uv)
	UF_VERTEX_DESCRIPTION(uf::graph::mesh::Base, R8G8B8A8_UNORM, color)
	UF_VERTEX_DESCRIPTION(uf::graph::mesh::Base, R32G32_SFLOAT, st)
	UF_VERTEX_DESCRIPTION(uf::graph::mesh::Base, R32G32B32_SFLOAT, normal)
	UF_VERTEX_DESCRIPTION(uf::graph::mesh::Base, R32G32B32_SFLOAT, tangent)
	UF_VERTEX_DESCRIPTION(uf::graph::mesh::Base, R16G16_UINT, id)
);
UF_VERTEX_DESCRIPTOR(uf::graph::mesh::Skinned,
	UF_VERTEX_DESCRIPTION(uf::graph::mesh::Skinned, R32G32B32_SFLOAT, position)
	UF_VERTEX_DESCRIPTION(uf::graph::mesh::Skinned, R32G32_SFLOAT, uv)
	UF_VERTEX_DESCRIPTION(uf::graph::mesh::Skinned, R8G8B8A8_UNORM, color)
	UF_VERTEX_DESCRIPTION(uf::graph::mesh::Skinned, R32G32_SFLOAT, st)
	UF_VERTEX_DESCRIPTION(uf::graph::mesh::Skinned, R32G32B32_SFLOAT, normal)
	UF_VERTEX_DESCRIPTION(uf::graph::mesh::Skinned, R32G32B32_SFLOAT, tangent)
	UF_VERTEX_DESCRIPTION(uf::graph::mesh::Skinned, R16G16_UINT, id)
	UF_VERTEX_DESCRIPTION(uf::graph::mesh::Skinned, R16G16B16A16_UINT, joints)
	UF_VERTEX_DESCRIPTION(uf::graph::mesh::Skinned, R32G32B32A32_SFLOAT, weights)
);
#if UF_USE_FLOAT16
UF_VERTEX_DESCRIPTOR(uf::graph::mesh::Base_16f,
	UF_VERTEX_DESCRIPTION(uf::graph::mesh::Base_16f, R16G16B16_SFLOAT, position)
	UF_VERTEX_DESCRIPTION(uf::graph::mesh::Base_16f, R16G16_SFLOAT, uv)
	UF_VERTEX_DESCRIPTION(uf::graph::mesh::Base_16f, R8G8B8A8_UNORM, color)
	UF_VERTEX_DESCRIPTION(uf::graph::mesh::Base_16f, R16G16_SFLOAT, st)
	UF_VERTEX_DESCRIPTION(uf::graph::mesh::Base_16f, R16G16B16_SFLOAT, normal)
	UF_VERTEX_DESCRIPTION(uf::graph::mesh::Base_16f, R16G16B16_SFLOAT, tangent)
	UF_VERTEX_DESCRIPTION(uf::graph::mesh::Base_16f, R16G16_UINT, id)
);
UF_VERTEX_DESCRIPTOR(uf::graph::mesh::Skinned_16f,
	UF_VERTEX_DESCRIPTION(uf::graph::mesh::Skinned_16f, R16G16B16_SFLOAT, position)
	UF_VERTEX_DESCRIPTION(uf::graph::mesh::Skinned_16f, R16G16_SFLOAT, uv)
	UF_VERTEX_DESCRIPTION(uf::graph::mesh::Skinned_16f, R8G8B8A8_UNORM, color)
	UF_VERTEX_DESCRIPTION(uf::graph::mesh::Skinned_16f, R16G16_SFLOAT, st)
	UF_VERTEX_DESCRIPTION(uf::graph::mesh::Skinned_16f, R16G16B16_SFLOAT, normal)
	UF_VERTEX_DESCRIPTION(uf::graph::mesh::Skinned_16f, R16G16B16_SFLOAT, tangent)
	UF_VERTEX_DESCRIPTION(uf::graph::mesh::Skinned_16f, R16G16_UINT, id)
	UF_VERTEX_DESCRIPTION(uf::graph::mesh::Skinned_16f, R16G16B16A16_UINT, joints)
	UF_VERTEX_DESCRIPTION(uf::graph::mesh::Skinned_16f, R16G16B16A16_SFLOAT, weights)
);
#endif

UF_VERTEX_DESCRIPTOR(uf::graph::mesh::Base_u16q,
	UF_VERTEX_DESCRIPTION(uf::graph::mesh::Base_u16q, R16G16B16_UINT, position)
	UF_VERTEX_DESCRIPTION(uf::graph::mesh::Base_u16q, R16G16_UINT, uv)
	UF_VERTEX_DESCRIPTION(uf::graph::mesh::Base_u16q, R8G8B8A8_UNORM, color)
	UF_VERTEX_DESCRIPTION(uf::graph::mesh::Base_u16q, R16G16_UINT, st)
	UF_VERTEX_DESCRIPTION(uf::graph::mesh::Base_u16q, R16G16B16_UINT, normal)
	UF_VERTEX_DESCRIPTION(uf::graph::mesh::Base_u16q, R16G16B16_UINT, tangent)
	UF_VERTEX_DESCRIPTION(uf::graph::mesh::Base_u16q, R16G16_UINT, id)
);
UF_VERTEX_DESCRIPTOR(uf::graph::mesh::Skinned_u16q,
	UF_VERTEX_DESCRIPTION(uf::graph::mesh::Skinned_u16q, R16G16B16_UINT, position)
	UF_VERTEX_DESCRIPTION(uf::graph::mesh::Skinned_u16q, R16G16_UINT, uv)
	UF_VERTEX_DESCRIPTION(uf::graph::mesh::Skinned_u16q, R8G8B8A8_UNORM, color)
	UF_VERTEX_DESCRIPTION(uf::graph::mesh::Skinned_u16q, R16G16_UINT, st)
	UF_VERTEX_DESCRIPTION(uf::graph::mesh::Skinned_u16q, R16G16B16_UINT, normal)
	UF_VERTEX_DESCRIPTION(uf::graph::mesh::Skinned_u16q, R16G16B16_UINT, tangent)
	UF_VERTEX_DESCRIPTION(uf::graph::mesh::Skinned_u16q, R16G16_UINT, id)
	UF_VERTEX_DESCRIPTION(uf::graph::mesh::Skinned_u16q, R16G16B16A16_UINT, joints)
	UF_VERTEX_DESCRIPTION(uf::graph::mesh::Skinned_u16q, R16G16B16A16_UINT, weights)
);

pod::Matrix4f uf::graph::local( pod::Graph& graph, int32_t index ) {
	auto& node = 0 < index && index <= graph.nodes.size() ? graph.nodes[index] : graph.root;
	return
		uf::matrix::translate( uf::matrix::identity(), node.transform.position ) *
		uf::quaternion::matrix(node.transform.orientation) *
		uf::matrix::scale( uf::matrix::identity(), node.transform.scale ) *
		node.transform.model;
}
pod::Matrix4f uf::graph::matrix( pod::Graph& graph, int32_t index ) {
	pod::Matrix4f matrix = local( graph, index );
	auto& node = *uf::graph::find( graph, index );
	int32_t parent = node.parent;
	while ( 0 < parent && parent <= graph.nodes.size() ) {
		matrix = local( graph, parent ) * matrix;
		parent = graph.nodes[parent].parent;
	}
	return matrix;
}
pod::Node* uf::graph::find( pod::Graph& graph, int32_t index ) {
	return 0 <= index && index < graph.nodes.size() ? &graph.nodes[index] : NULL;
}
pod::Node* uf::graph::find( pod::Graph& graph, const uf::stl::string& name ) {
	for ( auto& node : graph.nodes ) if ( node.name == name ) return &node;
	return NULL;
}

void uf::graph::initializeGraphics( pod::Graph& graph, uf::Object& entity, uf::Mesh& mesh ) {
	auto& scene = uf::scene::getCurrentScene();
	auto& sceneTextures = scene.getComponent<pod::SceneTextures>();
	auto& sceneMetadataJson = scene.getComponent<uf::Serializer>();
	auto& storage = uf::graph::globalStorage ? uf::graph::storage : scene.getComponent<pod::Graph::Storage>();
	
	auto& graphic = entity.getComponent<uf::renderer::Graphic>();
	graphic.initialize();
	graphic.initializeMesh( mesh );

	graphic.device = &uf::renderer::device;
	graphic.material.device = &uf::renderer::device;
	graphic.descriptor.frontFace = graph.metadata["renderer"]["invert"].as<bool>(true) ? uf::renderer::enums::Face::CW : uf::renderer::enums::Face::CCW;
	graphic.descriptor.cullMode = uf::renderer::enums::CullMode::BACK;

	auto tag = ext::json::find( entity.getName(), graph.metadata["tags"] );
	if ( !ext::json::isObject( tag ) ) {
		tag["renderer"] = graph.metadata["renderer"];
	}

	if ( tag["renderer"]["front face"].is<uf::stl::string>() ) {
		const auto mode = uf::string::lowercase( tag["renderer"]["front face"].as<uf::stl::string>() );
		if ( mode == "cw" ) graphic.descriptor.frontFace = uf::renderer::enums::Face::CW;
		else if ( mode == "ccw" ) graphic.descriptor.frontFace = uf::renderer::enums::Face::CCW;
		else if ( mode == "auto" ) {
			if ( uf::matrix::reverseInfiniteProjection ) {
				graphic.descriptor.frontFace = graph.metadata["renderer"]["invert"].as<bool>(true) ? uf::renderer::enums::Face::CW : uf::renderer::enums::Face::CCW;
			} else {
				graphic.descriptor.frontFace = graph.metadata["renderer"]["invert"].as<bool>(true) ? uf::renderer::enums::Face::CW : uf::renderer::enums::Face::CCW;
			//	graphic.descriptor.frontFace = graph.metadata["renderer"]["invert"].as<bool>(true) ? uf::renderer::enums::Face::CCW : uf::renderer::enums::Face::CW;
			}
		}
		else UF_MSG_WARNING("Invalid Face enum string specified: {}", mode);
	}
	if ( tag["renderer"]["cull mode"].is<uf::stl::string>() ) {
		const auto mode = uf::string::lowercase( tag["renderer"]["cull mode"].as<uf::stl::string>() );
		if ( mode == "back" ) graphic.descriptor.cullMode = uf::renderer::enums::CullMode::BACK;
		else if ( mode == "front" ) graphic.descriptor.cullMode = uf::renderer::enums::CullMode::FRONT;
		else if ( mode == "none" ) graphic.descriptor.cullMode = uf::renderer::enums::CullMode::NONE;
		else if ( mode == "both" ) graphic.descriptor.cullMode = uf::renderer::enums::CullMode::BOTH;
		else UF_MSG_WARNING("Invalid CullMode enum string specified: {}", mode);
	}

/*
#if UF_USE_OPENGL
	if ( !uf::matrix::reverseInfiniteProjection ) {
		if ( graphic.descriptor.cullMode == uf::renderer::enums::CullMode::FRONT ) graphic.descriptor.cullMode = uf::renderer::enums::CullMode::BACK;
		if ( graphic.descriptor.cullMode == uf::renderer::enums::CullMode::BACK ) graphic.descriptor.cullMode = uf::renderer::enums::CullMode::FRONT;
	}
#endif
*/

	{
	//	for ( auto& s : graph.samplers ) graphic.material.samplers.emplace_back( storage.samplers.map[s] );
	//	for ( auto pair : storage.samplers.map ) graphic.material.samplers.emplace_back( pair.second );
	//	for ( auto& key : storage.samplers.keys ) graphic.material.samplers.emplace_back( storage.samplers.map[key] );

	//	for ( auto& i : graph.images ) graphic.material.textures.emplace_back().aliasTexture( storage.texture2Ds.map[i] );
	//	for ( auto pair : storage.texture2Ds.map ) graphic.material.textures.emplace_back().aliasTexture( pair.second );

		for ( auto& key : storage.texture2Ds.keys ) graphic.material.textures.emplace_back().aliasTexture( storage.texture2Ds.map[key] );

		// bind scene's voxel texture
	#if UF_USE_VULKAN
		if ( uf::renderer::settings::pipelines::vxgi ) {
			auto& scene = uf::scene::getCurrentScene();
			auto& sceneTextures = scene.getComponent<pod::SceneTextures>();
			for ( auto& t : sceneTextures.voxels.drawId ) graphic.material.textures.emplace_back().aliasTexture(t);
			for ( auto& t : sceneTextures.voxels.instanceId ) graphic.material.textures.emplace_back().aliasTexture(t);
			for ( auto& t : sceneTextures.voxels.normalX ) graphic.material.textures.emplace_back().aliasTexture(t);
			for ( auto& t : sceneTextures.voxels.normalY ) graphic.material.textures.emplace_back().aliasTexture(t);
			for ( auto& t : sceneTextures.voxels.radianceR ) graphic.material.textures.emplace_back().aliasTexture(t);
			for ( auto& t : sceneTextures.voxels.radianceG ) graphic.material.textures.emplace_back().aliasTexture(t);
			for ( auto& t : sceneTextures.voxels.radianceB ) graphic.material.textures.emplace_back().aliasTexture(t);
			for ( auto& t : sceneTextures.voxels.radianceA ) graphic.material.textures.emplace_back().aliasTexture(t);
			for ( auto& t : sceneTextures.voxels.count ) graphic.material.textures.emplace_back().aliasTexture(t);
			for ( auto& t : sceneTextures.voxels.output ) graphic.material.textures.emplace_back().aliasTexture(t);
		}
	#endif
	}
	
	uf::stl::string root = uf::io::directory( graph.name );
	size_t texture2Ds = 0;
	size_t texture3Ds = 0;
	for ( auto& texture : graphic.material.textures ) {
		if ( texture.width > 1 && texture.height > 1 && texture.depth == 1 && texture.layers == 1 ) ++texture2Ds;
		else if ( texture.width > 1 && texture.height > 1 && texture.depth > 1 && texture.layers == 1 ) ++texture3Ds;
	}

	// standard pipeline
	uf::stl::string vertexShaderFilename = graph.metadata["shaders"]["vertex"].as<uf::stl::string>("/graph/base/vert.spv"); {
		std::pair<bool, uf::stl::string> settings[] = {
			{ graph.metadata["renderer"]["skinned"].as<bool>(), "skinned.vert" },
			{ !graph.metadata["renderer"]["separate"].as<bool>(), "instanced.vert" },
		};
		FOR_ARRAY(settings) if ( settings[i].first ) vertexShaderFilename = uf::string::replace( vertexShaderFilename, "vert", settings[i].second );
		vertexShaderFilename = entity.resolveURI( vertexShaderFilename, root );
	}
	uf::stl::string geometryShaderFilename = graph.metadata["shaders"]["geometry"].as<uf::stl::string>("");
	uf::stl::string fragmentShaderFilename = graph.metadata["shaders"]["fragment"].as<uf::stl::string>("/graph/base/frag.spv"); {
	#if 0
		std::pair<bool, uf::stl::string> settings[] = {
			{ uf::renderer::settings::invariant::deferredSampling, "deferredSampling.frag" },
		};
		FOR_ARRAY(settings) if ( settings[i].first ) fragmentShaderFilename = uf::string::replace( fragmentShaderFilename, "frag", settings[i].second );
	#endif
		fragmentShaderFilename = entity.resolveURI( fragmentShaderFilename, root );
	}
	{
		graphic.material.metadata.autoInitializeUniformBuffers = false;
		graphic.material.attachShader(vertexShaderFilename, uf::renderer::enums::Shader::VERTEX);
		graphic.material.attachShader(fragmentShaderFilename, uf::renderer::enums::Shader::FRAGMENT);
		graphic.material.metadata.autoInitializeUniformBuffers = true;
	}

	uf::renderer::Buffer* indirect = NULL;
	for ( auto& buffer : graphic.buffers ) if ( !indirect && buffer.usage & uf::renderer::enums::Buffer::INDIRECT ) indirect = &buffer;

/*
	{
		auto& renderMode = uf::renderer::getRenderMode("", true);
		if ( !renderMode.hasBuffer("camera") ) {
			renderMode.metadata.buffers["camera"] = renderMode.initializeBuffer( (const void*) nullptr, sizeof(pod::Camera::Viewports), uf::renderer::enums::Buffer::UNIFORM );
		}
	}
*/
	{
		auto& shader = graphic.material.getShader("vertex");
	#if UF_USE_VULKAN
		shader.aliasBuffer( "camera", storage.buffers.camera );
	#else
		shader.buffers.emplace_back( storage.buffers.camera.alias() );
	#endif
//	//	shader.buffers.emplace_back( storage.buffers.drawCommands.alias() );
	#if UF_USE_VULKAN
		shader.buffers.emplace_back( indirect->alias() );
	#endif
		shader.buffers.emplace_back( storage.buffers.instance.alias() );
		shader.buffers.emplace_back( storage.buffers.joint.alias() );
	#if UF_USE_VULKAN
		uint32_t maxPasses = 6;
		shader.setSpecializationConstants({
			{ "PASSES", maxPasses }
		});
	#endif
	}
	{
		auto& shader = graphic.material.getShader("fragment");

		shader.buffers.emplace_back( storage.buffers.drawCommands.alias() );
		shader.buffers.emplace_back( storage.buffers.instance.alias() );
		shader.buffers.emplace_back( storage.buffers.instanceAddresses.alias() );
		shader.buffers.emplace_back( storage.buffers.material.alias() );
		shader.buffers.emplace_back( storage.buffers.texture.alias() );
		shader.buffers.emplace_back( storage.buffers.light.alias() );
	
	#if UF_USE_VULKAN
		uint32_t maxTextures = graph.textures.size(); // texture2Ds;
		shader.setSpecializationConstants({
			{ "TEXTURES", maxTextures },
		});
		shader.setDescriptorCounts({
			{ "samplerTextures", maxTextures },
		});
	#endif
	}
#if UF_USE_VULKAN
	// culling pipeline
	if ( uf::renderer::settings::pipelines::culling ) {
		uf::renderer::Buffer* indirect = NULL;
		for ( auto& buffer : graphic.buffers ) if ( !indirect && buffer.usage & uf::renderer::enums::Buffer::INDIRECT ) indirect = &buffer;
		UF_ASSERT( indirect );
		if ( indirect ) {
			uf::stl::string compShaderFilename = graph.metadata["shaders"][uf::renderer::settings::pipelines::names::culling]["compute"].as<uf::stl::string>("/graph/cull/comp.spv");
			{
				graphic.material.metadata.autoInitializeUniformBuffers = false;
				compShaderFilename = entity.resolveURI( compShaderFilename, root );
				graphic.material.attachShader(compShaderFilename, uf::renderer::enums::Shader::COMPUTE, uf::renderer::settings::pipelines::names::culling);
				graphic.material.metadata.autoInitializeUniformBuffers = true;
			}
			graphic.descriptor.bind.width = graphic.descriptor.inputs.indirect.count;
			graphic.descriptor.bind.height = 1;
			graphic.descriptor.bind.depth = 1;

			auto& shader = graphic.material.getShader("compute", uf::renderer::settings::pipelines::names::culling);

		//	shader.buffers.emplace_back( storage.buffers.camera.alias() );
			shader.aliasBuffer( "camera", storage.buffers.camera );
			shader.buffers.emplace_back( indirect->alias() );
			shader.buffers.emplace_back( storage.buffers.instance.alias() );

			shader.aliasAttachment("depthPyramid");
		}
	}
	if ( geometryShaderFilename != "" && uf::renderer::device.enabledFeatures.geometryShader ) {
		geometryShaderFilename = entity.resolveURI( geometryShaderFilename, root );
		graphic.material.attachShader(geometryShaderFilename, uf::renderer::enums::Shader::GEOMETRY);
	}
	// depth only pipeline
	{
		graphic.material.metadata.autoInitializeUniformBuffers = false;
		uf::stl::string fragmentShaderFilename = graph.metadata["shaders"]["depth"]["fragmment"].as<uf::stl::string>("/graph/depth/frag.spv");
		fragmentShaderFilename = entity.resolveURI( fragmentShaderFilename, root );
		graphic.material.attachShader(fragmentShaderFilename, uf::renderer::enums::Shader::FRAGMENT, "depth");
		graphic.material.metadata.autoInitializeUniformBuffers = true;
		{
			auto& shader = graphic.material.getShader("fragment", "depth");

			uint32_t maxTextures = graph.textures.size(); // texture2Ds;
			shader.setSpecializationConstants({
				{ "TEXTURES", maxTextures },
			});
			shader.setDescriptorCounts({
				{ "samplerTextures", maxTextures },
			});

			shader.buffers.emplace_back( storage.buffers.drawCommands.alias() );
			shader.buffers.emplace_back( storage.buffers.instance.alias() );
			shader.buffers.emplace_back( storage.buffers.instanceAddresses.alias() );
			shader.buffers.emplace_back( storage.buffers.material.alias() );
			shader.buffers.emplace_back( storage.buffers.texture.alias() );
			shader.buffers.emplace_back( storage.buffers.light.alias() );
		}
	}
	// vxgi pipeline
	if ( uf::renderer::settings::pipelines::vxgi ) {
		uf::stl::string vertexShaderFilename = graph.metadata["shaders"][uf::renderer::settings::pipelines::names::vxgi]["vertex"].as<uf::stl::string>("/graph/base/vert.spv");
		uf::stl::string geometryShaderFilename = graph.metadata["shaders"][uf::renderer::settings::pipelines::names::vxgi]["geometry"].as<uf::stl::string>("/graph/voxelize/geom.spv");
		uf::stl::string fragmentShaderFilename = graph.metadata["shaders"][uf::renderer::settings::pipelines::names::vxgi]["fragment"].as<uf::stl::string>("/graph/voxelize/frag.spv");

		{
			fragmentShaderFilename = entity.resolveURI( fragmentShaderFilename, root );
			graphic.material.metadata.autoInitializeUniformBuffers = false;
			graphic.material.attachShader(fragmentShaderFilename, uf::renderer::enums::Shader::FRAGMENT, uf::renderer::settings::pipelines::names::vxgi);
			graphic.material.metadata.autoInitializeUniformBuffers = true;
		}
		if ( geometryShaderFilename != "" && uf::renderer::device.enabledFeatures.geometryShader ) {
			geometryShaderFilename = entity.resolveURI( geometryShaderFilename, root );
			graphic.material.attachShader(geometryShaderFilename, uf::renderer::enums::Shader::GEOMETRY, uf::renderer::settings::pipelines::names::vxgi);
		}
		{
			uint32_t voxelTypes = 0;
			if ( !sceneTextures.voxels.drawId.empty() ) ++voxelTypes;
			if ( !sceneTextures.voxels.instanceId.empty() ) ++voxelTypes;
			if ( !sceneTextures.voxels.normalX.empty() ) ++voxelTypes;
			if ( !sceneTextures.voxels.normalY.empty() ) ++voxelTypes;
			if ( !sceneTextures.voxels.radianceR.empty() ) ++voxelTypes;
			if ( !sceneTextures.voxels.radianceG.empty() ) ++voxelTypes;
			if ( !sceneTextures.voxels.radianceB.empty() ) ++voxelTypes;
			if ( !sceneTextures.voxels.radianceA.empty() ) ++voxelTypes;
			if ( !sceneTextures.voxels.count.empty() ) ++voxelTypes;
			if ( !sceneTextures.voxels.output.empty() ) ++voxelTypes;

			uint32_t maxTextures = texture2Ds;
			uint32_t maxCascades = texture3Ds / voxelTypes;

			auto& shader = graphic.material.getShader("fragment", uf::renderer::settings::pipelines::names::vxgi);
			shader.setSpecializationConstants({
				{ "TEXTURES", maxTextures },
				{ "CASCADES", maxCascades },
			});
			shader.setDescriptorCounts({
				{ "samplerTextures", maxTextures },
				{ "voxelDrawId", maxCascades },
				{ "voxelInstanceId", maxCascades },
				{ "voxelNormalX", maxCascades },
				{ "voxelNormalY", maxCascades },
				{ "voxelRadianceR", maxCascades },
				{ "voxelRadianceG", maxCascades },
				{ "voxelRadianceB", maxCascades },
				{ "voxelRadianceA", maxCascades },
				{ "voxelCount", maxCascades },
				{ "voxelOutput", maxCascades },
			});
			
			shader.buffers.emplace_back( storage.buffers.drawCommands.alias() );
			shader.buffers.emplace_back( storage.buffers.instance.alias() );
			shader.buffers.emplace_back( storage.buffers.instanceAddresses.alias() );
			shader.buffers.emplace_back( storage.buffers.material.alias() );
			shader.buffers.emplace_back( storage.buffers.texture.alias() );
			shader.buffers.emplace_back( storage.buffers.light.alias() );
		}
	}
	// baking pipeline
	if ( graph.metadata["baking"]["enabled"].as<bool>() ) {
		{
			graphic.material.metadata.autoInitializeUniformBuffers = false;
			uf::stl::string vertexShaderFilename = uf::io::resolveURI("/graph/baking/vert.spv");
			uf::stl::string geometryShaderFilename = uf::io::resolveURI("/graph/baking/geom.spv");
			uf::stl::string fragmentShaderFilename = uf::io::resolveURI("/graph/baking/frag.spv");
			std::pair<bool, uf::stl::string> settings[] = {
				{ uf::renderer::settings::pipelines::rt, "rt.frag" },
			};
			FOR_ARRAY(settings) if ( settings[i].first ) fragmentShaderFilename = uf::string::replace( fragmentShaderFilename, "frag", settings[i].second );

			graphic.material.attachShader(vertexShaderFilename, uf::renderer::enums::Shader::VERTEX, "baking");
		//	graphic.material.attachShader(geometryShaderFilename, uf::renderer::enums::Shader::GEOMETRY, "baking");
			graphic.material.attachShader(fragmentShaderFilename, uf::renderer::enums::Shader::FRAGMENT, "baking");
			graphic.material.metadata.autoInitializeUniformBuffers = true;
		}
		{
			uint32_t maxPasses = 6;

			auto& shader = graphic.material.getShader("vertex", "baking");
			shader.setSpecializationConstants({
				{ "PASSES", maxPasses },
			});

		#if UF_USE_VULKAN
			shader.buffers.emplace_back( indirect->alias() );
		#endif
			shader.buffers.emplace_back( storage.buffers.instance.alias() );
			shader.buffers.emplace_back( storage.buffers.joint.alias() );
		}

		{
			size_t maxTextures = ext::config["engine"]["scenes"]["textures"]["max"]["2D"].as<size_t>(512);
			size_t maxCubemaps = ext::config["engine"]["scenes"]["textures"]["max"]["cube"].as<size_t>(128);
			size_t maxTextures3D = ext::config["engine"]["scenes"]["textures"]["max"]["3D"].as<size_t>(128);

			auto& shader = graphic.material.getShader("fragment", "baking");
			shader.setSpecializationConstants({
				{ "TEXTURES", maxTextures },
				{ "CUBEMAPS", maxCubemaps },
			});
			shader.setDescriptorCounts({
				{ "samplerTextures", maxTextures },
				{ "samplerCubemaps", maxCubemaps },
			});

		//	shader.buffers.emplace_back( storage.buffers.camera.alias() );
			shader.aliasBuffer( "camera", storage.buffers.camera );
			shader.buffers.emplace_back( storage.buffers.drawCommands.alias() );
			shader.buffers.emplace_back( storage.buffers.instance.alias() );
			shader.buffers.emplace_back( storage.buffers.instanceAddresses.alias() );
			shader.buffers.emplace_back( storage.buffers.material.alias() );
			shader.buffers.emplace_back( storage.buffers.texture.alias() );
			shader.buffers.emplace_back( storage.buffers.light.alias() );
		}
	}
	if ( uf::renderer::settings::pipelines::rt ) {
		// rt pipeline
		if ( mesh.vertex.count && graph.metadata["renderer"]["skinned"].as<bool>() ) {

			struct PushConstant {
				uint32_t jointID;
			};

			if ( mesh.isInterleaved( mesh.vertex ) ) {
				auto& vertexSourceData = mesh.buffers[mesh.vertex.interleaved];
				size_t vertexSourceDataIndex = graphic.initializeBuffer( (const void*) vertexSourceData.data(), vertexSourceData.size(), VK_BUFFER_USAGE_STORAGE_BUFFER_BIT | VK_BUFFER_USAGE_SHADER_DEVICE_ADDRESS_BIT | VK_BUFFER_USAGE_ACCELERATION_STRUCTURE_BUILD_INPUT_READ_ONLY_BIT_KHR );
				
			/*
				auto& vertexIn = graphic.buffers.at(vertexSourceDataIndex);
				auto& vertexOut = graphic.buffers.at(graphic.descriptor.inputs.vertex.interleaved);
			*/
				auto& vertexIn = graphic.buffers.at(graphic.descriptor.inputs.vertex.interleaved);
				auto& vertexOut = graphic.buffers.at(vertexSourceDataIndex);
				graphic.metadata.buffers["vertexSkinned"] = vertexSourceDataIndex;

				uf::stl::string compShaderFilename = graph.metadata["shaders"]["skinning"]["compute"].as<uf::stl::string>("/graph/skinning/skinning.interleaved.comp.spv");
				{
					graphic.material.metadata.autoInitializeUniformBuffers = false;
					compShaderFilename = entity.resolveURI( compShaderFilename, root );
					graphic.material.attachShader(compShaderFilename, uf::renderer::enums::Shader::COMPUTE, "skinning");
					graphic.material.metadata.autoInitializeUniformBuffers = true;
				}
				graphic.descriptor.bind.width = mesh.vertex.count;
				graphic.descriptor.bind.height = 1;
				graphic.descriptor.bind.depth = 1;

				auto& shader = graphic.material.getShader("compute", "skinning");

				struct {
					uint32_t jointID;
				} uniforms = {
					.jointID = 0
				};

				shader.updateBuffer( (const void*) &uniforms, sizeof(uniforms), shader.getUniformBuffer("UBO") );

				shader.buffers.emplace_back( storage.buffers.joint.alias() );
				shader.buffers.emplace_back( vertexIn.alias() );
				shader.buffers.emplace_back( vertexOut.alias() );
			} else {
				uf::Mesh::Attribute vertexPos;
				uf::Mesh::Attribute vertexJoints;
				uf::Mesh::Attribute vertexWeights;

				size_t vertexPosIndex = 0;
				size_t vertexJointsIndex = 0;
				size_t vertexWeightsIndex = 0;

				for ( size_t i = 0; i < graphic.descriptor.inputs.vertex.attributes.size(); ++i ) {
					auto& attribute = graphic.descriptor.inputs.vertex.attributes[i];

					if ( attribute.buffer < 0 ) continue;
					if ( attribute.descriptor.name == "position" ) {
						vertexPos = attribute; 
						vertexPosIndex = graphic.metadata.buffers["vertex[position]"];
					}
					else if ( attribute.descriptor.name == "joints" ) {
						vertexJoints = attribute;
						vertexJointsIndex = graphic.metadata.buffers["vertex[joints]"];
					}
					else if ( attribute.descriptor.name == "weights" ) {
						vertexWeights = attribute;
						vertexWeightsIndex = graphic.metadata.buffers["vertex[weights]"];
					}
				}

				auto& vertexSourceData = mesh.buffers[vertexPos.buffer];
				size_t vertexSourceDataIndex = graphic.initializeBuffer( (const void*) vertexSourceData.data(), vertexSourceData.size(), VK_BUFFER_USAGE_STORAGE_BUFFER_BIT | VK_BUFFER_USAGE_SHADER_DEVICE_ADDRESS_BIT | VK_BUFFER_USAGE_ACCELERATION_STRUCTURE_BUILD_INPUT_READ_ONLY_BIT_KHR );

			/*
				auto& vertexPositionBuffer = graphic.buffers.at(vertexSourceDataIndex);
				auto& vertexJointsBuffer = graphic.buffers.at(vertexJointsIndex);
				auto& vertexWeightsBuffer = graphic.buffers.at(vertexWeightsIndex);

				auto& vertexOutPosition = graphic.buffers.at(vertexPosIndex);
			*/
				auto& vertexPositionBuffer = graphic.buffers.at(vertexPosIndex);
				auto& vertexJointsBuffer = graphic.buffers.at(vertexJointsIndex);
				auto& vertexWeightsBuffer = graphic.buffers.at(vertexWeightsIndex);

				auto& vertexOutPosition = graphic.buffers.at(vertexSourceDataIndex);
				graphic.metadata.buffers["vertexSkinned"] = vertexSourceDataIndex;

				uf::stl::string compShaderFilename = graph.metadata["shaders"]["skinning"]["compute"].as<uf::stl::string>("/graph/skinning/skinning.deinterleaved.comp.spv");
				{
				//	graphic.material.metadata.autoInitializeUniformBuffers = false;
					compShaderFilename = entity.resolveURI( compShaderFilename, root );
					graphic.material.attachShader(compShaderFilename, uf::renderer::enums::Shader::COMPUTE, "skinning");
				//	graphic.material.metadata.autoInitializeUniformBuffers = true;
				}
				graphic.descriptor.bind.width = mesh.vertex.count;
				graphic.descriptor.bind.height = 1;
				graphic.descriptor.bind.depth = 1;

				auto& shader = graphic.material.getShader("compute", "skinning");

				struct {
					uint32_t jointID;
				} uniforms = {
					.jointID = 0
				};

				shader.updateBuffer( (const void*) &uniforms, sizeof(uniforms), shader.getUniformBuffer("UBO") );

				shader.buffers.emplace_back( storage.buffers.joint.alias() );
				shader.buffers.emplace_back( vertexPositionBuffer.alias() );
				shader.buffers.emplace_back( vertexJointsBuffer.alias() );
				shader.buffers.emplace_back( vertexWeightsBuffer.alias() );
				shader.buffers.emplace_back( vertexOutPosition.alias() );
			}
		}
		graphic.generateBottomAccelerationStructures();
	}

	// grab addresses
	if ( uf::renderer::settings::invariant::deviceAddressing ) {
		pod::DrawCommand* drawCommands = (pod::DrawCommand*) mesh.getBuffer( mesh.indirect ).data();
		for ( size_t drawID = 0; drawID < mesh.indirect.count; ++drawID ) {
			auto& drawCommand = drawCommands[drawID];
			auto instanceID = drawCommand.instanceID;
			auto instanceKeyName = std::to_string(instanceID);

		/*
			if ( storage.instanceAddresses.map.count(instanceKeyName) > 0 ) {
				auto objectKeyName = std::to_string(objectID);
				instanceKeyName = objectKeyName + "+" + instanceKeyName;
			}
		*/
			if ( storage.instanceAddresses.map.count(instanceKeyName) > 0 ) {
			//	UF_MSG_ERROR("DUPLICATE INSTANCE ID: {}", instanceKeyName);
			}

			auto& instanceAddresses = storage.instanceAddresses.map[instanceKeyName];
			if ( mesh.vertex.count ) {
				if ( mesh.isInterleaved( mesh.vertex ) ) {
					instanceAddresses.vertex = graphic.buffers.at(graphic.descriptor.inputs.vertex.interleaved).getAddress()/* + (drawCommand.vertexID * mesh.vertex.size)*/;
				} else {
					for ( auto& attribute : graphic.descriptor.inputs.vertex.attributes ) {
						if ( attribute.buffer < 0 ) continue;
						if ( attribute.descriptor.name == "position" ) 		instanceAddresses.position = graphic.buffers.at(attribute.buffer).getAddress()/* + (drawCommand.vertexID * attribute.stride)*/;
						else if ( attribute.descriptor.name == "uv" ) 		instanceAddresses.uv = graphic.buffers.at(attribute.buffer).getAddress()/* + (drawCommand.vertexID * attribute.stride)*/;
						else if ( attribute.descriptor.name == "color" ) 	instanceAddresses.color = graphic.buffers.at(attribute.buffer).getAddress()/* + (drawCommand.vertexID * attribute.stride)*/;
						else if ( attribute.descriptor.name == "st" ) 		instanceAddresses.st = graphic.buffers.at(attribute.buffer).getAddress()/* + (drawCommand.vertexID * attribute.stride)*/;
						else if ( attribute.descriptor.name == "normal" ) 	instanceAddresses.normal = graphic.buffers.at(attribute.buffer).getAddress()/* + (drawCommand.vertexID * attribute.stride)*/;
						else if ( attribute.descriptor.name == "tangent" ) 	instanceAddresses.tangent = graphic.buffers.at(attribute.buffer).getAddress()/* + (drawCommand.vertexID * attribute.stride)*/;
						else if ( attribute.descriptor.name == "joints" ) 	instanceAddresses.joints = graphic.buffers.at(attribute.buffer).getAddress()/* + (drawCommand.vertexID * attribute.stride)*/;
						else if ( attribute.descriptor.name == "weights" ) 	instanceAddresses.weights = graphic.buffers.at(attribute.buffer).getAddress()/* + (drawCommand.vertexID * attribute.stride)*/;
						else if ( attribute.descriptor.name == "id" ) 		instanceAddresses.id = graphic.buffers.at(attribute.buffer).getAddress()/* + (drawCommand.vertexID * attribute.stride)*/;
					}
				}
			}
			if ( mesh.index.count ) {
				if ( mesh.isInterleaved( mesh.index ) ) instanceAddresses.index = graphic.buffers.at(graphic.descriptor.inputs.index.interleaved).getAddress();
				else instanceAddresses.index = graphic.buffers.at(graphic.descriptor.inputs.index.attributes.front().buffer).getAddress();
			}

			if ( mesh.indirect.count ) {
				if ( mesh.isInterleaved( mesh.indirect ) ) instanceAddresses.indirect = graphic.buffers.at(graphic.descriptor.inputs.indirect.interleaved).getAddress();
				else instanceAddresses.indirect = graphic.buffers.at(graphic.descriptor.inputs.indirect.attributes.front().buffer).getAddress();

				instanceAddresses.drawID = drawID;
			}
		}
	}
#endif

	graphic.process = true;
}

void uf::graph::process( pod::Graph& graph ) {
	UF_DEBUG_TIMER_MULTITRACE_START("Processing {}", graph.name);

	//
	if ( !graph.root.entity ) graph.root.entity = new uf::Object;
	
	// copy lighting settings from graph
	auto& scene = uf::scene::getCurrentScene();
	auto& sceneMetadataJson = scene.getComponent<uf::Serializer>();
	auto& storage = uf::graph::globalStorage ? uf::graph::storage : scene.getComponent<pod::Graph::Storage>();

#if 1
	// merge light settings with global settings
	{
		const auto& globalSettings = graph.metadata["light"];
		ext::json::forEach( globalSettings, [&]( const uf::stl::string& key, const ext::json::Value& value ){
			if ( !ext::json::isNull( sceneMetadataJson["light"][key] ) ) return;
			sceneMetadataJson["light"][key] = value;
		} );
	}
	// merge bloom settings with global settings
	{
		const auto& globalSettings = graph.metadata["light"]["bloom"];
		ext::json::forEach( globalSettings, [&]( const uf::stl::string& key, const ext::json::Value& value ){
			if ( !ext::json::isNull( sceneMetadataJson["light"]["bloom"][key] ) ) return;
			sceneMetadataJson["light"]["bloom"][key] = value;
		} );
	}
	// merge shadows settings with global settings
	{
		const auto& globalSettings = graph.metadata["light"]["shadows"];
		ext::json::forEach( globalSettings, [&]( const uf::stl::string& key, const ext::json::Value& value ){
			if ( !ext::json::isNull( sceneMetadataJson["light"]["shadows"][key] ) ) return;
			sceneMetadataJson["light"]["shadows"][key] = value;
		} );
	}
	// merge fog settings with global settings
	{
		const auto& globalSettings = graph.metadata["light"]["fog"];
		ext::json::forEach( globalSettings, [&]( const uf::stl::string& key, const ext::json::Value& value ){
			if ( !ext::json::isNull( sceneMetadataJson["light"]["fog"][key] ) ) return;
			sceneMetadataJson["light"]["fog"][key] = value;
		} );
	}
#endif
#if 0
	if ( ext::json::isObject(graph.metadata["assets"]) || ext::json::isArray(graph.metadata["assets"]) ) {
		scene.loadAssets(graph.metadata["assets"]);
	}
#endif

	//
	uf::stl::unordered_map<uf::stl::string, bool> isSrgb;

	// process lightmap

	UF_DEBUG_TIMER_MULTITRACE("Parsing lightmaps");
	if ( true ) {
		constexpr const char* UF_GRAPH_DEFAULT_LIGHTMAP = "./lightmap.%i.png";
		uf::stl::unordered_map<size_t, uf::stl::string> filenames;
		uf::stl::unordered_map<size_t, size_t> lightmapIDs;
		uint32_t lightmapCount = 0;

		for ( auto& name : graph.instances ) {
			auto& instance = storage.instances[name];
			filenames[instance.auxID] = uf::string::replace(UF_GRAPH_DEFAULT_LIGHTMAP, "%i", std::to_string(instance.auxID));

			lightmapCount = std::max( lightmapCount, instance.auxID + 1 );
		}
		for ( auto& name : graph.primitives ) {
			auto& primitives = storage.primitives[name];
			for ( auto& primitive : primitives ) {
				filenames[primitive.instance.auxID] = uf::string::replace(UF_GRAPH_DEFAULT_LIGHTMAP, "%i", std::to_string(primitive.instance.auxID));

				lightmapCount = std::max( lightmapCount, primitive.instance.auxID + 1 );
			}
		}

		if ( graph.metadata["lights"]["lightmap"].is<bool>() && !graph.metadata["lights"]["lightmap"].as<bool>() ) {
			graph.metadata["baking"]["enabled"] = false;
		}
		if ( !sceneMetadataJson["light"]["useLightmaps"].as<bool>(true) ) {
			graph.metadata["lights"]["lightmap"] = false;
			graph.metadata["baking"]["enabled"] = false;
		}

		if ( graph.metadata["lights"]["lightmap"].is<uf::stl::string>() && graph.metadata["lights"]["lightmap"].as<uf::stl::string>() == "auto" ) {
			uint32_t mtime = uf::io::mtime( graph.name );
			// lightmaps are considered stale if they're older than the graph's source
			bool stale = false;
			for ( auto& pair : filenames ) {
				uf::stl::string filename = uf::io::sanitize( pair.second, uf::io::directory( graph.name ) );
				auto time = uf::io::mtime(filename);
				if ( !uf::io::exists( filename ) ) continue;
				if ( time < mtime ) {
					UF_MSG_INFO("Stale lightmap detected, disabling use of lightmaps: {}", filename);
					stale = true;
					break;
				}
			}
			graph.metadata["lights"]["lightmap"] = !stale;
		}


		graph.metadata["baking"]["layers"] = lightmapCount;
		
		if ( graph.metadata["lights"]["lightmap"].as<bool>() ) {
			for ( auto& pair : filenames ) {
				auto i = pair.first;
				auto f = uf::io::sanitize( pair.second, uf::io::directory( graph.name ) );
				if ( !uf::io::exists( f ) ) {
					graph.metadata["lights"]["lightmap"] = false;
					UF_MSG_ERROR( "lightmap does not exist: {} {}, disabling lightmaps", i, f )
					break;
				}
			}
		}
		if ( graph.metadata["lights"]["lightmap"].as<bool>() ) {
			for ( auto& pair : filenames ) {
				auto i = pair.first;
				auto f = uf::io::sanitize( pair.second, uf::io::directory( graph.name ) );

				auto textureID = graph.textures.size();
				auto imageID = graph.images.size();

				auto& texture = /*graph.storage*/storage.textures[graph.textures.emplace_back(f)];
				auto& image = /*graph.storage*/storage.images[graph.images.emplace_back(f)];
				image.open( f, false );

				texture.index = imageID;

				lightmapIDs[i] = textureID;

				graph.metadata["lights"]["lightmaps"][i] = f;
				graph.metadata["baking"]["enabled"] = false;

				isSrgb[f] = false;
			}
		}
				
		for ( auto& name : graph.instances ) {
			auto& instance = storage.instances[name];
			if ( lightmapIDs.count( instance.auxID ) == 0 ) continue;
			instance.lightmapID = lightmapIDs[instance.auxID];
		}
		for ( auto& name : graph.primitives ) {
			auto& primitives = storage.primitives[name];
			for ( auto& primitive : primitives ) {
				if ( lightmapIDs.count( primitive.instance.auxID ) == 0 ) continue;
				primitive.instance.lightmapID = lightmapIDs[primitive.instance.auxID];
			}
		}
	}
	// add atlas


	// setup textures
	storage.texture2Ds.reserve( storage.images.map.size() );

	// not having this block will cause our images and texture2Ds to be ordered out of sync
	for ( auto& key : graph.images ) {
		storage.images[key];
		storage.texture2Ds[key];
	}

	// figure out what texture is what exactly
	UF_DEBUG_TIMER_MULTITRACE("Determining format of textures");
	for ( auto& key : graph.materials ) {
		auto& material = storage.materials[key];
		auto ID = material.indexAlbedo;

		if ( !(0 <= ID && ID < graph.textures.size()) ) continue;

		auto texName = graph.textures[ID];
		isSrgb[texName] = true;
	}

	for ( auto& key : graph.images ) {
		auto& image = storage.images[key];
		auto& texture = storage.texture2Ds[key];
		if ( !texture.generated() ) {
			auto filter = uf::renderer::enums::Filter::LINEAR;
			auto tag = ext::json::find( key, graph.metadata["tags"] );
			if ( !ext::json::isObject( tag ) ) {
				tag["renderer"] = graph.metadata["renderer"];
			}
			if ( tag["renderer"]["filter"].is<uf::stl::string>() ) {
				const auto mode = uf::string::lowercase( tag["renderer"]["filter"].as<uf::stl::string>("linear") );
				if ( mode == "linear" ) filter = uf::renderer::enums::Filter::LINEAR;
				else if ( mode == "nearest" ) filter = uf::renderer::enums::Filter::NEAREST;
				else UF_MSG_WARNING("Invalid Filter enum string specified: {}", mode);
			}

			texture.sampler.descriptor.filter.min = filter;
			texture.sampler.descriptor.filter.mag = filter;
			texture.srgb = isSrgb.count(key) == 0 ? false : isSrgb[key];

			texture.loadFromImage( image );
		#if UF_ENV_DREAMCAST
			image.clear();
		#endif
		}
	}	

	
	// process nodes
	UF_DEBUG_TIMER_MULTITRACE("Processing nodes");
	for ( auto index : graph.root.children ) {
		process( graph, index, *graph.root.entity );

		auto& node = graph.nodes[index];
		if ( node.entity ) UF_DEBUG_TIMER_MULTITRACE("Processed node: {}", node.name);
	}

	// patch materials/textures
	UF_DEBUG_TIMER_MULTITRACE("Remapping patching/textures");
	for ( auto& name : graph.materials ) {
		auto& material = storage.materials[name];
		auto tag = ext::json::find( name, graph.metadata["tags"] );
		if ( ext::json::isObject( tag ) ) {
			material.colorBase = uf::vector::decode( tag["material"]["base"], material.colorBase);
			material.colorEmissive = uf::vector::decode( tag["material"]["emissive"], material.colorEmissive);
			material.factorMetallic = tag["material"]["fMetallic"].as(material.factorMetallic);
			material.factorRoughness = tag["material"]["fRoughness"].as(material.factorRoughness);
			material.factorOcclusion = tag["material"]["fOcclusion"].as(material.factorOcclusion);
			material.factorAlphaCutoff = tag["material"]["fAlphaCutoff"].as(material.factorAlphaCutoff);
			if ( tag["material"]["iAlbedo"].is<uf::stl::string>() ) {
				auto keyName = tag["material"]["iAlbedo"].as<uf::stl::string>();
				if ( storage.textures.map.count(keyName) > 0 ) {
					auto& texture = storage.textures[keyName];
					material.indexAlbedo = tag["material"]["iAlbedo"].as(texture.index);
				}
			} else {
				material.indexAlbedo = tag["material"]["iAlbedo"].as(material.indexAlbedo);
			}
			if ( tag["material"]["iNormal"].is<uf::stl::string>() ) {
				auto keyName = tag["material"]["iNormal"].as<uf::stl::string>();
				if ( storage.textures.map.count(keyName) > 0 ) {
					auto& texture = storage.textures[keyName];
					material.indexNormal = tag["material"]["iNormal"].as(texture.index);
				}
			} else {
				material.indexNormal = tag["material"]["iNormal"].as(material.indexNormal);
			}
			if ( tag["material"]["iEmissive"].is<uf::stl::string>() ) {
				auto keyName = tag["material"]["iEmissive"].as<uf::stl::string>();
				if ( storage.textures.map.count(keyName) > 0 ) {
					auto& texture = storage.textures[keyName];
					material.indexEmissive = tag["material"]["iEmissive"].as(texture.index);
				}
			} else {
				material.indexEmissive = tag["material"]["iEmissive"].as(material.indexEmissive);
			}
			if ( tag["material"]["iOcclusion"].is<uf::stl::string>() ) {
				auto keyName = tag["material"]["iOcclusion"].as<uf::stl::string>();
				if ( storage.textures.map.count(keyName) > 0 ) {
					auto& texture = storage.textures[keyName];
					material.indexOcclusion = tag["material"]["iOcclusion"].as(texture.index);
				}
			} else {
				material.indexOcclusion = tag["material"]["iOcclusion"].as(material.indexOcclusion);
			}
			if ( tag["material"]["iMetallicRoughness"].is<uf::stl::string>() ) {
				auto keyName = tag["material"]["iMetallicRoughness"].as<uf::stl::string>();
				if ( storage.textures.map.count(keyName) > 0 ) {
					auto& texture = storage.textures[keyName];
					material.indexMetallicRoughness = tag["material"]["iMetallicRoughness"].as(texture.index);
				}
			} else {
				material.indexMetallicRoughness = tag["material"]["iMetallicRoughness"].as(material.indexMetallicRoughness);
			}
			
			if ( tag["material"]["modeAlpha"].is<uf::stl::string>() ) {
				const auto mode = uf::string::lowercase( tag["material"]["modeAlpha"].as<uf::stl::string>() );
				if ( mode == "opaque" ) material.modeAlpha = 0;
				else if ( mode == "blend" ) material.modeAlpha = 1;
				else if ( mode == "mask" ) material.modeAlpha = 2;
				else UF_MSG_WARNING("Invalid AlphaMode enum string specified: {}", mode);
			} else {
				material.modeAlpha = tag["material"]["modeAlpha"].as(material.modeAlpha);
			}
		}
	}

	// remap textures->images IDs
	UF_DEBUG_TIMER_MULTITRACE("Remapping textures");
	for ( auto& name : graph.textures ) {
		auto& texture = storage.textures[name];
		auto& keys = storage.images.keys;
		auto& indices = storage.images.indices;

		if ( !(0 <= texture.index && texture.index < graph.images.size()) ) continue;

		auto& needle = graph.images[texture.index];
	#if 1
		texture.index = indices[needle];
	#elif 1
		for ( size_t i = 0; i < keys.size(); ++i ) {
			if ( keys[i] != needle ) continue;
			texture.index = i;
			break;
		}
	#else
		auto it = std::find( keys.begin(), keys.end(), needle );
		UF_ASSERT( it != keys.end() );
		texture.index = it - keys.begin();
	#endif
	}

	// remap materials->texture IDs
	UF_DEBUG_TIMER_MULTITRACE("Remapping materials");
	for ( auto& name : graph.materials ) {
		auto& material = storage.materials[name];
		auto& keys = storage.textures.keys;
		auto& indices = storage.textures.indices;
		int32_t* IDs[] = { &material.indexAlbedo, &material.indexNormal, &material.indexEmissive, &material.indexOcclusion, &material.indexMetallicRoughness };
		for ( auto* pointer : IDs ) {
			auto& ID = *pointer;
			if ( !(0 <= ID && ID < graph.textures.size()) ) continue;
			auto& needle = graph.textures[ID];
		#if 1
			ID = indices[needle];
		#elif 1
			for ( size_t i = 0; i < keys.size(); ++i ) {
				if ( keys[i] != needle ) continue;
				ID = i;
				break;
			}
		#else
			if ( !(0 <= ID && ID < graph.textures.size()) ) continue;
			auto it = std::find( keys.begin(), keys.end(), needle );
			UF_ASSERT( it != keys.end() );
			ID = it - keys.begin();
		#endif
		}
	}
	// remap instance variables
	UF_DEBUG_TIMER_MULTITRACE("Remapping instances");
	for ( auto& name : graph.instances ) {
		auto& instance = storage.instances[name];
	//	auto& instanceAddresses = storage.instanceAddresses[name];
	//	instance.addresses = instanceAddresses;
		
		if ( 0 <= instance.materialID && instance.materialID < graph.materials.size() ) {
			auto& keys = /*graph.storage*/storage.materials.keys;
			auto& indices = /*graph.storage*/storage.materials.indices;
			
			if ( !(0 <= instance.materialID && instance.materialID < graph.materials.size()) ) continue;

			auto& needle = graph.materials[instance.materialID];
		#if 1
			instance.materialID = indices[needle];
		#elif 1
			for ( size_t i = 0; i < keys.size(); ++i ) {
				if ( keys[i] != needle ) continue;
				instance.materialID = i;
				break;
			}
		#else
			auto it = std::find( keys.begin(), keys.end(), needle );
			UF_ASSERT( it != keys.end() );
			instance.materialID = it - keys.begin();
		#endif
		}
		if ( 0 <= instance.lightmapID && instance.lightmapID < graph.textures.size() ) {
			auto& keys = /*graph.storage*/storage.textures.keys;
			auto& indices = /*graph.storage*/storage.textures.indices;

			if ( !(0 <= instance.lightmapID && instance.lightmapID < graph.textures.size()) ) continue;

			auto& needle = graph.textures[instance.lightmapID];
		#if 1
			instance.lightmapID = indices[needle];
		#elif 1
			for ( size_t i = 0; i < keys.size(); ++i ) {
				if ( keys[i] != needle ) continue;
				instance.lightmapID = i;
				break;
			}
		#else
			auto it = std::find( keys.begin(), keys.end(), needle );
			UF_ASSERT( it != keys.end() );
			instance.lightmapID = it - keys.begin();
		#endif
		}
	#if 0
		// i genuinely dont remember what this is used for

		if ( 0 <= instance.imageID && instance.imageID < graph.images.size() ) {
			auto& keys = /*graph.storage*/storage.images.keys;
			auto it = std::find( keys.begin(), keys.end(), graph.images[instance.imageID] );
			UF_ASSERT( it != keys.end() );
			instance.imageID = it - keys.begin();
		}
	#endif
		// remap a skinID as an actual jointID
		if ( 0 <= instance.jointID && instance.jointID < graph.skins.size() ) {
			auto& name = graph.skins[instance.jointID];
			instance.jointID = 0;
			for ( auto key : storage.joints.keys ) {
				if ( key == name ) break;
				auto& joints = storage.joints[key];
				instance.jointID += joints.size();
			}
		}
	}

	// remap instance variables
#if 0
	UF_DEBUG_TIMER_MULTITRACE("Remapping drawCommands");
	for ( auto& name : graph.drawCommands ) {
		auto& drawCommands = storage.drawCommands[name];
		for ( auto& drawCommand : drawCommands ) {
			if ( 0 <= drawCommand.instanceID && drawCommand.instanceID < graph.instances.size() ) {
				auto& keys = /*graph.storage*/storage.instances.keys;
				auto& indices = /*graph.storage*/storage.instances.indices;
				
				if ( !(0 <= drawCommand.instanceID && drawCommand.instanceID < graph.instances.size()) ) continue;

				auto& needle = graph.instances[drawCommand.instanceID];
			#if 1
				drawCommand.instanceID = indices[needle];
			#elif 1
				for ( size_t i = 0; i < keys.size(); ++i ) {
					if ( keys[i] != needle ) continue;
					drawCommand.instanceID = i;
					break;
				}
			#else
				auto it = std::find( keys.begin(), keys.end(), needle );
				UF_ASSERT( it != keys.end() );
				drawCommand.instanceID = it - keys.begin();
			#endif
			}
		}
	}
#endif

	if ( graph.metadata["debug"]["print"]["lights"].as<bool>() ) {
		UF_MSG_DEBUG("Lights: {}", graph.lights.size());
		for ( auto& pair : graph.lights ) {
			UF_MSG_DEBUG("\tLight: {}", pair.first);
		}
	}
	if ( graph.metadata["debug"]["print"]["meshes"].as<bool>() ) {
		UF_MSG_DEBUG("Meshs: {}", graph.meshes.size());
		for ( auto& name : graph.meshes ) {
			UF_MSG_DEBUG("\tMesh: {}", name);
		}
	}

	if ( graph.metadata["debug"]["print"]["instances"].as<bool>() ) {
		UF_MSG_DEBUG("Instances: {}", graph.instances.size());
		for ( auto& name : graph.instances ) {
			auto& instance = storage.instances[name];
			UF_MSG_DEBUG("\tInstance: {} | {} | {}", name,
				instance.materialID,
				instance.lightmapID
			);
		}
	}
	if ( graph.metadata["debug"]["print"]["materials"].as<bool>() ) {
		UF_MSG_DEBUG("Materials: {}", graph.materials.size());
		for ( auto& name : graph.materials ) {
			auto& material = storage.materials[name];
			UF_MSG_DEBUG("\tMaterial: {} | {}", name, material.indexAlbedo);
		}
	}
	if ( graph.metadata["debug"]["print"]["textures"].as<bool>() ) {
		UF_MSG_DEBUG("Textures: {}", graph.textures.size());
		for ( auto& name : graph.textures ) {
			auto& texture = storage.textures[name];
			UF_MSG_DEBUG("\tTexture: {} | {}", name, texture.index);
		}
	}
	if ( graph.metadata["debug"]["print"]["images"].as<bool>() ) {
		UF_MSG_DEBUG("Images: {}", graph.images.size());
		for ( auto& name : graph.images ) {
			UF_MSG_DEBUG("\tImage: {}", name);
		}
	}

	UF_DEBUG_TIMER_MULTITRACE("Updating master graph");
	uf::graph::reload();

	// setup combined mesh if requested
	if ( !(graph.metadata["renderer"]["separate"].as<bool>()) ) {
		UF_DEBUG_TIMER_MULTITRACE("Processing root graphic");

		graph.root.mesh = graph.meshes.size();
		auto keyName = graph.name + "/" + graph.root.name;
		auto& mesh = storage.meshes[graph.meshes.emplace_back(keyName)];

		mesh.bindIndirect<pod::DrawCommand>();
		#if UF_ENV_DREAMCAST
			mesh.bind<uf::graph::mesh::Base, uint32_t>();
		#else
			mesh.bind<uf::graph::mesh::Skinned, uint32_t>();
		#endif
	/*
		{
		#if UF_ENV_DREAMCAST
			mesh.bind<uf::graph::mesh::Base_u16q, uint32_t>();
		//	mesh.convert<float, uint16_t>();
		#else
			auto conversion = graph.metadata["decode"]["conversion"].as<uf::stl::string>();
			if ( conversion != "" ) {
				if ( conversion == "uint16_t" ) mesh.bind<uf::graph::mesh::Skinned_16f, uint32_t>();
			#if UF_USE_FLOAT16
				else if ( conversion == "float16" ) mesh.bind<uf::graph::mesh::Skinned_16f, uint32_t>();
			#endif
			#if UF_USE_BFLOAT16
				else if ( conversion == "bfloat16" ) mesh.bind<uf::graph::mesh::Skinned_16f, uint32_t>();
			#endif
				else mesh.bind<uf::graph::mesh::Skinned, uint32_t>();
			} else mesh.bind<uf::graph::mesh::Skinned, uint32_t>();
		#endif
		}
	*/

		uf::stl::vector<pod::DrawCommand> drawCommands;
		
		size_t counts = 0;
		for ( auto& name : graph.meshes ) {
			if ( name == keyName ) continue;
			auto tag = ext::json::find( name, graph.metadata["tags"] );
			if ( ext::json::isObject( tag ) ) {
				if ( tag["ignore"].as<bool>() ) continue;
			}

			auto& m = storage.meshes.map[name];
			m.updateDescriptor();

			mesh.insertVertices( m );
			mesh.insertIndices( m );
			mesh.insertInstances( m );

		//	mesh.insertIndirects( m );
			pod::DrawCommand* drawCommand = (pod::DrawCommand*) m.getBuffer( m.indirect ).data();
			for ( size_t i = 0; i < m.indirect.count; ++i ) drawCommands.emplace_back( drawCommand[i] );
		}

		// fix up draw command for combined mesh
		{
			size_t totalIndices = 0;
			size_t totalVertices = 0;
			for ( auto& drawCommand : drawCommands ) {
				drawCommand.indexID = totalIndices;
				drawCommand.vertexID = totalVertices;

				totalIndices += drawCommand.indices;
				totalVertices += drawCommand.vertices;
			}
			
			mesh.insertIndirects( drawCommands );
		}

		#if UF_ENV_DREAMCAST
		{
			uf::stl::vector<uf::stl::string> attributesKept = ext::json::vector<uf::stl::string>(graph.metadata["decode"]["attributes"]);
			if ( !mesh.isInterleaved() ) {
				uf::stl::vector<size_t> remove; remove.reserve(mesh.vertex.attributes.size());

				for ( size_t i = 0; i < mesh.vertex.attributes.size(); ++i ) {
					auto& attribute = mesh.vertex.attributes[i];
					if ( std::find( attributesKept.begin(), attributesKept.end(), attribute.descriptor.name ) != attributesKept.end() ) continue;
					remove.insert(remove.begin(), i);
					UF_MSG_DEBUG("Removing mesh attribute: {}", attribute.descriptor.name);
				}
				for ( auto& i : remove ) {
					mesh.buffers[mesh.vertex.attributes[i].buffer].clear();
					mesh.buffers[mesh.vertex.attributes[i].buffer].shrink_to_fit();
					mesh.vertex.attributes.erase(mesh.vertex.attributes.begin() + i);
				}
			} else {
				UF_MSG_DEBUG("Attribute removal requested yet mesh is interleaved, ignoring...");
			}
		}
		#endif
	
		{
		#if UF_ENV_DREAMCAST
			mesh.convert<float, uint16_t>();
		#else
			auto conversion = graph.metadata["decode"]["conversion"].as<uf::stl::string>();
			if ( conversion != "" ) {
			#if UF_USE_FLOAT16
				if ( conversion == "float16" ) mesh.convert<float, float16>();
				else if ( conversion == "float" ) mesh.convert<float16, float>();
			#endif
			#if UF_USE_BFLOAT16
				if ( conversion == "bfloat16" ) mesh.convert<float, bfloat16>();
				else if ( conversion == "float" ) mesh.convert<bfloat16, float>();
			#endif
				if ( conversion == "uint16_t" ) mesh.convert<float, uint16_t>();
				else if ( conversion == "float" ) mesh.convert<uint16_t, float>();
			}
		#endif
		}
			
		mesh.updateDescriptor();

		{
			auto& graphic = graph.root.entity->getComponent<uf::renderer::Graphic>();
			uf::graph::initializeGraphics( graph, *graph.root.entity, mesh );
		}
	}

	storage.instanceAddresses.keys = storage.instances.keys;
	UF_DEBUG_TIMER_MULTITRACE_END("Processed graph.");
}
void uf::graph::process( pod::Graph& graph, int32_t index, uf::Object& parent ) {
	auto& scene = uf::scene::getCurrentScene();
	auto& storage = uf::graph::globalStorage ? uf::graph::storage : scene.getComponent<pod::Graph::Storage>();

	auto& node = graph.nodes[index];
	// 
	bool ignore = false;
	// ignore pesky light_Orientation nodes
	if ( uf::string::split( node.name, "_" ).back() == "Orientation" ) ignore = true;
	// for dreamcast, ignore lights if we're baked
/*
	if ( graph.metadata["lights"]["lightmap"].as<bool>() && graph.metadata["lights"]["disable if lightmapped"].as<bool>(true) ) {
		if ( graph.lights.count(node.name) > 0 ) ignore = true;
	}
*/
	
	// on systems where frametime is very, very important, we can set all static nodes to not tick
	
	ext::json::Value tag = ext::json::find( node.name, graph.metadata["tags"] );
	if ( ext::json::isObject( tag ) ) {
		if ( graph.metadata["baking"]["enabled"].as<bool>(false) && !tag["bake"].as<bool>(true) ) ignore = true;
		if ( tag["ignore"].as<bool>() ) ignore = true;
	}
	bool isLight = graph.lights.count(node.name) > 0;
/*
	if ( graph.metadata["lights"]["lightmap"].as<bool>() )
		if ( ext::json::isObject( tag ) ) {
			if ( !tag["light"]["dynamic"].as<bool>() && !tag["light"]["shadows"].as<bool>() ) {
				ignore = true;
				UF_MSG_DEBUG("IGNORING {}", node.name);
			}
		} else {
			ignore = true;
		}
	}
*/
	if ( ignore ) return;
		
	// create child
	uf::Object* pointer = new uf::Object;
	parent.addChild(*pointer);

	uf::Object& entity = *pointer;
	node.entity = &entity;
	
	bool setName = entity.getName() == "Entity";
	auto& metadata = entity.getComponent<uf::ObjectBehavior::Metadata>();
	auto& metadataJson = entity.getComponent<uf::Serializer>();
	metadataJson["system"]["graph"]["name"] = node.name;
	metadataJson["system"]["graph"]["index"] = index;

	if ( ext::json::isObject( tag ) ) {
		if ( tag["action"].as<uf::stl::string>() == "load" ) {
			if ( tag["filename"].is<uf::stl::string>() ) {
				uf::stl::string filename = uf::io::resolveURI( tag["filename"].as<uf::stl::string>(), graph.metadata["root"].as<uf::stl::string>() );
				entity.load(filename);
			} else if ( ext::json::isObject( tag["payload"] ) ) {
				uf::Serializer json = tag["payload"];
				json["root"] = graph.metadata["root"];
				entity.load(json);
			}
		} else if ( tag["action"].as<uf::stl::string>() == "attach" ) {
			uf::stl::string filename = uf::io::resolveURI( tag["filename"].as<uf::stl::string>(), graph.metadata["root"].as<uf::stl::string>() );
			auto& child = entity.loadChild( filename, false );
			auto& childTransform = child.getComponent<pod::Transform<>>();
			auto& childMetadataJson = child.getComponent<uf::Serializer>();

			auto flatten = uf::transform::flatten( node.transform );
		//	if ( !tag["preserve position"].as<bool>(false) ) childTransform.position = flatten.position;
		//	if ( !tag["preserve orientation"].as<bool>(false) ) childTransform.orientation = flatten.orientation;
			
		//	childTransform.position = flatten.position;
		//	childTransform.orientation = flatten.orientation;
			childTransform = flatten;

		//	childMetadataJson["transform"] = uf::transform::encode( flatten );
		}
		if ( tag["static"].is<bool>() ) {
			metadata.system.ignoreGraph = tag["static"].as<bool>();
		}
	}

	// create as light
	{
		if ( isLight ) {
			auto& l = graph.lights[node.name];
			
		#if UF_USE_OPENGL
			metadata.system.ignoreGraph = true;
		#else
			metadata.system.ignoreGraph = graph.metadata["debug"]["static"].as<bool>();
		#endif
			float powerScale = graph.metadata["lights"]["scale"].as<float>(1);
			
			uf::Serializer metadataLight;
			metadataLight["radius"][0] = 0.001;
			metadataLight["radius"][1] = l.range; // l.range <= 0.001f ? graph.metadata["lights"]["range cap"].as<float>() : l.range;
			metadataLight["power"] = l.intensity * powerScale; //  * graph.metadata["lights"]["power scale"].as<float>();
			metadataLight["color"][0] = l.color.x;
			metadataLight["color"][1] = l.color.y;
			metadataLight["color"][2] = l.color.z;
			metadataLight["shadows"] = graph.metadata["lights"]["shadows"].as<bool>();
			metadataLight["dynamic"] = false;

			if ( uf::string::matched( node.name, R"(/\bspot\b/)" ) ) {
				metadataLight["type"] = "spot";
			}

			if ( ext::json::isArray( graph.metadata["lights"]["radius"] ) ) {
				metadataLight["radius"] = graph.metadata["lights"]["radius"];
			}
			if ( graph.metadata["lights"]["bias"].is<float>() ) {
				metadataLight["bias"] = graph.metadata["lights"]["bias"].as<float>();
			}
			// copy from tag information
			ext::json::forEach( graph.metadata["tags"][node.name]["light"], [&]( const uf::stl::string& key, ext::json::Value& value ){
				if ( key == "transform" ) return;
				metadataLight[key] = value;
			});
		/*
			bool should = false;
			if ( !graph.metadata["lights"]["lightmap"].as<bool>() ) {
				should = true;
			} else if ( metadataLight["shadows"].as<bool>() || metadataLight["dynamic"].as<bool>() ) {
				should = true;
			}
		*/
			auto& metadataJson = entity.getComponent<uf::Serializer>();
			entity.load("/light.json");
			// copy
			ext::json::forEach( metadataLight, [&]( const uf::stl::string& key, ext::json::Value& value ) {
				metadataJson["light"][key] = value;
			});
		}
	}

	// set name
	if ( setName ) {
		entity.setName( node.name );
	}

	// reference transform to parent
	auto& transform = entity.getComponent<pod::Transform<>>();
	{
		transform = node.transform;
		transform.reference = &parent.getComponent<pod::Transform<>>();
		// override transform
		if ( tag["transform"]["offset"].as<bool>() ) {
			auto parsed = uf::transform::decode( tag["transform"], pod::Transform<>{} );
			transform.position += parsed.position;
			transform.orientation = uf::quaternion::multiply( transform.orientation, parsed.orientation );
		} else {
			transform = uf::transform::decode( tag["transform"], transform );
			if ( tag["transform"]["parent"].is<uf::stl::string>() ) {
				auto* parentPointer = uf::graph::find( graph, tag["transform"]["parent"].as<uf::stl::string>() );
				if ( parentPointer ) {
					auto& parentNode = *parentPointer;
					// entity already exists, bind to its transform
					if ( parentNode.entity && parentNode.entity->hasComponent<pod::Transform<>>() ) {
						auto& parentTransform = parentNode.entity->getComponent<pod::Transform<>>();
						transform = uf::transform::reference( transform, parentTransform, tag["transform"]["reorient"].as<bool>() );
						transform.position = -transform.position;
					// doesnt exist, bind to the node transform
					} else {
						transform = uf::transform::reference( transform, parentNode.transform, tag["transform"]["reorient"].as<bool>() );
					}
				}
			}
		}
	}

	size_t objectID = storage.entities.keys.size();
	auto objectKeyName = std::to_string(objectID);
	storage.entities[objectKeyName] = &entity;

	if ( 0 <= node.mesh && node.mesh < graph.meshes.size() ) {
		auto model = uf::transform::model( transform );
		auto& mesh = storage.meshes.map[graph.meshes[node.mesh]];
		auto& primitives = storage.primitives.map[graph.primitives[node.mesh]];

		pod::Instance::Bounds bounds;
		// setup instances
		for ( auto i = 0; i < primitives.size(); ++i ) {
			auto& primitive = primitives[i];

			size_t instanceID = storage.instances.keys.size();
			auto instanceKeyName = graph.instances.emplace_back(std::to_string(instanceID));

			auto& instance = storage.instances[instanceKeyName];
			instance = primitive.instance;

			instance.model = model;
			instance.previous = model;
			instance.objectID = objectID;
			instance.jointID = graph.metadata["renderer"]["skinned"].as<bool>() ? 0 : -1;

			bounds.min = uf::vector::min( bounds.min, instance.bounds.min );
			bounds.max = uf::vector::max( bounds.max, instance.bounds.max );

			if ( mesh.indirect.count && mesh.indirect.count <= primitives.size() ) {
				auto& attribute = mesh.indirect.attributes.front();
				auto& buffer = mesh.buffers[mesh.isInterleaved(mesh.indirect.interleaved) ? mesh.indirect.interleaved : attribute.buffer];
				pod::DrawCommand* drawCommands = (pod::DrawCommand*) buffer.data();
				auto& drawCommand = drawCommands[i];
				drawCommand.instanceID = instanceID;
			}
		}
		if ( (graph.metadata["renderer"]["separate"].as<bool>()) && graph.metadata["renderer"]["render"].as<bool>() ) {
			uf::graph::initializeGraphics( graph, entity, mesh );
		}
		
		{
			auto phyziks = tag["physics"];
			if ( !ext::json::isObject( phyziks ) ) phyziks = metadataJson["physics"];
			else metadataJson["physics"] = phyziks;
			
			if ( ext::json::isObject( phyziks ) ) {
				uf::stl::string type = phyziks["type"].as<uf::stl::string>();		

				if ( type == "mesh" ) {
					auto& collider = entity.getComponent<pod::PhysicsState>();
					collider.stats.mass = phyziks["mass"].as(collider.stats.mass);
					collider.stats.friction = phyziks["friction"].as(collider.stats.friction);
					collider.stats.restitution = phyziks["restitution"].as(collider.stats.restitution);
					collider.stats.inertia = uf::vector::decode( phyziks["inertia"], collider.stats.inertia );
					collider.stats.gravity = uf::vector::decode( phyziks["gravity"], collider.stats.gravity );

					uf::physics::impl::create( entity.as<uf::Object>(), mesh, !phyziks["static"].as<bool>(true) );
				} else {
					auto min = uf::matrix::multiply<float>( model, bounds.min, 1.0f );
					auto max = uf::matrix::multiply<float>( model, bounds.max, 1.0f );

					pod::Vector3f center = (max + min) * 0.5f;
					pod::Vector3f corner = uf::vector::abs(max - min) * 0.5f;
					
					metadataJson["physics"]["center"] = uf::vector::encode( center );
					metadataJson["physics"]["corner"] = uf::vector::encode( corner );
				}
			}
		}
	}
	for ( auto index : node.children ) uf::graph::process( graph, index, entity );
}
void uf::graph::cleanup( pod::Graph& graph ) {
}
void uf::graph::override( pod::Graph& graph ) {
	auto& scene = uf::scene::getCurrentScene();
	auto& storage = uf::graph::globalStorage ? uf::graph::storage : scene.getComponent<pod::Graph::Storage>();

	graph.settings.animations.override.a = 0;
	graph.settings.animations.override.map.clear();
	bool toNeutralPose = graph.sequence.empty();
	// store every node's current transform
	for ( auto& node : graph.nodes ) {
		graph.settings.animations.override.map[node.index].first = node.transform;
		graph.settings.animations.override.map[node.index].second = node.transform;
		if ( toNeutralPose ) {
			graph.settings.animations.override.map[node.index].second.position = { 0, 0, 0 };
			graph.settings.animations.override.map[node.index].second.orientation = { 0, 0, 0, 1 };
			graph.settings.animations.override.map[node.index].second.scale = { 1, 1, 1 };
		}
	}
	// set our destination transform per node
	if ( !toNeutralPose ) {
		uf::stl::string name = graph.sequence.front();
		pod::Animation& animation = storage.animations.map[name]; // graph.animations[name];
		for ( auto& channel : animation.channels ) {
			auto& override = graph.settings.animations.override.map[channel.node];
			auto& sampler = animation.samplers[channel.sampler];
			if ( sampler.interpolator != "LINEAR" ) continue;
			for ( size_t i = 0; i < sampler.inputs.size() - 1; ++i ) {
				if ( !(animation.start >= sampler.inputs[i] && animation.start <= sampler.inputs[i+1]) ) continue;
				if ( channel.path == "translation" ) {
					override.second.position = sampler.outputs[i];
				} else if ( channel.path == "rotation" ) {
					override.second.orientation = uf::quaternion::normalize( sampler.outputs[i] );
				} else if ( channel.path == "scale" ) {
					override.second.scale = sampler.outputs[i];
				}
			}
		}
	}
}

void uf::graph::initialize( pod::Graph& graph ) {
	if ( graph.metadata["baking"]["enabled"].as<bool>() ) {
		auto& metadataJson = graph.root.entity->getComponent<uf::Serializer>();
		metadataJson["baking"] = graph.metadata["baking"];
		metadataJson["baking"]["root"] = uf::io::directory( graph.name );
		uf::instantiator::bind( "BakingBehavior", *graph.root.entity );
	}

	graph.root.entity->initialize();
	graph.root.entity->process([&]( uf::Entity* entity ) {
		if ( entity->getUid() == 0 ) entity->initialize();
	});

	auto& scene = uf::scene::getCurrentScene();
	scene.invalidateGraph();
}
void uf::graph::animate( pod::Graph& graph, const uf::stl::string& _name, float speed, bool immediate ) {
	auto& scene = uf::scene::getCurrentScene();
	auto& storage = uf::graph::globalStorage ? uf::graph::storage : scene.getComponent<pod::Graph::Storage>();

	if ( !(graph.metadata["renderer"]["skinned"].as<bool>()) ) return;
	const uf::stl::string name = /*graph.name + "/" +*/ _name;
//	if ( graph.animations.count( name ) > 0 ) {
	if ( storage.animations.map.count( name ) > 0 ) {
		// if already playing, ignore it
		if ( !graph.sequence.empty() && graph.sequence.front() == name ) return;
		if ( immediate ) {
			while ( !graph.sequence.empty() ) graph.sequence.pop();
		}
		bool empty = graph.sequence.empty();
		graph.sequence.emplace(name);
		if ( empty ) uf::graph::override( graph );
		graph.settings.animations.speed = speed;
	}
	update( graph, 0 );
}
void uf::graph::update( pod::Graph& graph ) {
	return update( graph, uf::physics::time::delta );
}
void uf::graph::update( pod::Graph& graph, float delta ) {
	auto& scene = uf::scene::getCurrentScene();
	auto& storage = uf::graph::globalStorage ? uf::graph::storage : scene.getComponent<pod::Graph::Storage>();

	// update our instances
#if !UF_ENV_DREAMCAST
//	UF_TIMER_MULTITRACE_START("Tick Start");
	uf::stl::unordered_map<size_t, pod::Matrix4f> instanceCache;
	for ( auto& name : storage.instances.keys ) {
		auto& instance = storage.instances[name];
		instance.previous = instance.model;

		if ( instanceCache.count( instance.objectID ) > 0 ) {
			instance.model = instanceCache[instance.objectID];
			continue;
		}

		auto& entity = *storage.entities[std::to_string(instance.objectID)];
		if ( !entity.isValid() ) continue;
		auto& metadata = entity.getComponent<uf::ObjectBehavior::Metadata>();
		if ( metadata.system.ignoreGraph ) continue;
		
		auto& transform = entity.getComponent<pod::Transform<>>();
		instance.model = (instanceCache[instance.objectID] = uf::transform::model( transform ));
	}
//	UF_TIMER_MULTITRACE_END("Tick End");
#endif

	// no skins
	if ( !(graph.metadata["renderer"]["skinned"].as<bool>()) ) {
		return;
	}

	if ( graph.sequence.empty() ) goto UPDATE;
	if ( graph.settings.animations.override.a >= 0 ) goto OVERRIDE;
	{
		uf::stl::string name = graph.sequence.front();
		pod::Animation* animation = &storage.animations.map[name]; // &graph.animations[name];
		animation->cur += delta * graph.settings.animations.speed; // * graph.settings.animations.override.speed;
		if ( animation->end < animation->cur ) {
			animation->cur = graph.settings.animations.loop ? animation->cur - animation->end : 0;
			// go-to next animation
			if ( !graph.settings.animations.loop ) {
				graph.sequence.pop();
				// out of animations, set to neutral pose
				if ( graph.sequence.empty() ) {
					uf::graph::override( graph );
					goto OVERRIDE;
				}
				name = graph.sequence.front();
				animation = &storage.animations.map[name]; // &graph.animations[name];
			}
		}
		for ( auto& channel : animation->channels ) {
			auto& sampler = animation->samplers[channel.sampler];
			if ( sampler.interpolator != "LINEAR" ) continue;
			for ( size_t i = 0; i < sampler.inputs.size() - 1; ++i ) {
				if ( !(animation->cur >= sampler.inputs[i] && animation->cur <= sampler.inputs[i+1]) ) continue;
				float a = (animation->cur - sampler.inputs[i]) / (sampler.inputs[i+1] - sampler.inputs[i]);
				auto& transform = graph.nodes[channel.node].transform;
				if ( channel.path == "translation" ) {
					transform.position = uf::vector::mix( sampler.outputs[i], sampler.outputs[i+1], a );
				} else if ( channel.path == "rotation" ) {
					transform.orientation = uf::quaternion::normalize( uf::quaternion::slerp(sampler.outputs[i], sampler.outputs[i+1], a) );
				} else if ( channel.path == "scale" ) {
					transform.scale = uf::vector::mix( sampler.outputs[i], sampler.outputs[i+1], a );
				}
			}
		}
		goto UPDATE;
	}
OVERRIDE:
	for ( auto pair : graph.settings.animations.override.map ) {
		graph.nodes[pair.first].transform.position = uf::vector::mix( pair.second.first.position, pair.second.second.position, graph.settings.animations.override.a );
		graph.nodes[pair.first].transform.orientation = uf::quaternion::normalize( uf::quaternion::slerp(pair.second.first.orientation, pair.second.second.orientation, graph.settings.animations.override.a) );
		graph.nodes[pair.first].transform.scale = uf::vector::mix( pair.second.first.scale, pair.second.second.scale, graph.settings.animations.override.a );
	}
	// finished our overrided interpolation, clear it
	if ( (graph.settings.animations.override.a += delta * graph.settings.animations.override.speed) >= 1 ) {
		graph.settings.animations.override.a = -std::numeric_limits<float>::max();
		graph.settings.animations.override.map.clear();
	}
UPDATE:
	for ( auto& node : graph.nodes ) uf::graph::update( graph, node );
}
void uf::graph::update( pod::Graph& graph, pod::Node& node ) {
	auto& scene = uf::scene::getCurrentScene();
	auto& storage = uf::graph::globalStorage ? uf::graph::storage : scene.getComponent<pod::Graph::Storage>();

	if ( 0 <= node.skin && node.skin < graph.skins.size() ) {
		pod::Matrix4f nodeMatrix = uf::graph::matrix( graph, node.index );
		pod::Matrix4f inverseTransform = uf::matrix::inverse( nodeMatrix );

		auto& name = graph.skins[node.skin];
		auto& skin = storage.skins[name];
		auto& joints = storage.joints[name];
		joints.resize( skin.joints.size() );
		for ( size_t i = 0; i < skin.joints.size(); ++i ) joints[i] = uf::matrix::identity();

		if ( graph.settings.animations.override.a >= 0 || !graph.sequence.empty() ) {
			for ( size_t i = 0; i < skin.joints.size(); ++i ) {
				joints[i] = inverseTransform * (uf::graph::matrix(graph, skin.joints[i]) * skin.inverseBindMatrices[i]);
			}
		}
	}
}

void uf::graph::destroy( pod::Graph& graph ) {
#if 0
	for ( auto& t : graph.textures ) t.texture.destroy();
	for ( auto& m : graph.meshes ) m.destroy();
#endif
}

void uf::graph::initialize() {
	if ( uf::graph::globalStorage ) return uf::graph::initialize( uf::graph::storage );
	return uf::graph::initialize( uf::scene::getCurrentScene() );
}
void uf::graph::initialize( uf::Object& object ) {
	return uf::graph::initialize( object.getComponent<pod::Graph::Storage>() );
}
void uf::graph::initialize( pod::Graph::Storage& storage ) {
	storage.buffers.camera.initialize( (const void*) nullptr, sizeof(pod::Camera::Viewports), uf::renderer::enums::Buffer::UNIFORM );
	storage.buffers.drawCommands.initialize( (const void*) nullptr, sizeof(pod::DrawCommand)  * uf::graph::initialBufferElements, uf::renderer::enums::Buffer::STORAGE );
	storage.buffers.instance.initialize( (const void*) nullptr, sizeof(pod::Instance) * uf::graph::initialBufferElements, uf::renderer::enums::Buffer::STORAGE );
	storage.buffers.instanceAddresses.initialize( (const void*) nullptr, sizeof(pod::Instance::Addresses) * uf::graph::initialBufferElements, uf::renderer::enums::Buffer::STORAGE );
	storage.buffers.joint.initialize( (const void*) nullptr, sizeof(pod::Matrix4f) * uf::graph::initialBufferElements, uf::renderer::enums::Buffer::STORAGE );
	storage.buffers.material.initialize( (const void*) nullptr, sizeof(pod::Material) * uf::graph::initialBufferElements, uf::renderer::enums::Buffer::STORAGE );
	storage.buffers.texture.initialize( (const void*) nullptr, sizeof(pod::Texture) * uf::graph::initialBufferElements, uf::renderer::enums::Buffer::STORAGE );
	storage.buffers.light.initialize( (const void*) nullptr, sizeof(pod::Light) * uf::graph::initialBufferElements, uf::renderer::enums::Buffer::STORAGE );
}
void uf::graph::tick() {
	if ( uf::graph::globalStorage ) return uf::graph::tick( uf::graph::storage );
	return uf::graph::tick( uf::scene::getCurrentScene() );
}
void uf::graph::tick( uf::Object& object ) {
	return uf::graph::tick( object.getComponent<pod::Graph::Storage>() );
}
void uf::graph::tick( pod::Graph::Storage& storage ) {
/*
	uf::stl::vector<pod::Instance> instances; instances.reserve(storage.instances.map.size());
	for ( auto& key : storage.instances.keys ) instances.emplace_back( storage.instances.map[key] );
	if ( !instances.empty() ) storage.buffers.instance.update( (const void*) instances.data(), instances.size() * sizeof(pod::Instance) );
*/
	bool rebuild = false;
	uf::stl::vector<pod::Instance> instances = storage.instances.flatten();
	rebuild = rebuild || storage.buffers.instance.update( (const void*) instances.data(), instances.size() * sizeof(pod::Instance) );
/*
	uf::stl::vector<pod::Instance::Addresses> instanceAddresses; instanceAddresses.reserve(storage.instanceAddresses.map.size());
	for ( auto& key : storage.instances.keys ) instanceAddresses.emplace_back( storage.instanceAddresses.map[key] );
	if ( !instanceAddresses.empty() ) rebuild = rebuild || storage.buffers.instanceAddresses.update( (const void*) instanceAddresses.data(), instanceAddresses.size() * sizeof(pod::Instance::Addresses) );
*/
	uf::stl::vector<pod::Instance::Addresses> instanceAddresses = storage.instanceAddresses.flatten();
	rebuild = rebuild || storage.buffers.instanceAddresses.update( (const void*) instanceAddresses.data(), instanceAddresses.size() * sizeof(pod::Instance::Addresses) );

	uf::stl::vector<pod::Matrix4f> joints; joints.reserve(storage.joints.map.size());
	for ( auto& key : storage.joints.keys ) {
		auto& matrices = storage.joints.map[key];
		joints.reserve( joints.size() + matrices.size() );
		for ( auto& mat : matrices ) joints.emplace_back( mat );
	}
	/*if ( !joints.empty() )*/ rebuild = rebuild || storage.buffers.joint.update( (const void*) joints.data(), joints.size() * sizeof(pod::Matrix4f) );

	if ( ::newGraphAdded ) {
	#if 1
		uf::stl::vector<pod::Material> materials = storage.materials.flatten();
		uf::stl::vector<pod::Texture> textures = storage.textures.flatten();

		uf::stl::vector<pod::DrawCommand> drawCommands; drawCommands.reserve(storage.drawCommands.map.size());
		for ( auto& key : storage.drawCommands.keys ) drawCommands.insert( drawCommands.end(), storage.drawCommands.map[key].begin(), storage.drawCommands.map[key].end() );
	#else
		uf::stl::vector<pod::DrawCommand> drawCommands; drawCommands.reserve(storage.drawCommands.map.size());
		uf::stl::vector<pod::Material> materials; materials.reserve(storage.materials.map.size());
		uf::stl::vector<pod::Texture> textures; textures.reserve(storage.textures.map.size());

		for ( auto& key : storage.drawCommands.keys ) drawCommands.insert( drawCommands.end(), storage.drawCommands.map[key].begin(), storage.drawCommands.map[key].end() );
		for ( auto& key : storage.materials.keys ) materials.emplace_back( storage.materials.map[key] );
		for ( auto& key : storage.textures.keys ) textures.emplace_back( storage.textures.map[key] );
	#endif
		rebuild = rebuild || storage.buffers.drawCommands.update( (const void*) drawCommands.data(), drawCommands.size() * sizeof(pod::DrawCommand) );
		rebuild = rebuild || storage.buffers.material.update( (const void*) materials.data(), materials.size() * sizeof(pod::Material) );
		rebuild = rebuild || storage.buffers.texture.update( (const void*) textures.data(), textures.size() * sizeof(pod::Texture) );

		::newGraphAdded = false;

	}
	if ( rebuild ) {
		UF_MSG_DEBUG("Graph buffers requesting renderer update");
		uf::renderer::states::rebuild = true;

	/*
		if ( uf::renderer::hasRenderMode("", true) ) {
			auto& renderMode = uf::renderer::getRenderMode("", true);
			auto& blitter = renderMode.getBlitter();
			auto& shader = blitter.material.getShader(blitter.material.hasShader("compute", "deferred") ? "compute" : "fragment", "deferred");

			blitter.material.textures.clear();

			auto moved = std::move( shader.buffers );
			for ( auto& buffer : moved ) {
				if ( buffer.aliased ) continue;
				shader.buffers.emplace_back( buffer );
				buffer.aliased = true;
			}

			shader.buffers.emplace_back( storage.buffers.drawCommands.alias() );
			shader.buffers.emplace_back( storage.buffers.instance.alias() );
			shader.buffers.emplace_back( storage.buffers.instanceAddresses.alias() );
			shader.buffers.emplace_back( storage.buffers.material.alias() );
			shader.buffers.emplace_back( storage.buffers.texture.alias() );
			shader.buffers.emplace_back( storage.buffers.light.alias() );
		}
	*/
	}
}
void uf::graph::render() {
	if ( uf::graph::globalStorage ) return uf::graph::render( uf::graph::storage );
	return uf::graph::render( uf::scene::getCurrentScene() );
}
void uf::graph::render( uf::Object& object ) {
	return uf::graph::render( object.getComponent<pod::Graph::Storage>() );
}
void uf::graph::render( pod::Graph::Storage& storage ) {	
	auto* renderMode = uf::renderer::getCurrentRenderMode();
	
	auto& scene = uf::scene::getCurrentScene();
	auto& controller = scene.getController();
	auto& camera = scene.getCamera( controller ); // controller.getComponent<uf::Camera>();
	
//	camera.update();
	auto viewport = camera.data().viewport;

#if UF_USE_FFX_FSR
	if ( ext::fsr::initialized && renderMode->getType() == "Deferred" ) {
		auto jitter = ext::fsr::getJitterMatrix();
		for ( auto i = 0; i < uf::camera::maxViews; ++i ) {
			viewport.matrices[i].projection = jitter * viewport.matrices[i].projection;
		}
	}
#endif

	storage.buffers.camera.update( (const void*) &viewport, sizeof(pod::Camera::Viewports) );

#if UF_USE_VULKAN
	if ( !renderMode ) return;
	if ( !renderMode->hasBuffer("camera") ) return;
	auto& buffer = renderMode->getBuffer("camera");
	buffer.update( (const void*) &viewport, sizeof(pod::Camera::Viewports) );
/*
	for ( auto& buffer : renderMode->buffers ) {
		if ( !(buffer.usage & uf::renderer::enums::Buffer::UNIFORM) ) continue;
		if ( buffer.allocationInfo.size != sizeof(pod::Camera::Viewports) ) continue;
		if ( buffer.buffer == storage.buffers.camera.buffer ) continue;
		buffer.update( (const void*) &viewport, sizeof(pod::Camera::Viewports) );
		return;
	}
*/
#endif
}
void uf::graph::destroy( bool soft ) {
	soft = false;
	if ( uf::graph::globalStorage ) return uf::graph::destroy( uf::graph::storage, soft );
	return uf::graph::destroy( uf::scene::getCurrentScene(), soft );
}
void uf::graph::destroy( uf::Object& object, bool soft ) {
	soft = false;
	return uf::graph::destroy( object.getComponent<pod::Graph::Storage>(), soft );
}
void uf::graph::destroy( pod::Graph::Storage& storage, bool soft ) {
	soft = false;
#if UF_USE_VULKAN
	for ( auto& texture : uf::renderer::gc::textures ) {
		texture.destroy( false );
	}
	uf::renderer::gc::textures.clear();
#endif

	// cleanup graphic handles
	for ( auto pair : storage.texture2Ds.map ) pair.second.destroy();
	for ( auto& t : storage.shadow2Ds ) t.destroy();
	for ( auto& t : storage.shadowCubes ) t.destroy();

	for ( auto pair : storage.atlases.map ) pair.second.clear();
	for ( auto pair : storage.images.map ) pair.second.clear();
	for ( auto pair : storage.meshes.map ) pair.second.destroy();

	// cleanup storage cache
	storage.instances.clear();
	storage.instanceAddresses.clear();
	storage.primitives.clear();
	storage.drawCommands.clear();
	storage.meshes.clear();
	storage.images.clear();
	storage.materials.clear();
	storage.textures.clear();
	storage.samplers.clear();
	storage.skins.clear();
	storage.animations.clear();
	storage.atlases.clear();
	storage.joints.clear();
	storage.texture2Ds.clear();
	storage.entities.clear();
	storage.shadow2Ds.clear();
	storage.shadowCubes.clear();

	// cleanup storage buffers
	if ( !soft ) {
		storage.buffers.camera.destroy(true);
		storage.buffers.drawCommands.destroy(true);
		storage.buffers.instance.destroy(true);
		storage.buffers.instanceAddresses.destroy(true);
		storage.buffers.joint.destroy(true);
		storage.buffers.material.destroy(true);
		storage.buffers.texture.destroy(true);
		storage.buffers.light.destroy(true);
	}

	uf::renderer::states::rebuild = true;
}
void uf::graph::reload() {
	::newGraphAdded = true;
}