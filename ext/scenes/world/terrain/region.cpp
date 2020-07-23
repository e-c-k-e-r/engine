#include "region.h"

#include "../world.h"
#include "../../../ext.h"
#include "generator.h"
#include "..//sprite.h"
#include <uf/engine/asset/asset.h>
#include <uf/utils/camera/camera.h>
#include <uf/utils/math/collision.h>

void ext::Region::initialize() {
	uf::Object::initialize();

	// alias Mesh types
	{
	//	this->addAlias<ext::TerrainGenerator::mesh_t, uf::MeshBase>();
		this->addAlias<ext::TerrainGenerator::mesh_t, uf::Mesh>();
	}

	this->addComponent<pod::Transform<>>();
	this->addComponent<ext::TerrainGenerator>();

	this->m_name = "Region";

	this->load();

	uf::Serializer& metadata = this->getComponent<uf::Serializer>();
	this->addHook( "region:Reload.%UID%", [&](const std::string& event)->std::string{	
		uf::Serializer json = event;

		if ( !this->hasComponent<ext::TerrainGenerator::mesh_t>() ) return "false";
		ext::TerrainGenerator::mesh_t& mesh = this->getComponent<ext::TerrainGenerator::mesh_t>();
		if ( mesh.vertices.empty() ) {
			mesh.destroy();
			this->deleteComponent<ext::TerrainGenerator::mesh_t>();
			return "false";
		}
		mesh.initialize(true);
		mesh.graphic.bindUniform<uf::StereoMeshDescriptor>();

		std::string suffix = ""; {
			std::string _ = this->getRootParent<uf::Scene>().getComponent<uf::Serializer>()["shaders"]["region"]["suffix"].asString();
			if ( _ != "" ) suffix = _ + ".";
		}
		mesh.graphic.initializeShaders({
			{"./data/shaders/terrain.stereo.vert.spv", VK_SHADER_STAGE_VERTEX_BIT},
			{"./data/shaders/terrain."+suffix+"frag.spv", VK_SHADER_STAGE_FRAGMENT_BIT}
		});
		std::string texture = ""; {
			uf::Serializer& metadata = this->getParent().getComponent<uf::Serializer>();
			uf::Asset assetLoader;
			for ( uint i = 0; i < metadata["_config"]["assets"].size(); ++i ) {
				if ( texture != "" ) break;
				std::string filename = metadata["_config"]["assets"][i].asString();
				texture = assetLoader.cache( filename );
			}
		}
		mesh.graphic.texture.loadFromFile( texture );
		mesh.graphic.initialize();

		this->queueHook("region:Generated.%UID%", "", 0.5);

		return "true";
	});
	this->addHook( "region:Generated.%UID%", [&](const std::string& event)->std::string{	
		ext::TerrainGenerator::mesh_t& mesh = this->getComponent<ext::TerrainGenerator::mesh_t>();
		mesh.graphic.autoAssign();
		
		metadata["region"]["generated"] = true;
		this->queueHook("region:Populate.%UID%", "", 1);
		return "true";
	});
	this->addHook( "region:Populate.%UID%", [&](const std::string& event)->std::string{	
		if ( metadata["region"][""]["initialized"].asBool() ) return "false";
		metadata["region"][""]["initialized"] = true;
		bool should = metadata["region"][""]["should"].asBool();
		bool first = false;
		pod::Transform<>& transform = this->getComponent<pod::Transform<>>();
		float r = (rand() % 100) / 100.0;
		pod::Vector3ui size = {
			metadata["region"]["size"][0].asUInt64() / 4,
			metadata["region"]["size"][1].asUInt64() / 4,
			metadata["region"]["size"][2].asUInt64() / 4,
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
		if ( r > metadata["region"][""]["rate"].asFloat() ) should = false;
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
			{
				uf::Object*  = new ext::HousamoSprite;
				this->addChild(*);
				uf::Serializer& pMetadata = ->getComponent<uf::Serializer>();
				->load("./entities/shiro.json");
				->initialize();

				pod::Transform<>& pTransform = ->getComponent<pod::Transform<>>();
				pTransform.position += transform.position + pod::Vector3f{ 2, 0, 0 };
			}
			// pong
			{
				uf::Object*  = new ext::HousamoSprite;
				this->addChild(*);
				uf::Serializer& pMetadata = ->getComponent<uf::Serializer>();
				->load("./entities/pongy.json");
				->initialize();

				pod::Transform<>& pTransform = ->getComponent<pod::Transform<>>();
				pTransform.position += transform.position + pod::Vector3f{ -2, 0, 0 };
			}
			return "true";
		}

		for ( uint i = 0; i < metadata["region"][""]["amount"].asUInt64(); ++i ) {
			uf::Object*  = new ext::HousamoSprite;
			this->addChild(*);
			
			// set name
			uf::Serializer& pMetadata = ->getComponent<uf::Serializer>();
			->load("./entities/.json");
		//	int ri = floor((noise[i][i][i] * metadata["region"][""]["list"].size());
			int ri = floor(r * metadata["region"][""]["list"].size());
			pMetadata[""] = metadata["region"][""]["list"][ri];
			pMetadata["music"] = metadata["region"][""]["music"];
		//	pMetadata["hostile"] = true;
			->initialize();

			pod::Transform<>& pTransform = ->getComponent<pod::Transform<>>();
			float rx = first ? 0.3 : (rand() % 100) / 100.0;
			float rz = first ? 0.7 : (rand() % 100) / 100.0;
			float spread = 32.0f;
			spread = std::min( metadata["region"]["size"][0].asFloat(), spread );
			spread = std::min( metadata["region"]["size"][1].asFloat(), spread );
			spread = std::min( metadata["region"]["size"][2].asFloat(), spread );
			pod::Vector3f randomOffset = {
				rx * spread - spread/2.0f,
				0,
				rz * spread - spread/2.0f,
			};
			pTransform.position += transform.position + randomOffset;
			pTransform.orientation = uf::quaternion::axisAngle( { 0.0f, 1.0f, 0.0f }, r * 2 * 3.1415926f );
		}
		return "true";
	});
}
void ext::Region::tick() {
	uf::Object::tick();

	// check if ready
	uf::Scene& world = this->getRootParent<uf::Scene>();
	uf::Serializer& metadata = this->getComponent<uf::Serializer>();
}
void ext::Region::destroy() {
	if ( this->hasComponent<ext::vulkan::RTGraphic>() ) {
		this->getComponent<ext::vulkan::RTGraphic>().destroy();
	}
	if ( this->hasComponent<ext::TerrainGenerator::mesh_t>() ) {
		this->getComponent<ext::TerrainGenerator::mesh_t>().destroy();
	}
	uf::Object::destroy();
}
void ext::Region::load() {
//	ext::TerrainGenerator::mesh_t& mesh = this->getComponent<ext::TerrainGenerator::mesh_t>();
	ext::TerrainGenerator& generator = this->getComponent<ext::TerrainGenerator>();
	uf::Serializer& metadata = this->getComponent<uf::Serializer>();

	if ( metadata["region"]["initialized"].asBool() ) return;

	pod::Vector3ui size; {
		size.x = metadata["region"]["size"][0].asUInt();
		size.y = metadata["region"]["size"][1].asUInt();
		size.z = metadata["region"]["size"][2].asUInt();
	}

	/* Metadata */ {
		metadata["region"]["modified"] = true;
		metadata["region"]["initialized"] = true;
		metadata["region"]["rasterized"] = false;
	}

	generator.initialize(size, metadata["region"]["modulus"].asUInt64());
	generator.generate(*this);

	/* Collider */ {
		pod::Transform<>& transform = this->getComponent<pod::Transform<>>();
		uf::CollisionBody& collider = this->getComponent<uf::CollisionBody>();
	
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

					uf::Collider* box = new uf::AABBox( offset, {0.5, 0.5, 0.5} );
					collider.add(box);
				}
			}
		}
	
	/*
		auto*** voxels = generator.getRawVoxels();
		for ( uint x = 0; x < size.x; ++x ) {
		for ( uint y = 0; y < size.y; ++y ) {
		for ( uint z = 0; z < size.z; ++z ) {
				pod::Vector3 offset = transform.position;
				offset.x += x - (size.x / 2.0f);
				offset.y += y - (size.y / 2.0f);
				offset.z += z - (size.z / 2.0f);

				ext::TerrainVoxel voxel = ext::TerrainVoxel::atlas( generator.getVoxel(x, y, z) );
				if ( !voxel.solid() ) continue;

				uf::Collider* box = new uf::AABBox( offset, {0.5, 0.5, 0.5} );
				collider.add(box);
			}
		}
		}
	*/
	

	}
