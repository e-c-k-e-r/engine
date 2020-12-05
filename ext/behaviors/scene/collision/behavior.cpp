#include "behavior.h"

#include <uf/utils/time/time.h>
#include <uf/utils/io/iostream.h>
#include <uf/utils/math/vector.h>
#include <uf/utils/math/transform.h>
#include <uf/utils/math/physics.h>
#include <uf/utils/window/window.h>

#include <uf/utils/audio/audio.h>
#include <uf/utils/thread/thread.h>
#include <uf/utils/camera/camera.h>

#include <uf/engine/asset/asset.h>
#include <uf/engine/asset/masterdata.h>

#include <uf/utils/renderer/renderer.h>

#include <uf/ext/gltf/gltf.h>

#include <uf/utils/math/collision.h>

#include <mutex>

#include <unordered_set>

UF_BEHAVIOR_REGISTER_CPP(ext::SceneCollisionBehavior)
#define this ((uf::Scene*) &self)

void ext::SceneCollisionBehavior::initialize( uf::Object& self ) {
	uf::instantiator::bind( "RenderBehavior", *this );
}
void ext::SceneCollisionBehavior::tick( uf::Object& self ) {
#if 0
	auto& metadata = this->getComponent<uf::Serializer>();

	if ( !metadata["system"]["physics"]["collision"].as<bool>() ) return;
	if ( this->hasComponent<std::mutex*>() ) {
		this->getComponent<std::mutex*>()->lock();
	}

	bool threaded = !metadata["system"]["physics"]["single threaded"].as<bool>();
	bool sort = metadata["system"]["physics"]["sort"].as<bool>();
	bool useStrongest = metadata["system"]["physics"]["use"]["strongest"].as<bool>();
	bool queued = metadata["system"]["physics"]["use"]["queue"].as<bool>();
	bool updatePhysics = !metadata["system"]["physics"]["optimizations"]["entity-local update"].as<bool>();
	bool ignoreStaticEntities = metadata["system"]["physics"]["optimizations"]["ignore static entities"].as<bool>();
	bool ignoreDuplicateTests = metadata["system"]["physics"]["optimizations"]["ignore duplicate tests"].as<bool>();
	bool useWorkers = metadata["system"]["physics"]["use"]["worker"].as<bool>();

	std::vector<std::function<int()>> jobs;
	std::vector<uf::Object*> entities; {
		// update physics
		if ( updatePhysics ) {
			this->process([&]( uf::Entity* entity ) {
				if ( !entity->hasComponent<pod::Physics>() ) return;
				auto& metadata = entity->getComponent<uf::Serializer>();
				auto& transform = entity->getComponent<pod::Transform<>>();
				auto& physics = entity->getComponent<pod::Physics>();
				if ( ext::json::isNull( metadata["system"]["physics"]["gravity"] ) ) {
					physics.linear.acceleration.x = metadata["system"]["physics"]["gravity"][0].as<float>();
					physics.linear.acceleration.y = metadata["system"]["physics"]["gravity"][1].as<float>();
					physics.linear.acceleration.z = metadata["system"]["physics"]["gravity"][2].as<float>();
				}
				if ( !metadata["system"]["physics"]["collision"].as<bool>() )  {
					physics.linear.acceleration.x = 0;
					physics.linear.acceleration.y = 0;
					physics.linear.acceleration.z = 0;
				}
				transform = uf::physics::update( transform, physics );
			});
		}
		this->process([&]( uf::Entity* entity ) {
			if ( !entity ) return;
			auto& metadata = entity->getComponent<uf::Serializer>();
			if ( !ext::json::isNull( metadata["system"]["physics"]["collision"] ) && !metadata["system"]["physics"]["collision"].as<bool>() ) return;
			if ( entity->hasComponent<uf::Collider>() )
				entities.push_back((uf::Object*) entity);
		});
		auto onCollision = []( pod::Collider::Manifold& manifold, size_t uidA, size_t uidB, bool queued ){
			uf::Serializer payload;
			payload["normal"][0] = manifold.normal.x;
			payload["normal"][1] = manifold.normal.y;
			payload["normal"][2] = manifold.normal.z;
			payload["entity"] = uidB;
			payload["depth"] = -manifold.depth;

			auto& scene = uf::scene::getCurrentScene();
			if ( queued ) scene.queueHook(uf::Object::formatHookName("world:Collision.%UID%",uidA), payload);
			else scene.callHook(uf::Object::formatHookName("world:Collision.%UID%",uidA), payload);
			
			payload["entity"] = uidA;
			payload["depth"] = manifold.depth;
			if ( queued ) scene.queueHook(uf::Object::formatHookName("world:Collision.%UID%",uidB), payload);
			else scene.callHook(uf::Object::formatHookName("world:Collision.%UID%",uidB), payload);
		};
		auto testColliders = [&]( uf::Collider& colliderA, uf::Collider& colliderB, size_t uidA, size_t uidB, bool useStrongest ){
			pod::Collider::Manifold strongest;
			auto manifolds = colliderA.intersects(colliderB);
			for ( auto manifold : manifolds ) {
				if ( manifold.colliding && manifold.depth > 0 ) {
					if ( !useStrongest ) onCollision(manifold, uidA, uidB, queued);
					else if ( strongest.depth < manifold.depth ) strongest = manifold;
				}
			}
			if ( useStrongest && strongest.colliding ) onCollision(strongest, uidA, uidB, queued);
		};
		// collide with others
		if ( ignoreDuplicateTests ) {
			struct Pair {
				uf::Object* a = NULL;
				uf::Object* b = NULL;
			};
			std::unordered_map<std::string, Pair> queued;
			for ( auto* _a : entities ) {
				uf::Object& entityA = *_a;
				if ( ignoreStaticEntities && !entityA.hasComponent<pod::Physics>() ) continue;
				for ( auto* _b : entities ) { if ( _a == _b ) continue;
					uf::Object& entityB = *_b;

					std::string hash = std::to_string(std::min( entityA.getUid(), entityB.getUid() )) + ":" + std::to_string(std::max( entityA.getUid(), entityB.getUid() ));
					if ( queued.count(hash) > 0 ) continue;

					queued[hash] = {
						.a = _a,
						.b = _b,
					};
				}
			}
			for ( auto& pair : queued ) {
				auto* entityA = pair.second.a;
				auto* entityB = pair.second.b;
				auto& colliderA = entityA->getComponent<uf::Collider>();
				auto& colliderB = entityB->getComponent<uf::Collider>();
				size_t uidA = entityA->getUid();
				size_t uidB = entityB->getUid();
				auto function = [&, uidA, uidB]() -> int {
					testColliders( colliderA, colliderB, uidA, uidB, useStrongest );
					return 0;
				};
				if ( threaded && useWorkers ) jobs.emplace_back(function); else function();
			}
		} else {
			for ( auto* _a : entities ) {
				uf::Object& entityA = *_a;
				size_t uidA = entityA.getUid();
				if ( ignoreStaticEntities && !entityA.hasComponent<pod::Physics>() ) continue;
				for ( auto* _b : entities ) { if ( _a == _b ) continue;
					uf::Object& entityB = *_b;
					auto& colliderA = entityA.getComponent<uf::Collider>();
					auto& colliderB = entityB.getComponent<uf::Collider>();
					size_t uidB = entityB.getUid();
					auto function = [&, uidA, uidB]() -> int {
						testColliders( colliderA, colliderB, uidA, uidB, useStrongest );
						return 0;
					};
					if ( threaded && useWorkers ) jobs.emplace_back(function); else function();
				}
			}
		}
	}
	if ( !jobs.empty() ) {
		if ( threaded ) uf::thread::batchWorkers( jobs );
		else {
			for ( auto& fun : jobs ) fun();
		}
	}
	if ( this->hasComponent<std::mutex*>() ) {
		this->getComponent<std::mutex*>()->lock();
	}
#endif
/*
	pod::Thread& thread = uf::thread::has("Physics") ? uf::thread::get("Physics") : uf::thread::create( "Physics", true, false );
	auto function = [&]() -> int {
		std::vector<uf::Object*> entities;
		// update physics
		if ( updatePhysics ) {
			this->process([&]( uf::Entity* entity ) {
				if ( !entity->hasComponent<pod::Physics>() ) return;
				auto& metadata = entity->getComponent<uf::Serializer>();
				auto& transform = entity->getComponent<pod::Transform<>>();
				auto& physics = entity->getComponent<pod::Physics>();
				if ( ext::json::isNull( metadata["system"]["physics"]["gravity"] ) ) {
					physics.linear.acceleration.x = metadata["system"]["physics"]["gravity"][0].as<float>();
					physics.linear.acceleration.y = metadata["system"]["physics"]["gravity"][1].as<float>();
					physics.linear.acceleration.z = metadata["system"]["physics"]["gravity"][2].as<float>();
				}
				if ( !metadata["system"]["physics"]["collision"].as<bool>() )  {
					physics.linear.acceleration.x = 0;
					physics.linear.acceleration.y = 0;
					physics.linear.acceleration.z = 0;
				}
				transform = uf::physics::update( transform, physics );
			});
		}
		this->process([&]( uf::Entity* entity ) {
			if ( !entity ) return;
			auto& metadata = entity->getComponent<uf::Serializer>();
			if ( !ext::json::isNull( metadata["system"]["physics"]["collision"] ) && !metadata["system"]["physics"]["collision"].as<bool>() ) return;
			if ( entity->hasComponent<uf::Collider>() )
				entities.push_back((uf::Object*) entity);
		});
		auto onCollision = []( pod::Collider::Manifold& manifold, size_t uidA, size_t uidB, bool queued ){
			uf::Serializer payload;
			payload["normal"][0] = manifold.normal.x;
			payload["normal"][1] = manifold.normal.y;
			payload["normal"][2] = manifold.normal.z;
			payload["entity"] = uidB;
			payload["depth"] = -manifold.depth;

			auto& scene = uf::scene::getCurrentScene();
			if ( queued ) scene.queueHook(uf::Object::formatHookName("world:Collision.%UID%",uidA), payload);
			else scene.callHook(uf::Object::formatHookName("world:Collision.%UID%",uidA), payload);
			
			payload["entity"] = uidA;
			payload["depth"] = manifold.depth;
			if ( queued ) scene.queueHook(uf::Object::formatHookName("world:Collision.%UID%",uidB), payload);
			else scene.callHook(uf::Object::formatHookName("world:Collision.%UID%",uidB), payload);
		};
		auto testColliders = [&]( uf::Collider& colliderA, uf::Collider& colliderB, size_t uidA, size_t uidB, bool useStrongest ){
			pod::Collider::Manifold strongest;
			auto manifolds = colliderA.intersects(colliderB);
			for ( auto manifold : manifolds ) {
				if ( manifold.colliding && manifold.depth > 0 ) {
					if ( !useStrongest ) onCollision(manifold, uidA, uidB, queued);
					else if ( strongest.depth < manifold.depth ) strongest = manifold;
				}
			}
			if ( useStrongest && strongest.colliding ) onCollision(strongest, uidA, uidB, queued);
		};
		// collide with others
		if ( ignoreDuplicateTests ) {
			struct Pair {
				uf::Object* a = NULL;
				uf::Object* b = NULL;
			};
			std::unordered_map<std::string, Pair> queued;
			for ( auto* _a : entities ) {
				uf::Object& entityA = *_a;
				if ( ignoreStaticEntities && !entityA.hasComponent<pod::Physics>() ) continue;
				for ( auto* _b : entities ) { if ( _a == _b ) continue;
					uf::Object& entityB = *_b;

					std::string hash = std::to_string(std::min( entityA.getUid(), entityB.getUid() )) + ":" + std::to_string(std::max( entityA.getUid(), entityB.getUid() ));
					if ( queued.count(hash) > 0 ) continue;

					queued[hash] = {
						.a = _a,
						.b = _b,
					};
				}
			}
			for ( auto& pair : queued ) {
				auto* entityA = pair.second.a;
				auto* entityB = pair.second.b;
				auto& colliderA = entityA->getComponent<uf::Collider>();
				auto& colliderB = entityB->getComponent<uf::Collider>();
				auto function = [&]() -> int {
					testColliders( colliderA, colliderB, entityA->getUid(), entityB->getUid(), useStrongest );
					return 0;
				};
				if ( threaded && useWorkers ) uf::thread::add( uf::thread::fetchWorker(), function, true ); else function();
			}
		} else {
			for ( auto* _a : entities ) {
				uf::Object& entityA = *_a;
				if ( ignoreStaticEntities && !entityA.hasComponent<pod::Physics>() ) continue;
				for ( auto* _b : entities ) { if ( _a == _b ) continue;
					uf::Object& entityB = *_b;
					auto& colliderA = entityA.getComponent<uf::Collider>();
					auto& colliderB = entityB.getComponent<uf::Collider>();
					auto function = [&]() -> int {
						testColliders( colliderA, colliderB, entityA.getUid(), entityB.getUid(), useStrongest );
						return 0;
					};
					if ( threaded && useWorkers ) uf::thread::add( uf::thread::fetchWorker(), function, true ); else function();
				}
			}
		}
		mutex.unlock();
		return 0;
	};
	if ( threaded ) uf::thread::add( thread, function, true ); else function();
*/
}

