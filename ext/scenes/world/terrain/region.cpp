#include "region.h"

#include "../world.h"
#include "../../../ext.h"
#include "generator.h"
#include "..//sprite.h"
#include <uf/engine/asset/asset.h>
#include <uf/utils/camera/camera.h>
#include <uf/utils/math/collision.h>
#include <uf/utils/graphic/graphic.h>
#include <uf/utils/graphic/mesh.h>
#include <uf/utils/string/ext.h>

namespace {
	std::string grabURI( std::string filename, std::string root = "" ) {
		if ( filename.substr(0,8) == "https://" ) return filename;
		std::string extension = uf::string::extension(filename);
		if ( filename[0] == '/' || root == "" ) {
			if ( extension == "json" ) root = "./data/entities/";
			if ( extension == "png" ) root = "./data/textures/";
			if ( extension == "ogg" ) root = "./data/audio/";
		}
		return uf::string::sanitize(filename, root);
	}
}

void ext::Region::initialize() {
	uf::Object::initialize();
	this->m_name = "Region";

	// alias Mesh types
	{
		this->addAlias<ext::TerrainGenerator::mesh_t, uf::Mesh>();
	}


	uf::Serializer& metadata = this->getComponent<uf::Serializer>();
	metadata["region"]["initialized"] = true;

	{
		std::string textureFilename = ""; {
			uf::Serializer& metadata = this->getParent().getComponent<uf::Serializer>();
			uf::Asset assetLoader;
			for ( uint i = 0; i < metadata["system"]["assets"].size(); ++i ) {
				if ( textureFilename != "" ) break;
				std::string filename = grabURI( metadata["system"]["assets"][i].asString(), metadata["system"]["root"].asString() );
				textureFilename = assetLoader.cache( filename );
			}
		}
		ext::TerrainGenerator::mesh_t& mesh = this->getComponent<ext::TerrainGenerator::mesh_t>();
		auto& graphic = this->getComponent<uf::Graphic>();

		graphic.initialize();
		graphic.process = false;

		auto& texture = graphic.material.textures.emplace_back();
		texture.sampler.filter = VK_FILTER_NEAREST;
		texture.loadFromFile( textureFilename );

		std::string suffix = ""; {
			std::string _ = this->getRootParent<uf::Scene>().getComponent<uf::Serializer>()["shaders"]["region"]["suffix"].asString();
			if ( _ != "" ) suffix = _ + ".";
		}
		graphic.material.attachShader("./data/shaders/terrain.stereo.vert.spv", VK_SHADER_STAGE_VERTEX_BIT);
		graphic.material.attachShader("./data/shaders/terrain."+suffix+"frag.spv", VK_SHADER_STAGE_FRAGMENT_BIT);
	}

	this->addHook( "region:Generate.%UID%", [&](const std::string& event)->std::string{	
		uf::Serializer json = event;

		uf::Serializer& metadata = this->getComponent<uf::Serializer>();
		if ( !metadata["region"]["initialized"].asBool() ) return "false";
		if ( metadata["region"]["generated"].asBool() ) return "false";

		pod::Vector3ui size; {
			size.x = metadata["region"]["size"][0].asUInt();
			size.y = metadata["region"]["size"][1].asUInt();
			size.z = metadata["region"]["size"][2].asUInt();
		}
		uint subdivisions = metadata["region"]["subdivisions"].asUInt();
		ext::TerrainGenerator& generator = this->getComponent<ext::TerrainGenerator>();
		generator.initialize(size, subdivisions);
		generator.generate(*this);
		generator.updateLight();

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

		ext::TerrainGenerator::mesh_t& mesh = this->getComponent<ext::TerrainGenerator::mesh_t>();
		auto& graphic = this->getComponent<uf::Graphic>();

		graphic.process = true;
		metadata["region"]["rasterized"] = true;

		return "true";
	});
	this->addHook( "region:Populate.%UID%", [&](const std::string& event)->std::string{	
		uf::Serializer json = event;

		if ( metadata["region"][""]["initialized"].asBool() ) return "false";

		metadata["region"][""]["initialized"] = true;
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
		// add lights
		{
			bool should = metadata["region"]["lights"]["should"].asBool();
			if ( r > metadata["region"]["lights"]["rate"].asFloat() ) should = false;
			// std::cout << "Rate: " << r << ": " << transform.position.x << ", " << transform.position.y << ", " << transform.position.z << std::endl;
			if (
				metadata["region"]["location"][0] == 0 &&
				metadata["region"]["location"][1] == 0 &&
				metadata["region"]["location"][2] == 0
			) {
				should = true;
			}
			if ( should ) {
				uf::Entity* light = this->findByUid(this->loadChild("./light.json", true));
				uf::Serializer& metadata = light->getComponent<uf::Serializer>();
				pod::Transform<>& lTransform = light->getComponent<pod::Transform<>>();
				lTransform.position = transform.position;
				metadata["light"]["color"][0] = r;
				metadata["light"]["color"][1] = r;
				metadata["light"]["color"][2] = r;
			}
		}
		// add mobs
		{
			bool should = metadata["region"][""]["should"].asBool();
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
				if ( metadata["region"][""]["NPCs"].asBool() ) {
					uf::Object*  = (uf::Object*) this->findByUid(this->loadChild("./shiro.json", true));
					pod::Transform<>& pTransform = ->getComponent<pod::Transform<>>();
					pTransform.position += transform.position + pod::Vector3f{ 2, 0, 0 };
				}
				// pong
				if ( metadata["region"][""]["NPCs"].asBool() ) {
					uf::Object*  = (uf::Object*) this->findByUid(this->loadChild("./pongy.json", true));
					uf::Serializer& pMetadata = ->getComponent<uf::Serializer>();

					pod::Transform<>& pTransform = ->getComponent<pod::Transform<>>();
					pTransform.position += transform.position + pod::Vector3f{ -2, 0, 0 };
				}
				return "true";
			}

			for ( uint i = 0; i < metadata["region"][""]["amount"].asUInt64(); ++i ) {
				uf::Object*  = (uf::Object*) this->findByUid(this->loadChild("./.json", false));
				// set name
				uf::Serializer& pMetadata = ->getComponent<uf::Serializer>();
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
		}
		return "true";
	});
}
void ext::Region::tick() {
	uf::Object::tick();
}
void ext::Region::destroy() {
	auto& graphic = this->getComponent<uf::Graphic>();
	graphic.destroy();

	uf::Object::destroy();
}
void ext::Region::render( ) {
	if ( !this->m_parent ) return;
	uf::Scene& root = this->getRootParent<uf::Scene>();
	ext::Terrain& terrain = this->getParent<ext::Terrain>();		
	uf::Serializer& metadata = this->getComponent<uf::Serializer>();
	
	uf::Object::render();

	if ( !metadata["region"]["rasterized"].asBool() ) return;
	/* Update uniforms */ if ( this->hasComponent<ext::TerrainGenerator::mesh_t>() ) {
		auto& world = this->getRootParent<uf::Scene>();
		auto& mesh = this->getComponent<ext::TerrainGenerator::mesh_t>();
		auto& graphic = this->getComponent<uf::Graphic>();
		auto& camera = world.getController()->getComponent<uf::Camera>();		
		if ( !graphic.initialized ) return;
	//	auto& uniforms = graphic.uniforms<uf::StereoMeshDescriptor>();
		auto& uniforms = graphic.material.shaders.front().uniforms.front().get<uf::StereoMeshDescriptor>();
		uniforms.matrices.model = uf::matrix::identity();
		for ( std::size_t i = 0; i < 2; ++i ) {
			uniforms.matrices.view[i] = camera.getView( i );
			uniforms.matrices.projection[i] = camera.getProjection( i );
		}

	//	graphic.updateBuffer( uniforms, 0, false );
		graphic.material.shaders.front().updateBuffer( uniforms, 0, false );
	}
}