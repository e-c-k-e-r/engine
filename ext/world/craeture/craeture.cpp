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

#include "..//battle.h"

#include "../terrain/generator.h"
#include "../world.h"
#include "../../asset/asset.h"

void ext::Craeture::initialize() {	
	ext::Object::initialize();

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
	if ( metadata["animation"] != Json::nullValue ) {
		for( auto it = metadata["animation"]["names"].begin() ; it != metadata["animation"]["names"].end() ; ++it ) { 
			const std::string& name = it->asString();
			if ( this->m_animation.transforms.find(name) == this->m_animation.transforms.end() ) this->m_animation.transforms[name];
		}
	}
	/* Gravity */ {
		if ( metadata["collision"]["gravity"] != Json::nullValue ) {
			physics.linear.acceleration.x = metadata["collision"]["gravity"][0].asFloat();
			physics.linear.acceleration.y = metadata["collision"]["gravity"][1].asFloat();
			physics.linear.acceleration.z = metadata["collision"]["gravity"][2].asFloat();
		}
	}
	/* Collider */ {
		uf::CollisionBody& collider = this->getComponent<uf::CollisionBody>();
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
	this->addHook( "asset:Cache.Sound.%UID%", [&](const std::string& event)->std::string{	
		uf::Serializer json = event;
		
		ext::World& world = this->getRootParent<ext::World>();
		uf::Serializer& masterdata = world.getComponent<uf::Serializer>();
		
		std::string filename = json["filename"].asString();

		if ( uf::string::extension(filename) != "ogg" ) return "false";

		if ( filename == "" ) return "false";
		uf::Audio& sfx = this->getComponent<uf::SoundEmitter>().add(filename);
		sfx.setVolume(masterdata["volumes"]["sfx"].asFloat());
		auto& pTransform = world.getPlayer().getComponent<pod::Transform<>>();
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
			ext::World& world = this->getRootParent<ext::World>();
			ext::Asset& assetLoader = world.getComponent<ext::Asset>();
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
	ext::Object::tick();

	uf::Serializer& metadata = this->getComponent<uf::Serializer>();
	ext::World& world = this->getRootParent<ext::World>();
	uf::Serializer& pMetadata = world.getPlayer().getComponent<uf::Serializer>();

	if ( !pMetadata["system"]["control"].asBool() ) return;

	pod::Transform<>& transform = this->getComponent<pod::Transform<>>();
	pod::Physics& physics = this->getComponent<pod::Physics>();
	transform = uf::physics::update( transform, physics );

	bool local = false;
	bool sort = false;
//	pod::Thread& thread = uf::thread::fetchWorker();
	pod::Thread& thread = uf::thread::has("Physics") ? uf::thread::get("Physics") : uf::thread::create( "Physics", true, false );

	/* Collision against world */ {
		auto function = [&]() -> int {
			if ( this->hasParent() && metadata["collision"]["should"].asBool() ) {
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

				uf::CollisionBody pCollider;
				std::vector<pod::Vector3ui> positions = {
					{ voxelPosition.x, voxelPosition.y, voxelPosition.z },
					{ voxelPosition.x - 1, voxelPosition.y, voxelPosition.z },
					{ voxelPosition.x + 1, voxelPosition.y, voxelPosition.z },
					{ voxelPosition.x, voxelPosition.y, voxelPosition.z - 1 },
					{ voxelPosition.x, voxelPosition.y, voxelPosition.z + 1},
				};

				if ( this->m_name == "Housamo" ) {
					// bottom
					uint16_t uid = generator.getVoxel( voxelPosition.x, voxelPosition.y, voxelPosition.z );
					if ( uid == ext::TerrainVoxelLava().uid() ) {
						this->callHook("world:Craeture.Hurt.%UID%");
					}
				}

				if ( false ) {
					// top
					if ( ext::TerrainVoxel::atlas( generator.getVoxel( voxelPosition.x, voxelPosition.y + 1, voxelPosition.z ) ).solid() && physics.linear.velocity.y != 0 ) {
						transform.position.y += physics.linear.velocity.y * uf::physics::time::delta;
						physics.linear.velocity.y = 0;
					}
					// bottom
					if ( ext::TerrainVoxel::atlas( generator.getVoxel( voxelPosition.x, voxelPosition.y - 1, voxelPosition.z ) ).solid() && physics.linear.velocity.y != 0 ) {
						transform.position.y -= physics.linear.velocity.y * uf::physics::time::delta;
						physics.linear.velocity.y = 0;
					}
				} else {
					positions.push_back( { voxelPosition.x, voxelPosition.y - 1, voxelPosition.z } );
					positions.push_back( { voxelPosition.x, voxelPosition.y + 1, voxelPosition.z } );
				}

				for ( auto& position : positions ) {
					ext::TerrainVoxel voxel = ext::TerrainVoxel::atlas( generator.getVoxel( position.x, position.y, position.z ) );
					pod::Vector3 offset = rTransform.position;
					offset.x += position.x - (size.x / 2.0f);
					offset.y += position.y - (size.y / 2.0f);
					offset.z += position.z - (size.z / 2.0f);

					if ( !voxel.solid() ) continue;

					uf::Collider* box = new uf::AABBox( offset, {0.5, 0.5, 0.5} );
					pCollider.add(box);
				}

				uf::CollisionBody& collider = this->getComponent<uf::CollisionBody>();
				pod::Transform<>& transform = this->getComponent<pod::Transform<>>(); {
					collider.clear();
					uf::Collider* box = new uf::AABBox( uf::vector::add({0, 1.5, 0}, transform.position), {0.7, 1.6, 0.7} );
					collider.add(box);
				}

				pod::Physics& physics = this->getComponent<pod::Physics>();
				auto result = pCollider.intersects(collider);
				uf::Collider::Manifold strongest;
				strongest.depth = 0.001;
				bool useStrongest = true;
				for ( auto manifold : result ) {
					if ( manifold.colliding && manifold.depth > 0 ) {
						if ( strongest.depth < manifold.depth ) strongest = manifold;
						if ( !useStrongest ) {
							pod::Vector3 correction = uf::vector::normalize(manifold.normal) * -(manifold.depth * manifold.depth * 1.001);
							transform.position += correction;
							if ( manifold.normal.x == 1 || manifold.normal.x == -1 ) physics.linear.velocity.x = 0;
							if ( manifold.normal.y == 1 || manifold.normal.y == -1 ) physics.linear.velocity.y = 0;
							if ( manifold.normal.z == 1 || manifold.normal.z == -1 ) physics.linear.velocity.z = 0;
						}
					}
				}
				if ( useStrongest && strongest.colliding ) {
					pod::Vector3 correction = uf::vector::normalize(strongest.normal) * -(strongest.depth * strongest.depth * 1.001);
					transform.position += correction;

				//	std::cout << "Collision! " << ( strongest.colliding ? "yes" : "no" ) << " " << strongest.normal.x << ", " << strongest.normal.y << ", " << strongest.normal.z << " / " << strongest.depth << std::endl;
					if ( strongest.normal.x == 1 || strongest.normal.x == -1 ) physics.linear.velocity.x = 0;
					if ( strongest.normal.y == 1 || strongest.normal.y == -1 ) physics.linear.velocity.y = 0;
					if ( strongest.normal.z == 1 || strongest.normal.z == -1 ) physics.linear.velocity.z = 0;
				}
			}
			return 0;
		};
		if ( local ) function(); else uf::thread::add( thread, function, true );
	}

	/* Collision against world */ if ( false ) {
		auto function = [&]() -> int {
			if ( this->hasParent() && metadata["collision"]["should"].asBool() ) {
				if ( !this->hasParent() ) { return 0; }
				if ( this->getName() == "Player" ) { return 0; }
				if ( this->getParent().getName() != "Region" ) { return 0; }
				if ( !this->getParent().hasParent() ) { return 0; }
				if ( this->getParent().getParent().getName() != "Terrain" ) { return 0; }

				uf::Entity& parentRegion = this->getParent();
				uf::Entity& parentTerrain = parentRegion.getParent();

				int regions = 0;
				std::vector<uf::Entity*> entities;
				entities.push_back(&parentRegion);
				if ( false ) {
					/* Retrieve close entities */ for ( uf::Entity* kv : parentTerrain.getChildren() ) { if ( !kv ) continue; if ( kv->getName() != "Region" ) continue;
						if ( kv->getUid() == parentRegion.getUid() ) continue;
						uf::Serializer& rMetadata = kv->getComponent<uf::Serializer>();
						if ( !rMetadata["region"]["initialized"].asBool() ) continue;
						if ( ++regions <= 2 ) entities.push_back(kv);
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
				}

				for ( uf::Entity* e : entities ) {
					uf::CollisionBody& collider = this->getComponent<uf::CollisionBody>();
					uf::CollisionBody& pCollider = e->getComponent<uf::CollisionBody>();

					pod::Transform<>& transform = this->getComponent<pod::Transform<>>(); {
						collider.clear();
						uf::Collider* box = new uf::AABBox( uf::vector::add({0, 1.5, 0}, transform.position), {0.7, 1.6, 0.7} );
						collider.add(box);
					}

					if ( !e ) continue;
					if ( e->getUid() == 0 ) continue;
					if ( this->getUid() == 0 ) return 0;
					if ( !this->hasComponent<pod::Physics>() ) return 0;

					pod::Physics& physics = this->getComponent<pod::Physics>();
					auto result = pCollider.intersects(collider);
					uf::Collider::Manifold strongest;
					strongest.depth = 0.001;
					bool useStrongest = true;
					for ( auto manifold : result ) {
						if ( manifold.colliding && manifold.depth > 0 ) {
							if ( strongest.depth < manifold.depth ) strongest = manifold;
							if ( !useStrongest ) {
								pod::Vector3 correction = uf::vector::normalize(manifold.normal) * -(manifold.depth * manifold.depth * 1.001);
								transform.position += correction;
								if ( manifold.normal.x == 1 || manifold.normal.x == -1 ) physics.linear.velocity.x = 0;
								if ( manifold.normal.y == 1 || manifold.normal.y == -1 ) physics.linear.velocity.y = 0;
								if ( manifold.normal.z == 1 || manifold.normal.z == -1 ) physics.linear.velocity.z = 0;
							}
						}
					}
					if ( useStrongest && strongest.colliding ) {
						pod::Vector3 correction = uf::vector::normalize(strongest.normal) * -(strongest.depth * strongest.depth * 1.001);
						transform.position += correction;

					//	std::cout << "Collision! " << ( strongest.colliding ? "yes" : "no" ) << " " << strongest.normal.x << ", " << strongest.normal.y << ", " << strongest.normal.z << " / " << strongest.depth << std::endl;
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

	/* Collision against other craetures */ {
		auto function = [&]() -> int {
			/* Collision */ if ( this->hasParent() && metadata["collision"]["should"].asBool() ) {
				if ( !this->hasParent() ) { return 0; }
				if ( this->getParent().getName() != "Region" ) { return 0; }

				uf::Entity& parent = this->getParent();

				int regions = 0;
				std::vector<uf::Entity*> entities;
				/* Retrieve close entities */ for ( uf::Entity* kv : parent.getChildren() ) {
					if ( !kv ) continue;
					if ( kv->getUid() == this->getUid() ) continue;
					if ( !kv->hasComponent<uf::CollisionBody>() ) continue;
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

					uf::CollisionBody& collider = this->getComponent<uf::CollisionBody>();
					uf::CollisionBody& pCollider = e->getComponent<uf::CollisionBody>();

					pod::Transform<>& transform = this->getComponent<pod::Transform<>>(); {
						collider.clear();
						uf::Collider* box = new uf::AABBox( uf::vector::add({0, 1.5, 0}, transform.position), {0.7, 1.6, 0.7} );
						collider.add(box);
					}

					pod::Physics& physics = this->getComponent<pod::Physics>();
					auto result = pCollider.intersects(collider);
					uf::Collider::Manifold strongest;
					strongest.depth = 0.001;
					bool useStrongest = true;
					for ( auto manifold : result ) {
						if ( manifold.colliding && manifold.depth > 0 ) {
							if ( strongest.depth < manifold.depth ) strongest = manifold;
							if ( !useStrongest ) {
								pod::Vector3 correction = uf::vector::normalize(manifold.normal) * -(manifold.depth * manifold.depth * 1.001);
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
						pod::Vector3 correction = uf::vector::normalize(strongest.normal) * -(strongest.depth * strongest.depth * 1.001);
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

						transform.position += correction;

					//	std::cout << "Collision! " << ( strongest.colliding ? "yes" : "no" ) << " " << strongest.normal.x << ", " << strongest.normal.y << ", " << strongest.normal.z << " / " << strongest.depth << std::endl;

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
}
void ext::Craeture::render() {
	ext::Object::render();
}