#include "behavior.h"

#include <uf/utils/graphic/mesh.h>
#include <uf/utils/graphic/graphic.h>
#include <uf/utils/camera/camera.h>
#include <uf/utils/math/transform.h>
#include <uf/utils/math/physics.h>
#include <uf/utils/serialize/serializer.h>
#include <uf/utils/math/collision.h>
#include <uf/utils/thread/thread.h>
#include <uf/utils/math/physics.h>

#include <uf/utils/window/window.h>

#include <uf/ext/openvr/openvr.h>

#include <uf/utils/renderer/renderer.h>
#include <uf/utils/noise/noise.h>

UF_BEHAVIOR_REGISTER_CPP(ext::NoiseBehavior)
#define this ((uf::Scene*) &self)
void ext::NoiseBehavior::initialize( uf::Object& self ) {
//	auto& image = this->getComponent<uf::Image>();
	auto& noiseGenerator = this->getComponent<uf::PerlinNoise>();
	auto& graphic = this->getComponent<uf::Graphic>();
	auto mesh = this->getComponent<uf::Mesh>();
	auto& texture = graphic.material.textures.emplace_back();
	auto& metadata = this->getComponent<uf::Serializer>();
	noiseGenerator.seed(rand());

	float high = std::numeric_limits<float>::min();
	float low = std::numeric_limits<float>::max();
	float amplitude = metadata["amplitude"].is<float>() ? metadata["amplitude"].as<float>() : 1.5;
	pod::Vector3ui size = uf::vector::decode(metadata["size"], pod::Vector3ui{512, 512, 1});
	pod::Vector3d coefficients = uf::vector::decode(metadata["coefficients"], pod::Vector3d{3.0, 3.0, 3.0});

	std::vector<uint8_t> pixels(size.x * size.y * size.z);
	std::vector<float> perlins(size.x * size.y * size.z);
#pragma omp parallel for
	for ( size_t z = 0; z < size.z; ++z ) {
	for ( size_t y = 0; y < size.y; ++y ) {
	for ( size_t x = 0; x < size.x; ++x ) {
		float nx = (float) x / (float) size.x;
		float ny = (float) y / (float) size.y;
		float nz = (float) z / (float) size.z;

		float n = amplitude * noiseGenerator.noise(coefficients.x * nx, coefficients.y * ny, coefficients.z * nz);
		high = std::max( high, n );
		low = std::min( low, n );
		perlins[x + y * size.x + z * size.x * size.y] = n;
	}
	}
	}
	for ( size_t i = 0; i < perlins.size(); ++i ) {
		float n = perlins[i];
		n = n - floor(n);
		float normalized = (n - low) / (high - low);
		pixels[i] = static_cast<uint8_t>(floor(normalized * 255));
	}
	texture.fromBuffers( (void*) pixels.data(), pixels.size(), uf::renderer::enums::Format::R8_UNORM, size.x, size.y, size.z, 1 );

	mesh.vertices = {
		{{-1*-0.5f, 0.0f, 0.0f}, {1.0f, 0.0f}, { 0.0f, 0.0f, -1.0f } },
		{{-1*0.5f, 0.0f, 0.0f}, {0.0f, 0.0f}, { 0.0f, 0.0f, -1.0f } },
		{{-1*0.5f, 1.0f, 0.0f}, {0.0f, 1.0f}, { 0.0f, 0.0f, -1.0f } },
		{{-1*0.5f, 1.0f, 0.0f}, {0.0f, 1.0f}, { 0.0f, 0.0f, -1.0f } },
		{{-1*-0.5f, 1.0f, 0.0f}, {1.0f, 1.0f}, { 0.0f, 0.0f, -1.0f } },
		{{-1*-0.5f, 0.0f, 0.0f}, {1.0f, 0.0f}, { 0.0f, 0.0f, -1.0f } },

		{{-1*0.5f, 1.0f, 0.0f}, {0.0f, 1.0f}, { 0.0f, 0.0f, 1.0f } },
		{{-1*0.5f, 0.0f, 0.0f}, {0.0f, 0.0f}, { 0.0f, 0.0f, 1.0f } },
		{{-1*-0.5f, 0.0f, 0.0f}, {1.0f, 0.0f}, { 0.0f, 0.0f, 1.0f } },
		{{-1*-0.5f, 0.0f, 0.0f}, {1.0f, 0.0f}, { 0.0f, 0.0f, 1.0f } },
		{{-1*-0.5f, 1.0f, 0.0f}, {1.0f, 1.0f}, { 0.0f, 0.0f, 1.0f } },
		{{-1*0.5f, 1.0f, 0.0f}, {0.0f, 1.0f}, { 0.0f, 0.0f, 1.0f } },
	};
	graphic.initialize();
	graphic.initializeGeometry( mesh );

	this->callHook("entity:TextureUpdate.%UID%");

	graphic.material.attachShader(uf::io::root+"/shaders/base.vert.spv", uf::renderer::enums::Shader::VERTEX);
	graphic.material.attachShader(uf::io::root+"/shaders/noise.frag.spv", uf::renderer::enums::Shader::FRAGMENT);
}
void ext::NoiseBehavior::tick( uf::Object& self ) {
	auto& metadata = this->getComponent<uf::Serializer>();
	
	float slice = metadata["slice"].as<float>();
	slice += uf::physics::time::delta * metadata["speed"].as<float>();
//	if ( slice >= metadata["size"][2].as<float>() ) slice = 0;
	metadata["slice"] = slice;

	if ( !this->hasComponent<uf::Graphic>() ) return;
	auto& scene = uf::scene::getCurrentScene();
	auto& graphic = this->getComponent<uf::Graphic>();
	auto& transform = this->getComponent<pod::Transform<>>();
	auto& controller = scene.getController();
	auto& camera = controller.getComponent<uf::Camera>();
	
	if ( !graphic.initialized ) return;
	if ( !graphic.material.hasShader("fragment") ) return;

	auto& shader = graphic.material.getShader("fragment");
	auto& uniform = shader.getUniform("UBO");
#if UF_UNIFORMS_UPDATE_WITH_JSON
//	auto uniforms = shader.getUniformJson("UBO");
	ext::json::Value uniforms;
	uniforms["slice"] = slice;
	shader.updateUniform("UBO", uniforms );
#else
	struct UniformDescriptor {
		alignas(4) float slice = 0;
	};
	auto& uniforms = uniform.get<UniformDescriptor>();
	uniforms.slice = slice;
	shader.updateUniform( "UBO", uniform );
#endif
}
void ext::NoiseBehavior::render( uf::Object& self ) {}
void ext::NoiseBehavior::destroy( uf::Object& self ) {}
#undef this