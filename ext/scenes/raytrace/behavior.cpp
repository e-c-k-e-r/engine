#if 0
#include "behavior.h"

#include <uf/utils/time/time.h>
#include <uf/utils/io/iostream.h>
#include <uf/utils/math/vector.h>
#include <uf/utils/math/transform.h>
#include <uf/utils/window/window.h>
#include <uf/utils/graphic/graphic.h>

#include <uf/utils/audio/audio.h>
#include <uf/utils/thread/thread.h>
#include <uf/utils/camera/camera.h>

#include <uf/engine/asset/asset.h>
#include <uf/engine/asset/masterdata.h>

#include <uf/utils/renderer/renderer.h>

#include <uf/ext/gltf/gltf.h>

#include <uf/utils/math/collision.h>

#include "../../ext.h"
#include "../../gui/gui.h"

UF_BEHAVIOR_REGISTER_CPP(ext::RayTracingSceneBehavior)
#define this ((uf::Scene*) &self)
void ext::RayTracingSceneBehavior::initialize( uf::Object& self ) {
	uf::Serializer& metadata = this->getComponent<uf::Serializer>();
	uf::Asset& assetLoader = this->getComponent<uf::Asset>();

	{
		auto& renderMode = this->getComponent<uf::renderer::ComputeRenderMode>();
		std::string name = "C:RT:" + std::to_string((int) this->getUid());
		uf::renderer::addRenderMode( &renderMode, name );
		if ( ext::json::isArray( metadata["light"]["shadows"]["resolution"] ) ) {
			renderMode.width = metadata["light"]["shadows"]["resolution"][0].as<size_t>();
			renderMode.height = metadata["light"]["shadows"]["resolution"][1].as<size_t>();
		} else {
			renderMode.width = metadata["light"]["shadows"]["resolution"].as<size_t>();
			renderMode.height = metadata["light"]["shadows"]["resolution"].as<size_t>();
		}
		{
			struct Shape {
				pod::Vector4f values;
				pod::Vector3f diffuse;
				float specular;
				uint32_t id;
				pod::Vector3ui _pad;
			};

			std::vector<Shape> shapes;
			{
				shapes.push_back( {{1.75f, -0.5f, 0.0f, 1.0f },  { 0.0f, 1.0f,   0.0f}, 32.0f, 1} );
				shapes.push_back( {{0.0f, 1.0f, -0.5f, 1.0f },  {0.65f, 0.77f, 0.97f}, 32.0f, 1} );
				shapes.push_back( {{-1.75f, -0.75f, -0.5f, 1.25f }, { 0.9f, 0.76f, 0.46f}, 32.0f, 1} );
			}
			{
				float roomDim = 12.0f;
				shapes.push_back( {{0.0f, 1.0f, 0.0f, roomDim}, {1.0f, 1.0f, 1.0f}, 32.0f, 2} );
				shapes.push_back( {{0.0f, -1.0f, 0.0f, roomDim}, {1.0f, 1.0f, 1.0f}, 32.0f, 2} );
				shapes.push_back( {{0.0f, 0.0f, 1.0f, roomDim}, {1.0f, 1.0f, 1.0f}, 32.0f, 2} );
				shapes.push_back( {{0.0f, 0.0f, -1.0f, roomDim}, {0.0f, 0.0f, 0.0f}, 32.0f, 2} );
				shapes.push_back( {{-1.0f, 0.0f, 0.0f, roomDim}, {1.0f, 0.0f, 0.0f}, 32.0f, 2} );
				shapes.push_back( {{1.0f, 0.0f, 0.0f, roomDim}, {0.0f, 1.0f, 0.0f}, 32.0f, 2} );
			}
			renderMode.compute.device = &uf::renderer::device;
			renderMode.compute.initializeBuffer(
				(void*) shapes.data(),
				shapes.size() * sizeof(Shape),
				uf::renderer::enums::Buffer::STORAGE
			);
		}
	}
}
void ext::RayTracingSceneBehavior::tick( uf::Object& self ) {
	uf::Serializer& metadata = this->getComponent<uf::Serializer>();
	uf::Asset& assetLoader = this->getComponent<uf::Asset>();

#if 1
	if ( this->hasComponent<uf::renderer::ComputeRenderMode>() ) {
		auto& renderMode = this->getComponent<uf::renderer::ComputeRenderMode>();
		/* Add lights to scene */ if ( renderMode.compute.initialized ) {
			renderMode.execute = true;
			struct UniformDescriptor {
				/*alignas(16)*/ pod::Matrix4f matrices[2];
				/*alignas(16)*/ pod::Vector4f ambient;
				struct {
					/*alignas(8)*/ pod::Vector2f range;
					/*alignas(8)*/ pod::Vector2f padding;
					/*alignas(16)*/ pod::Vector4f color;
				} fog;
				struct Light {
					/*alignas(16)*/ pod::Vector4f position;
					/*alignas(16)*/ pod::Vector4f color;
					/*alignas(8)*/ pod::Vector2i type;
					/*alignas(8)*/ pod::Vector2i padding;
					/*alignas(16)*/ pod::Matrix4f view;
					/*alignas(16)*/ pod::Matrix4f projection;
				} lights;
			};
			
			auto& shader = renderMode.compute.material.shaders.front();
			struct SpecializationConstant {
				uint32_t maxLights = 16;
				uint32_t eyes = 2;
			};
			auto& specializationConstants = shader.specializationConstants.get<SpecializationConstant>();

			struct PushConstant {
				uint32_t marchingSteps;
				uint32_t rayBounces;
				float shadowFactor;
				float reflectionStrength;
				float reflectionFalloff;
			};
			auto& pushConstant = shader.pushConstants.front().get<PushConstant>();
			pushConstant.marchingSteps = metadata["rays"]["marching steps"].as<size_t>();
			pushConstant.rayBounces = metadata["rays"]["ray bounces"].as<size_t>();
			pushConstant.shadowFactor = metadata["rays"]["shadow factor"].as<float>();
			pushConstant.reflectionStrength = metadata["rays"]["reflection"]["strength"].as<float>();
			pushConstant.reflectionFalloff = metadata["rays"]["reflection"]["falloff"].as<float>();

			auto& scene = uf::scene::getCurrentScene();
			auto& controller = scene.getController();
			auto& camera = controller.getComponent<uf::Camera>();
			auto& transform = controller.getComponent<pod::Transform<>>();
			
			auto& userdata = shader.uniforms.front();
			size_t uniforms_len = userdata.data().len;
			uint8_t* uniforms_buffer = (uint8_t*) (void*) userdata;
			UniformDescriptor* uniforms = (UniformDescriptor*) uniforms_buffer;
			for ( size_t i = 0; i < 2; ++i ) {
				uniforms->matrices[i] = uf::matrix::inverse( camera.getProjection(i) * camera.getView(i) );
			}

			{
				uniforms->ambient.x = metadata["light"]["ambient"][0].as<float>();
				uniforms->ambient.y = metadata["light"]["ambient"][1].as<float>();
				uniforms->ambient.z = metadata["light"]["ambient"][2].as<float>();
				uniforms->ambient.w = metadata["light"]["kexp"].as<float>();
			}
			{
				uniforms->fog.color.x = metadata["light"]["fog"]["color"][0].as<float>();
				uniforms->fog.color.y = metadata["light"]["fog"]["color"][1].as<float>();
				uniforms->fog.color.z = metadata["light"]["fog"]["color"][2].as<float>();
				
				uniforms->fog.range.x = metadata["light"]["fog"]["range"][0].as<float>();
				uniforms->fog.range.y = metadata["light"]["fog"]["range"][1].as<float>();
			}
		
			std::vector<uf::Entity*> entities;
			std::function<void(uf::Entity*)> filter = [&]( uf::Entity* entity ) {
				if ( !entity || entity->getName() != "Light" ) return;
				entities.push_back(entity);
			};
			for ( uf::Scene* scene : uf::scene::scenes ) { if ( !scene ) continue;
				scene->process(filter);
			}
			{
				const pod::Vector3& position = controller.getComponent<pod::Transform<>>().position;
				std::sort( entities.begin(), entities.end(), [&]( const uf::Entity* l, const uf::Entity* r ){
					if ( !l ) return false; if ( !r ) return true;
					if ( !l->hasComponent<pod::Transform<>>() ) return false; if ( !r->hasComponent<pod::Transform<>>() ) return true;
					return uf::vector::magnitude( uf::vector::subtract( l->getComponent<pod::Transform<>>().position, position ) ) < uf::vector::magnitude( uf::vector::subtract( r->getComponent<pod::Transform<>>().position, position ) );
				} );
			}

			{
				uf::Serializer& metadata = controller.getComponent<uf::Serializer>();
			//	if ( metadata["light"]["should"].as<bool>() )
					entities.push_back(&controller);
			}
			UniformDescriptor::Light* lights = (UniformDescriptor::Light*) &uniforms_buffer[sizeof(UniformDescriptor) - sizeof(UniformDescriptor::Light)];
			for ( size_t i = 0; i < specializationConstants.maxLights && i < entities.size(); ++i ) {
				UniformDescriptor::Light& light = lights[i];
				uf::Entity* entity = entities[i];

				pod::Transform<>& transform = entity->getComponent<pod::Transform<>>();
				uf::Serializer& metadata = entity->getComponent<uf::Serializer>();
				uf::Camera& camera = entity->getComponent<uf::Camera>();
				
				light.position.x = transform.position.x;
				light.position.y = transform.position.y;
				light.position.z = transform.position.z;

				if ( entity == &controller ) light.position.y += 2;

				light.position.w = metadata["light"]["radius"][1].as<float>();

				light.color.x = metadata["light"]["color"][0].as<float>();
				light.color.y = metadata["light"]["color"][1].as<float>();
				light.color.z = metadata["light"]["color"][2].as<float>();

				light.color.w = metadata["light"]["power"].as<float>();
			}
		
			shader.updateBuffer( (void*) uniforms_buffer, uniforms_len, 0  );
		}
	}
#endif
}
void ext::RayTracingSceneBehavior::render( uf::Object& self ) {
}
void ext::RayTracingSceneBehavior::destroy( uf::Object& self ) {
	if ( this->hasComponent<uf::renderer::ComputeRenderMode>() ) {
		auto& renderMode = this->getComponent<uf::renderer::ComputeRenderMode>();
		uf::renderer::removeRenderMode( &renderMode, false );
	}
}
#undef this
#endif