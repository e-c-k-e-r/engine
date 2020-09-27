#include "craeture.h"

#include <uf/utils/hook/hook.h>
#include <uf/utils/time/time.h>
#include <uf/utils/serialize/serializer.h>
#include <uf/utils/userdata/userdata.h>
#include <uf/utils/window/window.h>
#include <uf/utils/camera/camera.h>
#include <uf/utils/thread/thread.h>
#include <uf/utils/string/ext.h>
#include <uf/utils/audio/audio.h>
#include <uf/utils/math/transform.h>
#include <uf/utils/math/collision.h>

#include "..//battle.h"

#include "../terrain/generator.h"
#include "../scene.h"

#include <uf/engine/asset/asset.h>

EXT_OBJECT_REGISTER_CPP(Craeture)

void ext::Craeture::initialize() {	
	uf::Object::initialize();

	pod::Transform<>& transform = this->getComponent<pod::Transform<>>();
	transform = uf::transform::initialize(transform); {
		this->getComponent<uf::Camera>().getTransform().reference = this->getComponentPointer<pod::Transform<>>();
	}

	pod::Physics& physics = this->getComponent<pod::Physics>();
	physics.linear.velocity = {0,0,0};
	physics.linear.acceleration = {0,-9.81,0};
	physics.rotational.velocity = uf::quaternion::axisAngle( {0,1,0}, (pod::Math::num_t) 0 );
	physics.rotational.acceleration = {0,0,0,0};

	uf::Serializer& metadata = this->getComponent<uf::Serializer>();
/*
	if ( metadata["animation"] != Json::nullValue ) {
		for( auto it = metadata["animation"]["names"].begin() ; it != metadata["animation"]["names"].end() ; ++it ) { 
			const std::string& name = it->asString();
			if ( this->m_animation.transforms.find(name) == this->m_animation.transforms.end() ) this->m_animation.transforms[name];
		}
	}
*/
	/* Gravity */ {
		if ( metadata["system"]["physics"]["gravity"] != Json::nullValue ) {
			physics.linear.acceleration.x = metadata["system"]["physics"]["gravity"][0].asFloat();
			physics.linear.acceleration.y = metadata["system"]["physics"]["gravity"][1].asFloat();
			physics.linear.acceleration.z = metadata["system"]["physics"]["gravity"][2].asFloat();
		}
		if ( !metadata["system"]["physics"]["collision"].asBool() )  {
			physics.linear.acceleration.x = 0;
			physics.linear.acceleration.y = 0;
			physics.linear.acceleration.z = 0;
		}
	}
	/* Collider */ {
		uf::Collider& collider = this->getComponent<uf::Collider>();

		collider.clear();
		auto* box = new uf::BoundingBox( {0, 1.5, 0}, {0.7, 1.6, 0.7} );
		box->getTransform().reference = &transform;
		collider.add(box);
	}
	/* RPG */ {
		ext::HousamoBattle& battle = this->getComponent<ext::HousamoBattle>();
	}

	// Hooks
/*
	struct {
		uf::Timer<long long> flash = uf::Timer<long long>(false);
		uf::Timer<long long> sound = uf::Timer<long long>(false);
	} timers;
*/
	static uf::Timer<long long> timer(true);
	this->addHook( "world:Collision.%UID%", [&](const std::string& event)->std::string{	
		uf::Serializer json = event;

		size_t uid = json["uid"].asUInt64();
		// do not collide with children
		// if ( this->findByUid(uid) ) return "false";

		pod::Vector3 normal;
		float depth = json["depth"].asFloat() * 1.001f;

		normal.x = json["normal"][0].asFloat();
		normal.y = json["normal"][1].asFloat();
		normal.z = json["normal"][2].asFloat();
		pod::Vector3 correction = normal * depth;

		transform.position -= correction;

		if ( normal.x == 1 || normal.x == -1 ) physics.linear.velocity.x = 0;
		if ( normal.y == 1 || normal.y == -1 ) physics.linear.velocity.y = 0;
		if ( normal.z == 1 || normal.z == -1 ) physics.linear.velocity.z = 0;
		return "true";
	});
	this->addHook( "asset:Cache.Sound.%UID%", [&](const std::string& event)->std::string{	
		uf::Serializer json = event;
		
		uf::Scene& world = uf::scene::getCurrentScene();
		uf::Serializer& masterdata = world.getComponent<uf::Serializer>();
		
		std::string filename = json["filename"].asString();

		if ( uf::string::extension(filename) != "ogg" ) return "false";

		if ( filename == "" ) return "false";
		uf::Audio& sfx = this->getComponent<uf::SoundEmitter>().add(filename);
		sfx.setVolume(masterdata["volumes"]["sfx"].asFloat());
		auto& pTransform = world.getController().getComponent<pod::Transform<>>();
		sfx.setPosition( transform.position );
		sfx.play();

		return "true";
	});
	this->addHook( "world:Craeture.OnHit.%UID%", [&](const std::string& event)->std::string{	
		uf::Serializer json = event;
		uint64_t phase = json["phase"].asUInt64();
		// start color
		pod::Vector4f color = { 1, 1, 1, 0 };
		if ( phase == 0 ) {
			color = pod::Vector4f{
				json["color"][0].asFloat(),
				json["color"][1].asFloat(),
				json["color"][2].asFloat(),
				json["color"][3].asFloat(),
			};
		}
		metadata["color"][0] = color[0];
		metadata["color"][1] = color[1];
		metadata["color"][2] = color[2];
		metadata["color"][3] = color[3];

		if ( metadata["timers"]["hurt"].asFloat() < timer.elapsed().asDouble() ) {

			metadata["timers"]["hurt"] = timer.elapsed().asDouble() + 1.0f;
			uf::Scene& scene = uf::scene::getCurrentScene();
			uf::Asset& assetLoader = scene.getComponent<uf::Asset>();
			assetLoader.cache("./data/audio/battle/hurt.ogg", "asset:Cache.Sound." + std::to_string(this->getUid()));
		}

		return "true";
	});
	this->addHook( "world:Craeture.Hurt.%UID%", [&](const std::string& event)->std::string{	
		uf::Serializer json = event;
		if ( metadata["timers"]["flash"].asFloat() < timer.elapsed().asDouble() ) {
			metadata["timers"]["flash"] = timer.elapsed().asDouble() + 0.4f;
			for ( int i = 0; i < 16; ++i ) {
				uf::Serializer payload;
				payload["phase"] = i % 2 == 0 ? 0 : 1;
				payload["color"][0] = 1.0f;
				payload["color"][1] = 0.0f;
				payload["color"][2] = 0.0f;
				payload["color"][3] = 0.6f;
				this->queueHook("world:Craeture.OnHit.%UID%", payload, 0.05f * i);
			}
		}

		return "true";
	});
}