void ext::SceneCollisionBehavior::render( uf::Object& self ){}
void ext::SceneCollisionBehavior::destroy( uf::Object& self ){
/*
	{
		for ( int i = ext::bullet::dynamicsWorld->getNumCollisionObjects() - 1; i >= 0; i--) {
			btCollisionObject* obj = ext::bullet::dynamicsWorld->getCollisionObjectArray()[i];
			btRigidBody* body = btRigidBody::upcast(obj);
			if (body && body->getMotionState()) {
				delete body->getMotionState();
			}
			ext::bullet::dynamicsWorld->removeCollisionObject(obj);
			delete obj;
		}
		for ( size_t i = 0; i < ext::bullet::collisionShapes.size(); ++i ) {
			btCollisionShape* shape = ext::bullet::collisionShapes[i];
			ext::bullet::collisionShapes[i] = NULL;
			delete shape;
		}

		delete ext::bullet::dynamicsWorld;
		delete ext::bullet::solver;
		delete ext::bullet::overlappingPairCache;
		delete ext::bullet::dispatcher;
		delete ext::bullet::collisionConfiguration;
		ext::bullet::collisionShapes.clear();
	}
*/
#if 0
	{
		auto& mutexPointer = this->getComponent<std::mutex*>();
		delete mutexPointer;
		mutexPointer = NULL;
	}
#endif
}
#undef this