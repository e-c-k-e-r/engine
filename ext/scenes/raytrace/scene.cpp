#include "scene.h"

#include <uf/utils/time/time.h>
#include <uf/utils/io/iostream.h>
#include <uf/utils/math/vector.h>
#include <uf/utils/math/transform.h>
#include <uf/utils/window/window.h>

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

EXT_OBJECT_REGISTER_CPP(TestScene_RayTracing)
void ext::TestScene_RayTracing::initialize() {
	ext::Scene::initialize();
	uf::Serializer& metadata = this->getComponent<uf::Serializer>();
	uf::Asset& assetLoader = this->getComponent<uf::Asset>();

	{
		auto& renderMode = this->getComponent<uf::renderer::ComputeRenderMode>();
		std::string name = "C:RT:" + std::to_string((int) this->getUid());
		uf::renderer::addRenderMode( &renderMode, name );
		if ( metadata["light"]["shadows"]["resolution"].isArray() ) {
			renderMode.width = metadata["light"]["shadows"]["resolution"][0].asUInt64();
			renderMode.height = metadata["light"]["shadows"]["resolution"][1].asUInt64();
		} else {
			renderMode.width = metadata["light"]["shadows"]["resolution"].asUInt64();
			renderMode.height = metadata["light"]["shadows"]["resolution"].asUInt64();
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
				VK_BUFFER_USAGE_VERTEX_BUFFER_BIT | VK_BUFFER_USAGE_STORAGE_BUFFER_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT,
				VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT,
				true
			);
		}
	}
}

void ext::TestScene_RayTracing::render() {
	ext::Scene::render();
}
void ext::TestScene_RayTracing::destroy() {
	if ( this->hasComponent<uf::renderer::ComputeRenderMode>() ) {
		auto& renderMode = this->getComponent<uf::renderer::ComputeRenderMode>();
		uf::renderer::removeRenderMode( &renderMode, false );
	}
	ext::Scene::destroy();
}
void ext::TestScene_RayTracing::tick() {
	ext::Scene::tick();

	uf::Serializer& metadata = this->getComponent<uf::Serializer>();
	uf::Asset& assetLoader = this->getComponent<uf::Asset>();
#if 1
	if ( this->hasComponent<uf::renderer::ComputeRenderMode>() ) {
		auto& renderMode = this->getComponent<uf::renderer::ComputeRenderMode>();
		/* Add lights to scene */ if ( renderMode.compute.initialized ) {
			struct UniformDescriptor {
				alignas(16) pod::Matrix4f matrices[2];
				alignas(16) pod::Vector4f ambient;
				struct {
					alignas(8) pod::Vector2f range;
					alignas(16) pod::Vector4f color;
				} fog;
				struct Light {
					alignas(16) pod::Vector4f position;
					alignas(16) pod::Vector4f color;
					alignas(8) pod::Vector2i type;
					alignas(16) pod::Matrix4f view;
					alignas(16) pod::Matrix4f projection;
				} lights;
			};
			
			struct SpecializationConstant {
				int32_t eyes = 2;
				int32_t maxLights = 16;
			} specializationConstants;
			
			auto& shader = renderMode.compute.material.shaders.front();
			specializationConstants = shader.specializationConstants.get<SpecializationConstant>();

			struct PushConstant {
				uint32_t marchingSteps;
				uint32_t rayBounces;
				float shadowFactor;
				float reflectionStrength;
				float reflectionFalloff;
			};
			auto& pushConstant = shader.pushConstants.front().get<PushConstant>();
			pushConstant.marchingSteps = metadata["rays"]["marching steps"].asUInt64();
			pushConstant.rayBounces = metadata["rays"]["ray bounces"].asUInt64();
			pushConstant.shadowFactor = metadata["rays"]["shadow factor"].asFloat();
			pushConstant.reflectionStrength = metadata["rays"]["reflection"]["strength"].asFloat();
			pushConstant.reflectionFalloff = metadata["rays"]["reflection"]["falloff"].asFloat();

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
				uniforms->ambient.x = metadata["light"]["ambient"][0].asFloat();
				uniforms->ambient.y = metadata["light"]["ambient"][1].asFloat();
				uniforms->ambient.z = metadata["light"]["ambient"][2].asFloat();
				uniforms->ambient.w = metadata["light"]["kexp"].asFloat();
			}
			{
				uniforms->fog.color.x = metadata["light"]["fog"]["color"][0].asFloat();
				uniforms->fog.color.y = metadata["light"]["fog"]["color"][1].asFloat();
				uniforms->fog.color.z = metadata["light"]["fog"]["color"][2].asFloat();
				
				uniforms->fog.range.x = metadata["light"]["fog"]["range"][0].asFloat();
				uniforms->fog.range.y = metadata["light"]["fog"]["range"][1].asFloat();
			}
		
			std::vector<uf::Entity*> entities;
			std::function<void(uf::Entity*)> filter = [&]( uf::Entity* entity ) {
				if ( !entity || entity->getName() != "Light" ) return;
				entities.push_back(entity);
			};
			for ( uf::Scene* scene : uf::renderer::scenes ) { if ( !scene ) continue;
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
				if ( metadata["light"]["should"].asBool() ) entities.push_back(&controller);
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

				light.position.w = metadata["light"]["radius"][1].asFloat();

				light.color.x = metadata["light"]["color"][0].asFloat();
				light.color.y = metadata["light"]["color"][1].asFloat();
				light.color.z = metadata["light"]["color"][2].asFloat();

				light.color.w = metadata["light"]["power"].asFloat();
			}
		

			shader.updateBuffer( (void*) uniforms_buffer, uniforms_len, 0, false );
		}
	}