/*
	if ( this->getComponent<pod::Transform<>>().position.y < 0 ) {
		float r = (rand() % 100) / 100.0;
		bool addLight = r < metadata["region"]["light"]["random"].asFloat();
		static bool first = false; if ( !first ) addLight = first = true;
		// Guarantee at least one light source (per XZ plane)
		if ( false && !addLight ) { addLight = true;
			ext::Terrain& parent = this->getParent<ext::Terrain>();
			for ( const uf::Entity* pRegion : parent.getChildren() ) if ( pRegion->getName() == "Region" ) {
				const pod::Transform<>& tRegion = pRegion->getComponent<pod::Transform<>>();
				const pod::Transform<>& transform = this->getComponent<pod::Transform<>>();
				if ( tRegion.position.y != transform.position.y ) continue;
				for ( const uf::Entity* pLight : pRegion->getChildren() ) if ( pLight->getName() == "Light" ) {
					addLight = false;
					break;
				}
				if ( !addLight ) break;
			}
		}
	}
*/
}
void ext::Region::render( ) {
	if ( !this->m_parent ) return;
	uf::Scene& root = this->getRootParent<uf::Scene>();
	ext::Terrain& terrain = this->getParent<ext::Terrain>();		
	uf::Serializer& metadata = this->getComponent<uf::Serializer>();
	

	if ( !metadata["region"]["generated"].asBool() ) return;

	uf::Object::render();

	/* Update uniforms */ if ( this->hasComponent<ext::vulkan::RTGraphic>() ) {
		auto& world = this->getRootParent<uf::Scene>();
		auto& rt = this->getComponent<ext::vulkan::RTGraphic>();
		auto& player = world.getController<uf::Object>();
		auto& camera = player.getComponent<uf::Camera>();
		auto& transform = player.getComponent<pod::Transform<>>();
		pod::Transform<> flatten = uf::transform::flatten(camera.getTransform(), true);
		rt.compute.uniforms.camera.view = uf::quaternion::matrix( flatten.orientation ); 
		rt.compute.uniforms.camera.aspectRatio = (float) camera.getSize().x / (float) camera.getSize().y;
		rt.compute.uniforms.camera.position = camera.getTransform().position + transform.position;
		rt.updateUniformBuffer();
	}
	/* Update uniforms */ if ( this->hasComponent<ext::TerrainGenerator::mesh_t>() ) {
		auto& world = this->getRootParent<uf::Scene>();
		auto& mesh = this->getComponent<ext::TerrainGenerator::mesh_t>();
		auto& camera = world.getController()->getComponent<uf::Camera>();
		
		if ( !mesh.generated ) return;
		camera.updateView();

		uf::StereoMeshDescriptor uniforms;
		uniforms.matrices.model = uf::matrix::identity();
		for ( std::size_t i = 0; i < 2; ++i ) {
			uniforms.matrices.view[i] = camera.getView( i );
			uniforms.matrices.projection[i] = camera.getProjection( i );
		}

		mesh.graphic.updateBuffer( uniforms, 0, false );
	}
}