void ext::Craeture::tick() {
	uf::Object::tick();

	uf::Serializer& metadata = this->getComponent<uf::Serializer>();
	uf::Scene& world = uf::scene::getCurrentScene();
	uf::Serializer& pMetadata = world.getController().getComponent<uf::Serializer>();

	if ( !pMetadata["system"]["control"].asBool() ) return;

	pod::Transform<>& transform = this->getComponent<pod::Transform<>>();
	pod::Physics& physics = this->getComponent<pod::Physics>();
	/* Gravity */ {
		if ( metadata["system"]["physics"]["gravity"] != Json::nullValue ) {
			physics.linear.acceleration.x = metadata["system"]["physics"]["gravity"][0].asFloat();
			physics.linear.acceleration.y = metadata["system"]["physics"]["gravity"][1].asFloat();
			physics.linear.acceleration.z = metadata["system"]["physics"]["gravity"][2].asFloat();
		}
		if ( !metadata["system"]["physics"]["collision"].asBool() )  {
			physics.linear.acceleration.x = 0;
			physics.linear.acceleration.y = 0;
			physics.linear.acceleration.z = 0;
		}
	}
	transform = uf::physics::update( transform, physics );
#if 0
	if ( !false ) {
		bool sort = false;
		bool local = false;
		bool useStrongest = true;
		pod::Thread& thread = uf::thread::has("Physics") ? uf::thread::get("Physics") : uf::thread::create( "Physics", true, false );
		auto onCollision = []( pod::Collider::Manifold& manifold, uf::Object* a, uf::Object* b ){				
			pod::Transform<>& transform = b->getComponent<pod::Transform<>>();
			pod::Physics& physics = b->getComponent<pod::Physics>();

			uf::Serializer payload;
			pod::Vector3 correction = manifold.normal * manifold.depth;
			transform.position -= correction;

			if ( manifold.normal.x == 1 || manifold.normal.x == -1 ) physics.linear.velocity.x = 0;
			if ( manifold.normal.y == 1 || manifold.normal.y == -1 ) physics.linear.velocity.y = 0;
			if ( manifold.normal.z == 1 || manifold.normal.z == -1 ) physics.linear.velocity.z = 0;
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
		/* Collision against world */ {
			auto function = [&]() -> int {
				if ( this->hasParent() && metadata["system"]["physics"]["collision"].asBool() ) {
					if ( !this->hasParent() ) { return 0; }
					if ( this->getParent().getName() != "Region" ) { return 0; }

					uf::Entity& parent = this->getParent();
					ext::TerrainGenerator& generator = parent.getComponent<ext::TerrainGenerator>();
					uf::Serializer& rMetadata = parent.getComponent<uf::Serializer>();
					pod::Transform<>& rTransform = parent.getComponent<pod::Transform<>>();

					pod::Vector3ui size; {
						size.x = rMetadata["region"]["size"][0].asUInt();
						size.y = rMetadata["region"]["size"][1].asUInt();
						size.z = rMetadata["region"]["size"][2].asUInt();
					}
					pod::Vector3f voxelPosition = transform.position - rTransform.position;
					voxelPosition.x += size.x / 2.0f;
					voxelPosition.y += size.y / 2.0f + 1;
					voxelPosition.z += size.z / 2.0f;	

					uf::Collider pCollider;
					std::vector<pod::Vector3ui> positions = {
						{ voxelPosition.x, voxelPosition.y, voxelPosition.z },
						{ voxelPosition.x - 1, voxelPosition.y, voxelPosition.z },
						{ voxelPosition.x + 1, voxelPosition.y, voxelPosition.z },
						{ voxelPosition.x, voxelPosition.y - 1, voxelPosition.z },
						{ voxelPosition.x, voxelPosition.y + 1, voxelPosition.z },
						{ voxelPosition.x, voxelPosition.y, voxelPosition.z - 1 },
						{ voxelPosition.x, voxelPosition.y, voxelPosition.z + 1},
					};
					if ( this->m_name == "HousamoSprite" ) {
						// bottom
						uint16_t uid = generator.getVoxel( voxelPosition.x, voxelPosition.y, voxelPosition.z );
						auto light = generator.getLight( voxelPosition.x, voxelPosition.y, voxelPosition.z );
						metadata["color"][0] = ((light >> 12) & 0xF) / (float) (0xF);
						metadata["color"][1] = ((light >>  8) & 0xF) / (float) (0xF);
						metadata["color"][2] = ((light >>  4) & 0xF) / (float) (0xF);
						metadata["color"][3] = ((light      ) & 0xF) / (float) (0xF);
					//	if ( uid == ext::TerrainVoxelLava().uid() ) this->callHook("world:Craeture.Hurt.%UID%");
					}

					for ( auto& position : positions ) {
						ext::TerrainVoxel voxel = ext::TerrainVoxel::atlas( generator.getVoxel( position.x, position.y, position.z ) );
						pod::Vector3 offset = rTransform.position;
						offset.x += position.x - (size.x / 2.0f);
						offset.y += position.y - (size.y / 2.0f);
						offset.z += position.z - (size.z / 2.0f);

						if ( !voxel.solid() ) continue;

						pCollider.add( new uf::BoundingBox( offset, {0.5, 0.5, 0.5} ) );
					}

					uf::Collider& collider = this->getComponent<uf::Collider>();
					pod::Transform<>& transform = this->getComponent<pod::Transform<>>();
				
					{
						collider.clear();
						collider.add(new uf::BoundingBox( uf::vector::add({0, 1.5, 0}, transform.position), {0.7, 1.6, 0.7} ));
					}
				

					pod::Physics& physics = this->getComponent<pod::Physics>();

					auto result = pCollider.intersects(collider);
					pod::Collider::Manifold strongest;

					for ( auto manifold : result ) {
						if ( manifold.colliding && manifold.depth > 0 ) {
							if ( strongest.depth < manifold.depth ) strongest = manifold;
							if ( !useStrongest ) {
								pod::Vector3 correction = manifold.normal * manifold.depth;
								transform.position -= correction;

								if ( manifold.normal.x == 1 || manifold.normal.x == -1 ) physics.linear.velocity.x = 0;
								if ( manifold.normal.y == 1 || manifold.normal.y == -1 ) physics.linear.velocity.y = 0;
								if ( manifold.normal.z == 1 || manifold.normal.z == -1 ) physics.linear.velocity.z = 0;
							}
						}
					}
					if ( useStrongest && strongest.colliding ) {
						pod::Vector3 correction = strongest.normal * strongest.depth;
						transform.position -= correction;

						if ( strongest.normal.x == 1 || strongest.normal.x == -1 ) physics.linear.velocity.x = 0;
						if ( strongest.normal.y == 1 || strongest.normal.y == -1 ) physics.linear.velocity.y = 0;
						if ( strongest.normal.z == 1 || strongest.normal.z == -1 ) physics.linear.velocity.z = 0;
					}
				}
				return 0;
			};
			if ( local ) function(); else uf::thread::add( thread, function, true );
		}

		/* Collision against other craetures */ {
			auto function = [&]() -> int {
				/* Collision */ if ( this->hasParent() && metadata["system"]["physics"]["collision"].asBool() ) {
					if ( !this->hasParent() ) { return 0; }
					if ( this->getParent().getName() != "Region" ) { return 0; }

					uf::Entity& parent = this->getParent();

					int regions = 0;
					std::vector<uf::Entity*> entities;
					/* Retrieve close entities */ for ( uf::Entity* kv : parent.getChildren() ) {
						if ( !kv ) continue;
						if ( kv->getUid() == this->getUid() ) continue;
						if ( !kv->hasComponent<uf::Collider>() ) continue;
						entities.push_back(kv);
					}
					/* Sort by closest to farthest */ if ( sort ) {
						const pod::Vector3& position = this->getComponent<pod::Transform<>>().position;
						std::sort( entities.begin(), entities.end(), [&]( const uf::Entity* l, const uf::Entity* r ){
							if ( !l ) return false; if ( !r ) return true;
							if ( l->getUid() == 0 ) return false; if ( r->getUid() == 0 ) return true;
							if ( !l->hasComponent<pod::Transform<>>() ) return false; if ( !r->hasComponent<pod::Transform<>>() ) return true;
							return uf::vector::magnitude( uf::vector::subtract( l->getComponent<pod::Transform<>>().position, position ) ) < uf::vector::magnitude( uf::vector::subtract( r->getComponent<pod::Transform<>>().position, position ) );
						} );
					}

					for ( uf::Entity* e : entities ) {
						if ( e->getUid() == 0 ) continue;

						uf::Collider& collider = this->getComponent<uf::Collider>();
						uf::Collider& pCollider = e->getComponent<uf::Collider>();

						pod::Physics& physics = this->getComponent<pod::Physics>();
						pod::Transform<>& transform = this->getComponent<pod::Transform<>>();
					
						{
							collider.clear();
							collider.add(new uf::BoundingBox( uf::vector::add({0, 1.5, 0}, transform.position), {0.7, 1.6, 0.7} ));
						}
					
						
						auto result = pCollider.intersects(collider);
						
						pod::Collider::Manifold strongest;

						for ( auto manifold : result ) {
							if ( manifold.colliding && manifold.depth > 0 ) {
								if ( strongest.depth < manifold.depth ) strongest = manifold;
								if ( !useStrongest ) {
									pod::Vector3 correction = manifold.normal * manifold.depth;
									transform.position += correction;
									if ( manifold.normal.x == 1 || manifold.normal.x == -1 ) physics.linear.velocity.x = 0;
									if ( manifold.normal.y == 1 || manifold.normal.y == -1 ) physics.linear.velocity.y = 0;
									if ( manifold.normal.z == 1 || manifold.normal.z == -1 ) physics.linear.velocity.z = 0;
								}
							}
						}

						uf::Serializer& metadata = this->getComponent<uf::Serializer>();
						uf::Serializer& pSerializer = e->getComponent<uf::Serializer>();


						if ( useStrongest && strongest.colliding ) {
							// signal collision, if available
							pod::Vector3 correction = strongest.normal * strongest.depth;
							{
								uf::Serializer payload;
								payload["entity"] = e->getUid();
								payload["correction"]["x"] = correction.x;
								payload["correction"]["y"] = correction.y;
								payload["correction"]["z"] = correction.z;
								payload["depth"] = strongest.depth;
								payload["normal"]["x"] = strongest.normal.x;
								payload["normal"]["y"] = strongest.normal.y;
								payload["normal"]["z"] = strongest.normal.z;
								uf::hooks.call("world:Collision." + std::to_string(this->getUid()), payload);
							}
							transform.position -= correction;

							if ( strongest.normal.x == 1 || strongest.normal.x == -1 ) physics.linear.velocity.x = 0;
							if ( strongest.normal.y == 1 || strongest.normal.y == -1 ) physics.linear.velocity.y = 0;
							if ( strongest.normal.z == 1 || strongest.normal.z == -1 ) physics.linear.velocity.z = 0;
						}
					}
				}
				return 0;
			};
			if ( local ) function(); else uf::thread::add( thread, function, true );
		}
	} else if ( false ) {
		bool sort = false;
		bool local = false;
		bool useStrongest = true;
		pod::Thread& thread = uf::thread::has("Physics") ? uf::thread::get("Physics") : uf::thread::create( "Physics", true, false );
		auto onCollision = []( pod::Collider::Manifold& manifold, uf::Object* a, uf::Object* b ){				
			pod::Transform<>& transform = b->getComponent<pod::Transform<>>();
			pod::Physics& physics = b->getComponent<pod::Physics>();

			uf::Serializer payload;
			pod::Vector3 correction = manifold.normal * manifold.depth;
			transform.position -= correction;

			if ( manifold.normal.x == 1 || manifold.normal.x == -1 ) physics.linear.velocity.x = 0;
			if ( manifold.normal.y == 1 || manifold.normal.y == -1 ) physics.linear.velocity.y = 0;
			if ( manifold.normal.z == 1 || manifold.normal.z == -1 ) physics.linear.velocity.z = 0;
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
		/* Collision against world */ {
			auto function = [&]() -> int {
				if ( this->hasParent() && metadata["system"]["physics"]["collision"].asBool() ) {
					if ( !this->hasParent() ) { return 0; }
					if ( this->getParent().getName() != "Region" ) { return 0; }

					uf::Entity& parent = this->getParent();
					ext::TerrainGenerator& generator = parent.getComponent<ext::TerrainGenerator>();
					uf::Serializer& rMetadata = parent.getComponent<uf::Serializer>();
					pod::Transform<>& rTransform = parent.getComponent<pod::Transform<>>();

					pod::Vector3ui size; {
						size.x = rMetadata["region"]["size"][0].asUInt();
						size.y = rMetadata["region"]["size"][1].asUInt();
						size.z = rMetadata["region"]["size"][2].asUInt();
					}
					pod::Vector3f voxelPosition = transform.position - rTransform.position;
					voxelPosition.x += size.x / 2.0f;
					voxelPosition.y += size.y / 2.0f + 1;
					voxelPosition.z += size.z / 2.0f;	

					uf::Collider pCollider;
					std::vector<pod::Vector3ui> positions = {
						{ voxelPosition.x, voxelPosition.y, voxelPosition.z },
						{ voxelPosition.x - 1, voxelPosition.y, voxelPosition.z },
						{ voxelPosition.x + 1, voxelPosition.y, voxelPosition.z },
						{ voxelPosition.x, voxelPosition.y - 1, voxelPosition.z },
						{ voxelPosition.x, voxelPosition.y + 1, voxelPosition.z },
						{ voxelPosition.x, voxelPosition.y, voxelPosition.z - 1 },
						{ voxelPosition.x, voxelPosition.y, voxelPosition.z + 1},
					};
					if ( this->m_name == "HousamoSprite" ) {
						uint16_t uid = generator.getVoxel( voxelPosition.x, voxelPosition.y, voxelPosition.z );
						auto light = generator.getLight( voxelPosition.x, voxelPosition.y, voxelPosition.z );
						metadata["color"][0] = ((light >> 12) & 0xF) / (float) (0xF);
						metadata["color"][1] = ((light >>  8) & 0xF) / (float) (0xF);
						metadata["color"][2] = ((light >>  4) & 0xF) / (float) (0xF);
						metadata["color"][3] = ((light      ) & 0xF) / (float) (0xF);
					//	if ( uid == ext::TerrainVoxelLava().uid() ) this->callHook("world:Craeture.Hurt.%UID%");
					}

					for ( auto& position : positions ) {
						ext::TerrainVoxel voxel = ext::TerrainVoxel::atlas( generator.getVoxel( position.x, position.y, position.z ) );
						pod::Vector3 offset = rTransform.position;
						offset.x += position.x - (size.x / 2.0f);
						offset.y += position.y - (size.y / 2.0f);
						offset.z += position.z - (size.z / 2.0f);

						if ( !voxel.solid() ) continue;

						pCollider.add( new uf::BoundingBox( offset, {0.5, 0.5, 0.5} ) );
					}

					uf::Collider& collider = this->getComponent<uf::Collider>();
					testColliders( pCollider, collider, (uf::Object*) &parent, this, useStrongest );
				}
				return 0;
			};
			if ( local ) function(); else uf::thread::add( thread, function, true );
		}
	} else if ( false ) {
		bool local = false;
		bool sort = false;
		bool useStrongest = false;
		pod::Thread& thread = uf::thread::has("Physics") ? uf::thread::get("Physics") : uf::thread::create( "Physics", true, false );
		auto function = [&]() -> int {
			std::vector<uf::Object*> entities;
			std::function<void(uf::Entity*)> filter = [&]( uf::Entity* entity ) {
				auto& metadata = entity->getComponent<uf::Serializer>();
				if ( !metadata["system"]["physics"]["collision"].isNull() && !metadata["system"]["physics"]["collision"].asBool() ) return;
				if ( entity->hasComponent<uf::Collider>() )
					entities.push_back((uf::Object*) entity);
			};
			uf::Entity* regionPointer = &this->getParent();
			if ( regionPointer->getName() != "Region" ) return 0;
			ext::Region& region = *(ext::Region*) regionPointer;
			region.process(filter);
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

			// collide with world
			auto& metadata = region.getComponent<uf::Serializer>();
			auto& generator = region.getComponent<ext::TerrainGenerator>();
			auto& regionPosition = region.getComponent<pod::Transform<>>().position;
			pod::Vector3f size; {
				size.x = metadata["region"]["size"][0].asUInt();
				size.y = metadata["region"]["size"][1].asUInt();
				size.z = metadata["region"]["size"][2].asUInt();
			}
			for ( auto* _ : entities ) {
				uf::Object& entity = *_;
				auto& transform = entity.getComponent<pod::Transform<>>();
			
				pod::Vector3f voxelPosition = transform.position - regionPosition;
				voxelPosition.x += size.x / 2.0f;
				voxelPosition.y += size.y / 2.0f + 1;
				voxelPosition.z += size.z / 2.0f;

				uf::Collider collider;
				std::vector<pod::Vector3ui> positions = {
					{ voxelPosition.x, voxelPosition.y, voxelPosition.z },
					{ voxelPosition.x - 1, voxelPosition.y, voxelPosition.z },
					{ voxelPosition.x + 1, voxelPosition.y, voxelPosition.z },
					{ voxelPosition.x, voxelPosition.y - 1, voxelPosition.z },
					{ voxelPosition.x, voxelPosition.y + 1, voxelPosition.z },
					{ voxelPosition.x, voxelPosition.y, voxelPosition.z - 1 },
					{ voxelPosition.x, voxelPosition.y, voxelPosition.z + 1},
				};
				for ( auto& position : positions ) {
					ext::TerrainVoxel voxel = ext::TerrainVoxel::atlas( generator.getVoxel( position.x, position.y, position.z ) );
					pod::Vector3 offset = regionPosition;
					offset.x += position.x - (size.x / 2.0f);
					offset.y += position.y - (size.y / 2.0f);
					offset.z += position.z - (size.z / 2.0f);

					if ( !voxel.solid() ) continue;

					collider.add( new uf::BoundingBox( offset, {0.5, 0.5, 0.5} ) );
				/*
					uf::BaseMesh<pod::Vertex_3F> mesh;
					const ext::TerrainVoxel::Model& model = voxel.model();
					#define TERRAIN_SHOULD_RENDER_FACE(SIDE)\
						for ( uint i = 0; i < model.position.SIDE.size() / 3; ++i ) {\
							auto& vertex = mesh.vertices.emplace_back();\
							{\
								pod::Vector3f& p = vertex.position;\
								p.x = model.position.SIDE[i*3+0]; p.y = model.position.SIDE[i*3+1]; p.z = model.position.SIDE[i*3+2];\
								p.x += offset.x; p.y += offset.y; p.z += offset.z;\
							}\
						}
					TERRAIN_SHOULD_RENDER_FACE(left)
					TERRAIN_SHOULD_RENDER_FACE(right)
					TERRAIN_SHOULD_RENDER_FACE(top)
					TERRAIN_SHOULD_RENDER_FACE(bottom)
					TERRAIN_SHOULD_RENDER_FACE(back)
					TERRAIN_SHOULD_RENDER_FACE(front)
					uf::MeshCollider* mCollider = new uf::MeshCollider();
					mCollider->setPositions( mesh );
					pCollider.add(mCollider);
				*/
				}
				testColliders( collider, entity.getComponent<uf::Collider>(), this, &entity, useStrongest );
			}
		
			// collide with others
			for ( auto* _a : entities ) {
				uf::Object& entityA = *_a;
				for ( auto* _b : entities ) { if ( _a == _b ) continue;
					uf::Object& entityB = *_b;
					{
						uf::Collider& collider = entityB.getComponent<uf::Collider>();
						pod::Transform<>& transform = entityB.getComponent<pod::Transform<>>(); 
						collider.clear();
						collider.add(uf::BoundingBox( uf::vector::add({0, 1.5, 0}, transform.position), {0.7, 1.6, 0.7} ));
					}
					testColliders( entityA.getComponent<uf::Collider>(), entityB.getComponent<uf::Collider>(), &entityA, &entityB, useStrongest );
				}
			}
		
			return 0;
		};
		if ( local ) function(); else uf::thread::add( thread, function, true );
	}
#endif
}
void ext::Craeture::render() {
	uf::Object::render();
}