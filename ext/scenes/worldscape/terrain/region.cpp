#include "region.h"

#include "../../../ext.h"
#include "generator.h"

#include <uf/engine/asset/asset.h>
#include <uf/utils/camera/camera.h>
#include <uf/utils/thread/thread.h>
#include <uf/utils/math/collision.h>
#include <uf/utils/graphic/graphic.h>
#include <uf/utils/graphic/mesh.h>
#include <uf/utils/string/ext.h>
#include <mutex>

UF_BEHAVIOR_REGISTER_CPP(ext::RegionBehavior)
UF_BEHAVIOR_REGISTER_AS_OBJECT(ext::RegionBehavior, ext::Region)
#define this ((uf::Object*) &self)
void ext::RegionBehavior::initialize( uf::Object& self ) {
	// alias Mesh types
	{
		auto& mutexPointer = this->getComponent<std::mutex*>();
		mutexPointer = new std::mutex;
	}

	uf::Serializer& metadata = this->getComponent<uf::Serializer>();
	metadata["region"]["initialized"] = true;

	if ( !metadata["terrain"]["unified"].as<bool>() ) {
		std::string textureFilename = ""; {
			uf::Serializer& metadata = this->getParent().getComponent<uf::Serializer>();
			uf::Asset assetLoader;
			for ( uint i = 0; i < metadata["system"]["assets"].size(); ++i ) {
				if ( textureFilename != "" ) break;
				std::string filename = this->grabURI( metadata["system"]["assets"][i].as<std::string>(), metadata["system"]["root"].as<std::string>() );
				textureFilename = assetLoader.cache( filename );
			}
		}
		ext::TerrainGenerator::mesh_t& mesh = this->getComponent<ext::TerrainGenerator::mesh_t>();
		auto& graphic = this->getComponent<uf::Graphic>();

		graphic.initialize();
		graphic.process = false;

		auto& texture = graphic.material.textures.emplace_back();
		texture.sampler.descriptor.filter.min = VK_FILTER_NEAREST;
		texture.sampler.descriptor.filter.mag = VK_FILTER_NEAREST;
		texture.loadFromFile( textureFilename );

		std::string suffix = ""; {
			std::string _ = this->getRootParent<uf::Scene>().getComponent<uf::Serializer>()["shaders"]["region"]["suffix"].as<std::string>();
			if ( _ != "" ) suffix = _ + ".";
		}
	/*
		graphic.material.attachShader("./data/shaders/terrain.stereo.vert.spv", VK_SHADER_STAGE_VERTEX_BIT);
		graphic.material.attachShader("./data/shaders/terrain."+suffix+"frag.spv", VK_SHADER_STAGE_FRAGMENT_BIT);
	*/
		graphic.material.initializeShaders({
			{"./data/shaders/terrain.stereo.vert.spv", VK_SHADER_STAGE_VERTEX_BIT},
			{"./data/shaders/terrain."+suffix+"frag.spv", VK_SHADER_STAGE_FRAGMENT_BIT}
		});
	}

	this->addHook( "region:Generate.%UID%", [&](const std::string& event)->std::string{	
		uf::Serializer json = event;

		uf::Serializer& metadata = this->getComponent<uf::Serializer>();
		if ( !metadata["region"]["initialized"].as<bool>() ) return "false";
		if ( metadata["region"]["generated"].as<bool>() ) return "false";

		pod::Vector3ui size; {
			size.x = metadata["region"]["size"][0].as<size_t>();
			size.y = metadata["region"]["size"][1].as<size_t>();
			size.z = metadata["region"]["size"][2].as<size_t>();
		}
		uint subdivisions = metadata["region"]["subdivisions"].as<size_t>();
		ext::TerrainGenerator& generator = this->getComponent<ext::TerrainGenerator>();
		generator.initialize(size, subdivisions);
		generator.generate(*this);
		generator.updateLight();

		/* Collider */ if ( false ) {
			pod::Transform<>& transform = this->getComponent<pod::Transform<>>();
			uf::Collider& collider = this->getComponent<uf::Collider>();
		
			std::size_t i = 0;
			const auto& voxels = generator.getVoxels();
			for ( auto& _ : voxels ) {
				for ( std::size_t __ = 0; __ < _.length; ++__ ) {
					{
						std::size_t x = 0, y = 0, z = 0;
						{
							auto v = generator.unwrapIndex( i++ );
							x = v.x;
							y = v.y;
							z = v.z;
						}

						ext::TerrainVoxel voxel = ext::TerrainVoxel::atlas( _.value );
						pod::Vector3 offset = transform.position;
						offset.x += x - (size.x / 2.0f);
						offset.y += y - (size.y / 2.0f);
						offset.z += z - (size.z / 2.0f);

						if ( !voxel.solid() ) continue;

						collider.add(new uf::BoundingBox( offset, {0.5, 0.5, 0.5} ));
					}
				}
			}
		}
		metadata["region"]["generated"] = true;
		return "true";
	});
	this->addHook( "region:Rasterize.%UID%", [&](const std::string& event)->std::string{	
		uf::Serializer json = event;

		ext::TerrainGenerator& generator = this->getComponent<ext::TerrainGenerator>();
		ext::TerrainGenerator::mesh_t& mesh = this->getComponent<ext::TerrainGenerator::mesh_t>();
		auto& graphic = this->getComponent<uf::Graphic>();

		generator.rasterize(mesh.vertices, *this);
		graphic.initializeGeometry( mesh );

		this->queueHook("region:Finalize.%UID%", "");
		this->queueHook("region:Populate.%UID%", "");
		return "true";
	});
	this->addHook( "region:Finalize.%UID%", [&](const std::string& event)->std::string{	
		uf::Serializer json = event;

		if ( this->hasComponent<uf::Graphic>() ) {
			auto& graphic = this->getComponent<uf::Graphic>();
			graphic.process = true;
		}
		metadata["region"]["rasterized"] = true;

		return "true";
	});
	this->addHook( "region:Populate.%UID%", [&](const std::string& event)->std::string{	
		uf::Serializer json = event;

		if ( metadata["region"][""]["initialized"].as<bool>() ) return "false";

		metadata["region"][""]["initialized"] = true;
		bool first = false;
		pod::Transform<>& transform = this->getComponent<pod::Transform<>>();
		float r = (rand() % 100) / 100.0;
		pod::Vector3ui size = {
			metadata["region"]["size"][0].as<size_t>() / 4,
			metadata["region"]["size"][1].as<size_t>() / 4,
			metadata["region"]["size"][2].as<size_t>() / 4,
		};
		uint half_x = size.x / 2;
		uint half_y = size.y / 2;
		uint half_z = size.z / 2;
		double noise[size.x][size.y][size.z];
		double maxValue = 0.0;
		double sum = 0.0;
		{
			for ( int z = 0; z < size.z; ++z ) {
			for ( int y = 0; y < size.y; ++y ) {
			for ( int x = 0; x < size.x; ++x ) {
				pod::Vector3d position = {
					transform.position.x - half_x, transform.position.y - half_y, transform.position.z - half_z
				};
				position.x += (double) x / (double) size.x;
				position.y += (double) y / (double) size.y;
				position.z += (double) z / (double) size.z;
				noise[x][y][z] = ext::TerrainGenerator::noise.sample( position );
				maxValue = std::max( maxValue, noise[x][y][z] );
			}
			}
			}
			for ( int z = 0; z < size.z; ++z ) {
			for ( int y = 0; y < size.y; ++y ) {
			for ( int x = 0; x < size.x; ++x ) {
				sum += noise[x][y][z] / maxValue;
			}
			}
			}
			sum /= size.x * size.y * size.z;
		}
		// add lights
		{
			bool should = metadata["region"]["lights"]["should"].as<bool>();
			if ( r > metadata["region"]["lights"]["rate"].as<float>() ) should = false;
			// std::cout << "Rate: " << r << ": " << transform.position.x << ", " << transform.position.y << ", " << transform.position.z << std::endl;
			if (
				metadata["region"]["location"][0] == 0 &&
				metadata["region"]["location"][1] == 0 &&
				metadata["region"]["location"][2] == 0
			) {
				should = true;
			}
			if ( should ) {
			//	uf::Entity* light = this->findByUid(this->loadChildUid("./light.json", true));
				uf::Object& light = this->loadChild("./light.json", true);
				pod::Vector3f origin = transform.position + light.getComponent<pod::Transform<>>().position;
				pod::Vector3f color = {
					(rand() % 100) / 100.0,
					(rand() % 100) / 100.0,
					(rand() % 100) / 100.0,
				};
				std::function<void(uf::Entity*, int)> filter = [&]( uf::Entity* entity, int indent ) {
					auto& metadata = entity->getComponent<uf::Serializer>();
					auto& transform = entity->getComponent<pod::Transform<>>();
					transform.position = origin;
				//	if ( !ext::json::isArray( metadata["light"]["color"] ) ) {
						metadata["light"]["color"][0] = color.x;
						metadata["light"]["color"][1] = color.y;
						metadata["light"]["color"][2] = color.z;
				//	}
				};
				light.process(filter);
			/*
				uf::Serializer& metadata = light.getComponent<uf::Serializer>();
				pod::Transform<>& lTransform = light.getComponent<pod::Transform<>>();
				lTransform.position += transform.position;
				if ( !ext::json::isArray( metadata["light"]["color"] ) ) {
					metadata["light"]["color"][0] = (rand() % 100) / 100.0;
					metadata["light"]["color"][1] = (rand() % 100) / 100.0;
					metadata["light"]["color"][2] = (rand() % 100) / 100.0;
				}
			*/
			}
		}
		// add mobs
		{
			bool should = metadata["region"][""]["should"].as<bool>();
			if ( r > metadata["region"][""]["rate"].as<float>() ) should = false;
			// std::cout << "Rate: " << r << ": " << transform.position.x << ", " << transform.position.y << ", " << transform.position.z << std::endl;
			if (
				metadata["region"]["location"][0] == 0 &&
				metadata["region"]["location"][1] == 0 &&
				metadata["region"]["location"][2] == 0
			) {
				first = true;
				should = true;
			}
			if ( !should ) return "false";

			if ( first ) {
				// shiro
				if ( metadata["region"][""]["NPCs"].as<bool>() ) {
				//	uf::Object*  = (uf::Object*) this->findByUid(this->loadChildUid("./shiro.json", true));
					uf::Object&  = this->loadChild("./shiro.json", true);
					pod::Transform<>& pTransform = .getComponent<pod::Transform<>>();
					pTransform.position += transform.position + pod::Vector3f{ 2, 0, 0 };
				}
				// pong
				if ( metadata["region"][""]["NPCs"].as<bool>() ) {
				//	uf::Object*  = (uf::Object*) this->findByUid(this->loadChildUid("./pongy.json", true));
					uf::Object&  = this->loadChild("./pongy.json", true);
					uf::Serializer& pMetadata = .getComponent<uf::Serializer>();

					pod::Transform<>& pTransform = .getComponent<pod::Transform<>>();
					pTransform.position += transform.position + pod::Vector3f{ -2, 0, 0 };
				}
				return "true";
			}

			for ( uint i = 0; i < metadata["region"][""]["amount"].as<size_t>(); ++i ) {
			//	uf::Object*  = (uf::Object*) this->findByUid(this->loadChildUid("./.json", false));
				uf::Object&  = this->loadChild("./.json", false);
				// set name
				uf::Serializer& pMetadata = .getComponent<uf::Serializer>();
			//	int ri = floor((noise[i][i][i] * metadata["region"][""]["list"].size());
				int ri = floor(r * metadata["region"][""]["list"].size());
				pMetadata[""] = metadata["region"][""]["list"][ri];
				pMetadata["music"] = metadata["region"][""]["music"];
			//	pMetadata["hostile"] = true;
				.initialize();

				pod::Transform<>& pTransform = .getComponent<pod::Transform<>>();
				float rx = first ? 0.3 : (rand() % 100) / 100.0;
				float rz = first ? 0.7 : (rand() % 100) / 100.0;
				float spread = 32.0f;
				spread = std::min( metadata["region"]["size"][0].as<float>(), spread );
				spread = std::min( metadata["region"]["size"][1].as<float>(), spread );
				spread = std::min( metadata["region"]["size"][2].as<float>(), spread );
				pod::Vector3f randomOffset = {
					rx * spread - spread/2.0f,
					0,
					rz * spread - spread/2.0f,
				};
				pTransform.position += transform.position + randomOffset;
				pTransform.orientation = uf::quaternion::axisAngle( { 0.0f, 1.0f, 0.0f }, r * 2 * 3.1415926f );
			}
		}
		return "true";
	});
}
void ext::RegionBehavior::tick( uf::Object& self ) {
	// do collision on children
#if 0
	auto& mutex = *(this->getComponent<std::mutex*>());
	mutex.lock();

	auto& scene = uf::scene::getCurrentScene();
	auto& sMetadata = scene.getComponent<uf::Serializer>();
	auto& metadata = this->getComponent<uf::Serializer>();
	bool threaded = !sMetadata["system"]["physics"]["single threaded"].as<bool>();
	bool sort = sMetadata["system"]["physics"]["sort"].as<bool>();
	bool useStrongest = sMetadata["system"]["physics"]["use"]["strongest"].as<bool>();
	bool queued = sMetadata["system"]["physics"]["use"]["queue"].as<bool>();
	bool updatePhysics = !sMetadata["system"]["physics"]["entity-local update"].as<bool>();
	bool useWorkers = sMetadata["system"]["physics"]["use"]["worker"].as<bool>();
	pod::Thread& thread = uf::thread::has("Physics") ? uf::thread::get("Physics") : uf::thread::create( "Physics", true, false );

	auto& generator = this->getComponent<ext::TerrainGenerator>();
	auto& regionPosition = this->getComponent<pod::Transform<>>().position;
	pod::Vector3f size; {
		size.x = metadata["region"]["size"][0].as<size_t>();
		size.y = metadata["region"]["size"][1].as<size_t>();
		size.z = metadata["region"]["size"][2].as<size_t>();
	}

	auto function = [&]() -> int {
		std::vector<uf::Object*> entities;
		// update physics
		if ( updatePhysics ) {
			std::function<void(uf::Entity*)> filter = [&]( uf::Entity* entity ) {
				if ( !entity->hasComponent<pod::Physics>() ) return;
				auto& metadata = entity->getComponent<uf::Serializer>();
				auto& transform = entity->getComponent<pod::Transform<>>();
				auto& physics = entity->getComponent<pod::Physics>();
				if ( metadata["system"]["physics"]["gravity"] != Json::nullValue ) {
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
			};
			this->process(filter);
		}
		{
			std::function<void(uf::Entity*)> filter = [&]( uf::Entity* entity ) {
				auto& metadata = entity->getComponent<uf::Serializer>();
				if ( !ext::json::isNull( metadata["system"]["physics"]["collision"] ) && !metadata["system"]["physics"]["collision"].as<bool>() ) return;
				if ( entity->hasComponent<uf::Collider>() )
					entities.push_back((uf::Object*) entity);
			};
			this->process(filter);
		}
		auto onCollision = []( pod::Collider::Manifold& manifold, uf::Object* a, uf::Object* b, bool queued ){				
			uf::Serializer payload;
			payload["normal"][0] = manifold.normal.x;
			payload["normal"][1] = manifold.normal.y;
			payload["normal"][2] = manifold.normal.z;
			payload["entity"] = b->getUid();
			payload["depth"] = -manifold.depth;
			if ( queued ) a->queueHook("world:Collision.%UID%", payload);
			else a->callHook("world:Collision.%UID%", payload);
			
			payload["entity"] = a->getUid();
			payload["depth"] = manifold.depth;
			if ( queued ) b->queueHook("world:Collision.%UID%", payload);
			else b->callHook("world:Collision.%UID%", payload);
		};
		auto testColliders = [&]( uf::Collider& colliderA, uf::Collider& colliderB, uf::Object* a, uf::Object* b, bool useStrongest ){
			pod::Collider::Manifold strongest;
			auto manifolds = colliderA.intersects(colliderB);
			for ( auto manifold : manifolds ) {
				if ( manifold.colliding && manifold.depth > 0 ) {
					if ( !useStrongest ) onCollision(manifold, a, b, queued);
					else if ( strongest.depth < manifold.depth ) strongest = manifold;
				}
			}
			if ( useStrongest && strongest.colliding ) onCollision(strongest, a, b, queued);
		};
		// collide with world
		for ( auto* _ : entities ) {
			uf::Object& entity = *_;
			auto& entityCollider = entity.getComponent<uf::Collider>();
			auto function = [&]() -> int {
			auto& entityTransform = entity.getComponent<pod::Transform<>>();
			pod::Vector3f voxelPosition = entityTransform.position - regionPosition;
			voxelPosition.x += size.x / 2.0f;
			voxelPosition.y += size.y / 2.0f + 1;
			voxelPosition.z += size.z / 2.0f;

			std::vector<pod::Vector3ui> positions = {
				{ voxelPosition.x, voxelPosition.y, voxelPosition.z },
				{ voxelPosition.x - 1, voxelPosition.y, voxelPosition.z },
				{ voxelPosition.x + 1, voxelPosition.y, voxelPosition.z },
				{ voxelPosition.x, voxelPosition.y - 1, voxelPosition.z },
				{ voxelPosition.x, voxelPosition.y + 1, voxelPosition.z },
				{ voxelPosition.x, voxelPosition.y, voxelPosition.z - 1 },
				{ voxelPosition.x, voxelPosition.y, voxelPosition.z + 1},
			};
			
			uf::Collider collider;
			for ( auto& position : positions ) {
				ext::TerrainVoxel voxel = ext::TerrainVoxel::atlas( generator.getVoxel( position.x, position.y, position.z ) );
				pod::Vector3 offset = regionPosition;
				offset.x += position.x - (size.x / 2.0f);
				offset.y += position.y - (size.y / 2.0f);
				offset.z += position.z - (size.z / 2.0f);

				if ( !voxel.solid() ) continue;

				collider.add( new uf::BoundingBox( offset, {0.5, 0.5, 0.5} ) );
			#if 0
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
			#endif
			}
				testColliders( collider, entityCollider, this, &entity, useStrongest );
				return 0;
			};
			// if ( threaded && useWorkers ) uf::thread::add( uf::thread::fetchWorker(), function, true ); else
				function();
		}
	
		// collide with others
		for ( auto* _a : entities ) {
			uf::Object& entityA = *_a;
			for ( auto* _b : entities ) { if ( _a == _b ) continue;
				uf::Object& entityB = *_b;
				auto& colliderA = entityA.getComponent<uf::Collider>();
				auto& colliderB = entityB.getComponent<uf::Collider>();
				auto function = [&]() -> int {
					testColliders( colliderA, colliderB, &entityA, &entityB, useStrongest );
					return 0;
				};
				if ( threaded && useWorkers ) uf::thread::add( uf::thread::fetchWorker(), function, true ); else function();
			}
		}
		mutex.unlock();
		return 0;
	};
	if ( threaded ) uf::thread::add( thread, function, true ); else function();
#else
#if 1
	{

		auto& metadata = this->getComponent<uf::Serializer>();

		auto& scene = uf::scene::getCurrentScene();
		auto& sMetadata = scene.getComponent<uf::Serializer>();

		if ( !sMetadata["system"]["physics"]["collision"].as<bool>() ) return;
		auto& mutex = *(this->getComponent<std::mutex*>());
		mutex.lock();

		bool threaded = !sMetadata["system"]["physics"]["single threaded"].as<bool>();
		bool sort = sMetadata["system"]["physics"]["sort"].as<bool>();
		bool useStrongest = sMetadata["system"]["physics"]["use"]["strongest"].as<bool>();
		bool queued = sMetadata["system"]["physics"]["use"]["queue"].as<bool>();
		bool ignoreStaticEntities = sMetadata["system"]["physics"]["optimizations"]["ignore static entities"].as<bool>();
		bool ignoreDuplicateTests = sMetadata["system"]["physics"]["optimizations"]["ignore duplicate tests"].as<bool>();
		bool updatePhysics = !sMetadata["system"]["physics"]["optimizations"]["entity-local update"].as<bool>();
		bool useWorkers = sMetadata["system"]["physics"]["use"]["worker"].as<bool>();
		pod::Thread& thread = uf::thread::has("Physics") ? uf::thread::get("Physics") : uf::thread::create( "Physics", true, false );
		auto function = [&]() -> int {
			std::vector<uf::Object*> entities;
			std::function<void(uf::Entity*)> filter = [&]( uf::Entity* entity ) {
				auto& metadata = entity->getComponent<uf::Serializer>();
				if ( !ext::json::isNull( metadata["system"]["physics"]["collision"] ) && !metadata["system"]["physics"]["collision"].as<bool>() ) return;
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

			// collide with world
			auto& metadata = this->getComponent<uf::Serializer>();
			auto& generator = this->getComponent<ext::TerrainGenerator>();
			auto& regionPosition = this->getComponent<pod::Transform<>>().position;
			pod::Vector3f size; {
				size.x = metadata["region"]["size"][0].as<size_t>();
				size.y = metadata["region"]["size"][1].as<size_t>();
				size.z = metadata["region"]["size"][2].as<size_t>();
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
						testColliders( colliderA, colliderB, entityA, entityB, useStrongest );
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
							testColliders( colliderA, colliderB, &entityA, &entityB, useStrongest );
							return 0;
						};
						if ( threaded && useWorkers ) uf::thread::add( uf::thread::fetchWorker(), function, true ); else function();
					}
				}
			}
		/*
			for ( auto* _a : entities ) {
				uf::Object& entityA = *_a;
				for ( auto* _b : entities ) { if ( _a == _b ) continue;
					uf::Object& entityB = *_b;
					testColliders( entityA.getComponent<uf::Collider>(), entityB.getComponent<uf::Collider>(), &entityA, &entityB, useStrongest );
				}
			}
		*/
			mutex.unlock();
			return 0;
		};
		if ( threaded ) function(); else uf::thread::add( thread, function, true );
	}
