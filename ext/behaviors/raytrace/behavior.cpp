#include <uf/config.h>
#if UF_USE_VULKAN

#include "behavior.h"

#include <uf/utils/renderer/renderer.h>

#include <uf/utils/math/transform.h>
#include <uf/utils/math/physics.h>
#include <uf/utils/camera/camera.h>
#include <uf/ext/gltf/gltf.h>
#include <uf/engine/asset/asset.h>
#include <uf/utils/graphic/graphic.h>
#include <uf/ext/xatlas/xatlas.h>

#include "../light/behavior.h"
#include "../scene/behavior.h"

#define TEST 0

UF_BEHAVIOR_REGISTER_CPP(ext::RayTraceSceneBehavior)
UF_BEHAVIOR_TRAITS_CPP(ext::RayTraceSceneBehavior, ticks = true, renders = false, multithread = false)
#define this (&self)
void ext::RayTraceSceneBehavior::initialize( uf::Object& self ) {
#if TEST
	auto& metadata = this->getComponent<ext::RayTraceSceneBehavior::Metadata>();
	auto& graphic = this->getComponent<uf::renderer::Graphic>(); //metadata.renderer.graphic;
	auto& mesh = this->getComponent<uf::Mesh>();


	mesh.bind<pod::Vertex_3F>();
	mesh.insertVertices<pod::Vertex_3F>({
		{-1.0f,-1.0f,-1.0f,},
		{-1.0f,-1.0f, 1.0f,},
		{-1.0f, 1.0f, 1.0f,},
		{1.0f, 1.0f,-1.0f,},
		{-1.0f,-1.0f,-1.0f,},
		{-1.0f, 1.0f,-1.0f,},
		{1.0f,-1.0f, 1.0f,},
		{-1.0f,-1.0f,-1.0f,},
		{1.0f,-1.0f,-1.0f,},
		{1.0f, 1.0f,-1.0f,},
		{1.0f,-1.0f,-1.0f,},
		{-1.0f,-1.0f,-1.0f,},
		{-1.0f,-1.0f,-1.0f,},
		{-1.0f, 1.0f, 1.0f,},
		{-1.0f, 1.0f,-1.0f,},
		{1.0f,-1.0f, 1.0f,},
		{-1.0f,-1.0f, 1.0f,},
		{-1.0f,-1.0f,-1.0f,},
		{-1.0f, 1.0f, 1.0f,},
		{-1.0f,-1.0f, 1.0f,},
		{1.0f,-1.0f, 1.0f,},
		{1.0f, 1.0f, 1.0f,},
		{1.0f,-1.0f,-1.0f,},
		{1.0f, 1.0f,-1.0f,},
		{1.0f,-1.0f,-1.0f,},
		{1.0f, 1.0f, 1.0f,},
		{1.0f,-1.0f, 1.0f,},
		{1.0f, 1.0f, 1.0f,},
		{1.0f, 1.0f,-1.0f,},
		{-1.0f, 1.0f,-1.0f,},
		{1.0f, 1.0f, 1.0f,},
		{-1.0f, 1.0f,-1.0f,},
		{-1.0f, 1.0f, 1.0f,},
		{1.0f, 1.0f, 1.0f,},
		{-1.0f, 1.0f, 1.0f,},
		{1.0f,-1.0f, 1.0f,},
	});
	mesh.updateDescriptor();

	graphic.initialize("Compute");
	graphic.initializeMesh( mesh );

	uf::stl::vector<pod::Instance> instances;
	auto& instance = instances.emplace_back();
	instance.model = uf::matrix::identity();

	graphic.generateBottomAccelerationStructures();
	graphic.generateTopAccelerationStructure( { &graphic }, instances );

	auto& shader = graphic.material.getShader("ray:gen", uf::renderer::settings::pipelines::names::rt);
	auto& image = shader.textures.front();
	
	if ( !uf::renderer::hasRenderMode("Compute", true) ) {
		auto* renderMode = new uf::renderer::RenderTargetRenderMode;
		renderMode->setTarget("Compute");
		renderMode->metadata.json["shaders"]["vertex"] = "/shaders/display/renderTargetSimple.vert.spv";
		renderMode->metadata.json["shaders"]["fragment"] = "/shaders/display/renderTargetSimple.frag.spv";
		renderMode->blitter.descriptor.subpass = 1;
		renderMode->metadata.type = uf::renderer::settings::pipelines::names::rt;
		renderMode->metadata.pipelines.emplace_back(uf::renderer::settings::pipelines::names::rt);
		renderMode->execute = false;
		uf::renderer::addRenderMode( renderMode, "Compute" );
	} else {
		auto& renderMode = uf::renderer::getRenderMode("Compute", true);
		auto& blitter = *renderMode.getBlitter();
		
		if ( blitter.material.hasShader("fragment") ) {
			auto& shader = blitter.material.getShader("fragment");
			shader.textures.emplace_back().aliasTexture( image );
		}
	}
#else
	if ( !uf::renderer::hasRenderMode("Compute", true) ) {
		auto* renderMode = new uf::renderer::RenderTargetRenderMode;
		renderMode->setTarget("Compute");
		renderMode->metadata.json["shaders"]["vertex"] = "/shaders/display/renderTargetSimple.vert.spv";
		renderMode->metadata.json["shaders"]["fragment"] = "/shaders/display/renderTargetSimple.frag.spv";
		renderMode->blitter.descriptor.subpass = 1;
		renderMode->metadata.type = uf::renderer::settings::pipelines::names::rt;
		renderMode->metadata.pipelines.emplace_back(uf::renderer::settings::pipelines::names::rt);
		renderMode->execute = false;
		uf::renderer::addRenderMode( renderMode, "Compute" );
	}
#endif
}
void ext::RayTraceSceneBehavior::tick( uf::Object& self ) {
	auto& metadata = this->getComponent<ext::RayTraceSceneBehavior::Metadata>();
#if !TEST
	if ( !metadata.renderer.bound ) {
		auto instances = uf::graph::storage.instances.flatten();
		if ( instances.empty() ) return;

		uf::stl::vector<uf::Graphic*> graphics;
		this->process([&]( uf::Entity* entity ) {
			if ( !entity->hasComponent<uf::Graphic>() ) return;
			graphics.emplace_back(entity->getComponentPointer<uf::Graphic>());
		});
		if ( graphics.empty() ) return;
	
		auto& graphic = this->getComponent<uf::renderer::Graphic>();
		graphic.initialize("Compute");
		graphic.generateTopAccelerationStructure( graphics, instances );

		UF_MSG_DEBUG( graphics.size() << " " << instances.size() );

		metadata.renderer.bound = true;
		return;
	}
	if ( !metadata.renderer.bound ) return;
#endif

	auto& graphic = this->getComponent<uf::renderer::Graphic>();
	auto& shader = graphic.material.getShader("ray:gen", uf::renderer::settings::pipelines::names::rt);
	auto& image = shader.textures.front();
	
	struct UniformDescriptor {
		struct Matrices {
			alignas(16) pod::Matrix4f view;
			alignas(16) pod::Matrix4f projection;
			alignas(16) pod::Matrix4f iView;
			alignas(16) pod::Matrix4f iProjection;
			alignas(16) pod::Matrix4f iProjectionView;
			alignas(16) pod::Vector4f eyePos;
		} matrices[2];
	} uniforms;

	auto& scene = uf::scene::getCurrentScene();
	auto& controller = scene.getController();
	auto& camera = controller.getComponent<uf::Camera>();

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

	for ( auto& buffer : shader.buffers ) {
		if ( !(buffer.usage & uf::renderer::enums::Buffer::UNIFORM) ) continue;
		if ( buffer.allocationInfo.size != sizeof(UniformDescriptor) ) continue;
		
		buffer.update( (const void*) &uniforms, sizeof(UniformDescriptor) );
		break;
	}

	auto& renderMode = uf::renderer::getRenderMode("Compute", true);
	auto& blitter = *renderMode.getBlitter();
	if ( blitter.material.hasShader("fragment") ) {
		auto& shader = blitter.material.getShader("fragment");
		if ( shader.textures.empty() ) {
			shader.textures.emplace_back().aliasTexture( image );
			renderMode.execute = true;
		}
	}
	
	if ( uf::renderer::states::resized ) {
		image.destroy();
		image.fromBuffers( NULL, 0, uf::renderer::enums::Format::R8G8B8A8_UNORM, uf::renderer::settings::width, uf::renderer::settings::height, 1, 1, VK_IMAGE_USAGE_SAMPLED_BIT | VK_IMAGE_USAGE_STORAGE_BIT, VK_IMAGE_LAYOUT_GENERAL );

		graphic.descriptor.inputs.width = image.width;
		graphic.descriptor.inputs.height = image.height;
		
		graphic.getPipeline().update( graphic );

		if ( blitter.material.hasShader("fragment") ) {
			auto& shader = blitter.material.getShader("fragment");
			shader.textures.front().aliasTexture( image );
		}
	}

	TIMER(1.0, uf::Window::isKeyPressed("R") && ) {
		UF_MSG_DEBUG("Screenshotting RT scene...");
		image.screenshot().save("./data/rt.png");
	}
}
void ext::RayTraceSceneBehavior::render( uf::Object& self ){}
void ext::RayTraceSceneBehavior::destroy( uf::Object& self ){
	auto& metadata = this->getComponent<ext::RayTraceSceneBehavior::Metadata>();
	auto& graphic = this->getComponent<uf::renderer::Graphic>(); // metadata.renderer.graphic;

	graphic.destroy();
}
void ext::RayTraceSceneBehavior::Metadata::serialize( uf::Object& self, uf::Serializer& serializer ) {}
void ext::RayTraceSceneBehavior::Metadata::deserialize( uf::Object& self, uf::Serializer& serializer ) {}
#undef this
#endif