#endif

	/* Collision */ {
		bool local = false;
		bool sort = false;
		bool useStrongest = false;
	//	pod::Thread& thread = uf::thread::fetchWorker();
		pod::Thread& thread = uf::thread::has("Physics") ? uf::thread::get("Physics") : uf::thread::create( "Physics", true, false );
		auto function = [&]() -> int {
			std::vector<uf::Object*> entities;
			std::function<void(uf::Entity*)> filter = [&]( uf::Entity* entity ) {
				auto& metadata = entity->getComponent<uf::Serializer>();
				if ( !metadata["system"]["physics"]["collision"].isNull() && !metadata["system"]["physics"]["collision"].asBool() ) return;
				if ( entity->hasComponent<uf::Collider>() )
					entities.push_back((uf::Object*) entity);
			};
			this->process(filter);
			auto onCollision = []( pod::Collider::Manifold& manifold, uf::Object* a, uf::Object* b ){				
				uf::Serializer payload;
				payload["normal"][0] = manifold.normal.x;
				payload["normal"][1] = manifold.normal.y;
				payload["normal"][2] = manifold.normal.z;
				payload["entity"] = b->getUid();
				payload["depth"] = -manifold.depth;
				a->callHook("world:Collision.%UID%", payload);
				
				payload["entity"] = a->getUid();
				payload["depth"] = manifold.depth;
				b->callHook("world:Collision.%UID%", payload);
			};
			auto testColliders = [&]( uf::Collider& colliderA, uf::Collider& colliderB, uf::Object* a, uf::Object* b, bool useStrongest ){
				pod::Collider::Manifold strongest;
				auto manifolds = colliderA.intersects(colliderB);
				for ( auto manifold : manifolds ) {
					if ( manifold.colliding && manifold.depth > 0 ) {
						if ( !useStrongest ) onCollision(manifold, a, b);
						else if ( strongest.depth < manifold.depth ) strongest = manifold;
					}
				}
				if ( useStrongest && strongest.colliding ) onCollision(strongest, a, b);
			};
			// collide with others
			for ( auto* _a : entities ) {
				uf::Object& entityA = *_a;
				for ( auto* _b : entities ) { if ( _a == _b ) continue;
					uf::Object& entityB = *_b;
					testColliders( entityA.getComponent<uf::Collider>(), entityB.getComponent<uf::Collider>(), &entityA, &entityB, useStrongest );
				}
			}
		
			return 0;
		};
		if ( local ) function(); else uf::thread::add( thread, function, true );
	}
}