#endif
#endif
}
void ext::RegionBehavior::render( uf::Object& self ){
	uf::Scene& scene = uf::scene::getCurrentScene();
	uf::Serializer& metadata = this->getComponent<uf::Serializer>();
	
	if ( !metadata["region"]["rasterized"].as<bool>() ) return;
	/* Update uniforms */ if ( this->hasComponent<uf::Graphic>() ) {
		auto& scene = uf::scene::getCurrentScene();
		auto& mesh = this->getComponent<ext::TerrainGenerator::mesh_t>();
		auto& graphic = this->getComponent<uf::Graphic>();
		auto& camera = scene.getController().getComponent<uf::Camera>();		
		if ( !graphic.initialized ) return;
		auto& uniforms = graphic.material.shaders.front().uniforms.front().get<uf::StereoMeshDescriptor>();
		uniforms.matrices.model = uf::matrix::identity();
		for ( std::size_t i = 0; i < 2; ++i ) {
			uniforms.matrices.view[i] = camera.getView( i );
			uniforms.matrices.projection[i] = camera.getProjection( i );
		}
		graphic.material.shaders.front().updateBuffer( uniforms, 0, true );
	}
}
void ext::RegionBehavior::destroy( uf::Object& self ){
	{
		auto& mutexPointer = this->getComponent<std::mutex*>();
		delete mutexPointer;
		mutexPointer = NULL;
	}
}
#undef this