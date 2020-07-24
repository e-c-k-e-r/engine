#include "generator.h"

#include "../../../ext.h"
#include "voxel.h"

#include <uf/engine/entity/entity.h>

#include <uf/utils/math/transform.h>
#include <uf/utils/math/physics.h>

#include <uf/utils/string/rle.h>
#include <uf/utils/string/base64.h>
#include <uf/utils/string/hash.h>

#include <fstream>
#include <queue>

namespace {
	bool SWIZZLE_OPTIMIZATION = false;
	ext::TerrainVoxel::light_t AMBIENT_LIGHT = 0x6666; //{std::numeric_limits<ext::TerrainVoxel::light_t::type_t>::max(), std::numeric_limits<ext::TerrainVoxel::light_t::type_t>::max(), std::numeric_limits<ext::TerrainVoxel::light_t::type_t>::max()};
	struct COLOR {
		uint r : 4;
		uint g : 4;
		uint b : 4;
		uint a : 4;
	};

	COLOR uint16ToColor( uint16_t color ) {
		COLOR result;
		memcpy( &result, &color, sizeof color );
		return result;
	}
	uint16_t colorToUint16( COLOR color ) {
		uint16_t result;
		memcpy( &result, &color, sizeof color );
		return result;
	}
	bool inBounds( int x, int y, int z, const pod::Vector3ui& size ) {
		return (x >= 0 && x < size.x && y >= 0 && y < size.y && z >= 0 && z < size.z);
	}
}


ext::TerrainGenerator::Swizzle ext::TerrainGenerator::DEFAULT_SWIZZLE = Swizzle::YZX;
uf::PerlinNoise ext::TerrainGenerator::noise;

void ext::TerrainGenerator::initialize( const pod::Vector3ui& size, uint modulus ){
	this->m_size = size;
	this->m_modulus = modulus;
}
void ext::TerrainGenerator::destroy(){
/*
	if ( !this->m_voxels.id.raw ) return;
	for ( uint x = 0; x < this->m_size.x; ++x ) { 
		for ( uint y = 0; y < this->m_size.y; ++y ) delete[] this->m_voxels.id.raw[x][y];
		delete[] this->m_voxels.id.raw[x];
	}
	delete[] this->m_voxels.id.raw;
	this->m_voxels.id.raw = NULL;
*/

//	this->m_voxels.id.raw.clear();
//	this->m_voxels.lighting.raw.clear();

}
void ext::TerrainGenerator::generate( ext::Region& region ){
	// if ( !this->m_voxels.id.raw.empty() || !this->m_voxels.id.rle.empty() ) return;
	this->destroy();

	uint half_x = this->m_size.x / 2;
	uint half_y = this->m_size.y / 2;
	uint half_z = this->m_size.z / 2;

	const pod::Transform<>& transform = region.getComponent<pod::Transform<>>();
	const pod::Vector3i location = {
		transform.position.x / this->m_size.x,
		transform.position.y / this->m_size.y,
		transform.position.z / this->m_size.z,
	}; this->m_location = location;
	this->m_terrain = &region.getParent();
/*
	COLOR ambientLight = uint16ToColor(AMBIENT_LIGHT); {
		uf::Serializer& tMetadata = region.getComponent<uf::Serializer>();
		ambientLight.r *= tMetadata["region"]["light"]["ambient"][0].asFloat();
		ambientLight.g *= tMetadata["region"]["light"]["ambient"][1].asFloat();
		ambientLight.b *= tMetadata["region"]["light"]["ambient"][2].asFloat();
	}
*/
	std::string base = region.getParent().getComponent<uf::Serializer>()["_config"]["hash"].asString();
	std::string filename = "./data/save/" + base + "/regions/" + std::to_string(location.x) + "." + std::to_string(location.y) + "." + std::to_string(location.z) + ".json";
	// old region, load save
	if ( uf::string::exists( filename ) ) {
		uf::Serializer save; save.readFromFile(filename);

		// ID
		if ( save["voxels"]["id"]["base64"].isString() ) {
			std::string base64 = save["voxels"]["id"]["base64"].asString();
			auto raw = uf::base64::decode( base64 );

			if ( save["voxels"]["id"]["rle"].asBool() ) {
				// [uint8_t]*4 -> [uint16_t, uint16_t]
				this->m_voxels.id.rle.resize( raw.size() / ( sizeof(pod::RLE<ext::TerrainVoxel::uid_t>::length_t) + sizeof(pod::RLE<ext::TerrainVoxel::uid_t>::value_t) ) );
				memcpy( &this->m_voxels.id.rle[0], &raw[0], raw.size() );
			// normal voxel load or something
			} else {

			}
			this->m_voxels.id.swizzle = (TerrainGenerator::Swizzle) save["voxels"]["id"]["swizzle"].asUInt64();
			this->unwrapVoxel();
		}
		// Lighting
		if ( save["voxels"]["lighting"]["base64"].isString() ) {
			std::string base64 = save["voxels"]["lighting"]["base64"].asString();
			auto raw = uf::base64::decode( base64 );

			if ( save["voxels"]["lighting"]["rle"].asBool() ) {
				// [uint8_t]*4 -> [uint8_t, uint8_t]
				this->m_voxels.lighting.rle.resize( raw.size() / ( sizeof(pod::RLE<ext::TerrainVoxel::light_t>::length_t) + sizeof(pod::RLE<ext::TerrainVoxel::light_t>::value_t) ) );
				memcpy( &this->m_voxels.lighting.rle[0], &raw[0], raw.size() );
			// normal voxel load or something
			} else {

			}
			this->m_voxels.lighting.swizzle = (TerrainGenerator::Swizzle) save["voxels"]["lighting"]["swizzle"].asUInt64();
			this->unwrapLight();
		}
	// load failed / new region
	}

	// maze
	/*
	{
		ext::Maze& maze = this->m_terrain->getComponent<ext::Maze>();
		int ROW = this->m_location.z + (maze.WIDTH / 2) - 1;
		int COL = this->m_location.x + (maze.LENGTH / 2) - 1;
		auto value = maze( ROW, COL, 0 );
		bool east = value & ext::Maze::EAST; // ROW + 1 < maze.WIDTH ? maze( ROW + 1, COL, 1 ) : 0;
		bool west = value & ext::Maze::WEST; // ROW - 1 > 0 ? maze( ROW - 1, COL, 1 ) : 0;
		bool north = value & ext::Maze::NORTH; // COL + 1 < maze.LENGTH ? maze( ROW, COL + 1, 1 ) : 0;
		bool south = value & ext::Maze::SOUTH; // COL - 1 > 0 ? maze( ROW, COL - 1, 1 ) : 0;
		std::cout << ROW << ", " << COL << " = " << value << "\t" << east << " " << west << " " << north << " " << south << std::endl;
	}
	*/

	if ( this->m_voxels.id.rle.empty() ) {
		double maxValue = 0.0;
		double base = 0;
		
		ext::TerrainVoxel::uid_t raw_id[this->m_size.x][this->m_size.y][this->m_size.z];
		ext::TerrainVoxel::light_t raw_lighting[this->m_size.x][this->m_size.y][this->m_size.z];
		double raw_noise[this->m_size.x][this->m_size.y][this->m_size.z];

		for ( uint y = 0; y < this->m_size.y; ++y )
		for ( uint z = 0; z < this->m_size.z; ++z )
		for ( uint x = 0; x < this->m_size.x; ++x ) {
			pod::Vector3d position = {
				transform.position.x - half_x, transform.position.y - half_y, transform.position.z - half_z
			};
			position.x += (double) x / (double) this->m_size.x;
			position.y += (double) y / (double) this->m_size.y;
			position.z += (double) z / (double) this->m_size.z;
			raw_noise[x][y][z] = ext::TerrainGenerator::noise.sample( position );
			maxValue = std::max( maxValue, raw_noise[x][y][z] );
		}
		

		for ( uint y = 0; y < this->m_size.y; ++y )
		for ( uint z = 0; z < this->m_size.z; ++z )
		for ( uint x = 0; x < this->m_size.x; ++x )
			base += raw_noise[x][y][z] / maxValue;

		base /= this->m_size.x * this->m_size.y * this->m_size.z;

		uint partitions = this->m_modulus;
		struct {
			ext::TerrainVoxel::uid_t floor = ext::TerrainVoxelFloor().uid();
			ext::TerrainVoxel::uid_t wall = ext::TerrainVoxelWall().uid();
			ext::TerrainVoxel::uid_t ceiling = ext::TerrainVoxelCeiling().uid();
			ext::TerrainVoxel::uid_t stair = ext::TerrainVoxelStair().uid();
			ext::TerrainVoxel::uid_t pillar = ext::TerrainVoxelPillar().uid();
			ext::TerrainVoxel::uid_t lava = ext::TerrainVoxelLava().uid();
		} atlas;
		
		this->m_voxels.id.raw.reserve( this->m_size.x * this->m_size.y * this->m_size.z );
		this->m_voxels.lighting.raw.reserve( this->m_size.x * this->m_size.y * this->m_size.z );

		ext::Maze& maze = this->m_terrain->getComponent<ext::Maze>();
		int LABYRINTH = ext::Maze::FLOOR | ext::Maze::CEIL;
		{
			int ROW = this->m_location.z + (maze.WIDTH / 2) - 1;
			int COL = this->m_location.x + (maze.LENGTH / 2) - 1;
			if ( ROW >= 0 && ROW < maze.WIDTH && COL >= 0 && COL < maze.LENGTH )
				LABYRINTH = maze( ROW, COL, 0 );
		}

		std::size_t i = 0;
		for ( uint y = 0; y < this->m_size.y; ++y ) {
		for ( uint z = 0; z < this->m_size.z; ++z ) {
		for ( uint x = 0; x < this->m_size.x; ++x ) {
			ext::TerrainVoxel::uid_t voxel = 0;

			double e = raw_noise[x][y][z] / maxValue;
			// generate walls if wall exists in maze
	
		//	if ( (x) % (this->m_size.x / partitions) == 0 ) voxel = atlas.wall;
		//	if ( (z) % (this->m_size.z / partitions) == 0 ) voxel = atlas.wall;
		/*
			if ( (x + 1) % (this->m_size.x / partitions) == 0 ) voxel = atlas.wall;
			if ( (z + 1) % (this->m_size.z / partitions) == 0 ) voxel = atlas.wall;

			if ( y > 0 && y < this->m_size.y - 1 ) {
				if ( x % (this->m_size.x / partitions) >= 3 && x % (this->m_size.x / partitions) <= partitions - 4 ) voxel = 0;
				if ( z % (this->m_size.z / partitions) >= 3 && z % (this->m_size.z / partitions) <= partitions - 4 ) voxel = 0;
			}
		*/
			if ( y <= 1 ) voxel = atlas.floor; if ( y == this->m_size.y - 1 ) voxel = atlas.ceiling;
			if ( uf::vector::sum(location) != 0 ) {
				if ( e < 0.5 ) voxel = y == 0 ? atlas.lava : 0;
			//	if ( raw_noise[x][1][z] / maxValue < 0.2 || raw_noise[x][2][z] / maxValue > 0.8 ) voxel = atlas.stair;
				if ( raw_noise[x][1][z] < 0.2 || raw_noise[x][2][z] > 0.8 ) voxel = atlas.stair;
			} else {
			//	if ( x == half_x && y == 4 && z == half_z ) voxel = atlas.lava;
			}
	
			auto light = ext::TerrainVoxel::atlas(voxel).light();

			if ( LABYRINTH & ext::Maze::EAST ) if ( x == 0 ) voxel = atlas.wall;
			if ( LABYRINTH & ext::Maze::WEST ) if ( x == this->m_size.x - 1 ) voxel = atlas.wall;
			if ( LABYRINTH & ext::Maze::NORTH ) if ( z == 0 ) voxel = atlas.wall;
			if ( LABYRINTH & ext::Maze::SOUTH ) if ( z == this->m_size.z - 1 ) voxel = atlas.wall;
		//	if ( LABYRINTH & ext::Maze::FLOOR ) if ( y == 0 ) voxel = atlas.floor;
		//	if ( LABYRINTH & ext::Maze::CEIL ) if ( y == this->m_size.y - 1 ) voxel = atlas.ceiling;
			
			if ( light < AMBIENT_LIGHT ) light = AMBIENT_LIGHT;

			raw_id[x][y][z] = voxel;
			raw_lighting[x][y][z] = light;
			
			// write to raw buffers
			this->m_voxels.id.raw.push_back(voxel);
			this->m_voxels.lighting.raw.push_back(light);
		}
		}
		}

		uf::Serializer serializer;
		// redundant but sure
		serializer["location"][0] = location.x;
		serializer["location"][1] = location.y;
		serializer["location"][2] = location.z;
		
		this->writeToFile();
	}
	this->destroy();
}

void ext::TerrainGenerator::writeToFile() {
	this->wrapVoxel();
	this->wrapLight();

	uf::Serializer serializer;
	std::string base = this->m_terrain ? this->m_terrain->getComponent<uf::Serializer>()["_config"]["hash"].asString() : "UNKNOWN";
	std::string filename = "./data/save/" + base + "/regions/" + std::to_string(this->m_location.x) + "." + std::to_string(this->m_location.y) + "." + std::to_string(this->m_location.z) + ".json";
	{
		// encode as base64, json safe
		serializer["voxels"]["id"]["base64"] = uf::base64::encode( this->m_voxels.id.rle );
		// hash of raw data for data integrity
		serializer["voxels"]["id"]["hash"] = uf::string::sha256( uf::base64::decode(serializer["voxels"]["id"]["rle"]["base64"].asString()) );
		serializer["voxels"]["id"]["rle"] = true;
		serializer["voxels"]["id"]["swizzle"] = this->m_voxels.id.swizzle;
	}
	{
		// encode as base64, json safe
		serializer["voxels"]["lighting"]["base64"] = uf::base64::encode( this->m_voxels.lighting.rle );
		// hash of raw data for data integrity
		serializer["voxels"]["lighting"]["hash"] = uf::string::sha256( uf::base64::decode(serializer["voxels"]["lighting"]["rle"]["base64"].asString()) );
		serializer["voxels"]["lighting"]["rle"] = true;
		serializer["voxels"]["lighting"]["swizzle"] = this->m_voxels.lighting.swizzle;
	}

	serializer.writeToFile(filename);

	this->destroy();
}

std::vector<ext::TerrainVoxel::uid_t> ext::TerrainGenerator::getRawVoxels() {
	return this->m_voxels.id.raw;
}
const pod::RLE<ext::TerrainVoxel::uid_t>::string_t& ext::TerrainGenerator::getVoxels() const {
	return this->m_voxels.id.rle;
}

ext::TerrainVoxel::uid_t ext::TerrainGenerator::getVoxel( int x, int y, int z ) const {
	// check oob
	if ( !inBounds( x, y, z, this->m_size )  ) {
		if ( !this->m_terrain ) return 0;

		const ext::Terrain& terrain = *((const ext::Terrain*)this->m_terrain);
		pod::Vector3i location = this->m_location;
		while ( !inBounds( x, y, z, this->m_size )  ) {
			if ( x < 0 ) {
				x += this->m_size.x;
				--location.x;
			} else if ( x >= this->m_size.x ) {
				x -= this->m_size.x;
				++location.x;
			}
			if ( y < 0 ) {
				y += this->m_size.y;
				--location.y;
			} else if ( y >= this->m_size.y ) {
				y -= this->m_size.y;
				++location.y;
			}
			if ( z < 0 ) {
				z += this->m_size.z;
				--location.z;
			} else if ( z >= this->m_size.z ) {
				z -= this->m_size.z;
				++location.z;
			}
		}
		ext::Region* region = terrain.at( location );
		if ( !region ) return 0;
		return region->getComponent<ext::TerrainGenerator>().getVoxel( x, y, z );
	/*
		while ( x < 0 ) {
			--location.x;
			x += this->m_size.x;
		}
		while ( x >= this->m_size.x - 1 ) {
			++location.x;
			x -= this->m_size.x;
		}
		while ( y < 0 ) {
			--location.y;
			y += this->m_size.y;
		}
		while ( y >= this->m_size.y - 1 ) {
			++location.y;
			y -= this->m_size.y;
		}
		while ( z < 0 ) {
			--location.z;
			z += this->m_size.z;
		}
		while ( z >= this->m_size.z - 1 ) {
			++location.z;
			z -= this->m_size.z;
		}
		ext::Region* region = terrain.at( location );
		if ( !region ) return 0;
		return region->getComponent<ext::TerrainGenerator>().getVoxel( x, y, z );
	*/
	}

//	if ( this->m_voxels.id.raw ) return this->m_voxels.id.raw[x][y][z];
	if ( !this->m_voxels.id.raw.empty() ) {
		std::size_t i = this->wrapPosition( x, y, z, this->m_voxels.id.swizzle );
		return this->m_voxels.id.raw[i];
	}

	std::size_t i = 0;
	for ( auto& _ : this->m_voxels.id.rle ) {
		for ( std::size_t __ = 0; __ < _.length; ++__ ) {
			auto v = this->unwrapIndex( i++, this->m_voxels.id.swizzle );
			if ( v.x == x && v.y == y && v.z == z ) return _.value;
		}
	}
	return 0;
}
ext::TerrainVoxel::uid_t ext::TerrainGenerator::getVoxel( const pod::Vector3i& location ) const {
	return this->getVoxel( location.x, location.y, location.z );
}
ext::TerrainVoxel::light_t ext::TerrainGenerator::getLight( int x, int y, int z ) const {
	// check oob
	if ( !inBounds( x, y, z, this->m_size )  ) {
		if ( !this->m_terrain ) {
//			std::cout << "Invalid call @ getLight( " << x << ", " << y << ", " << z << " ) " << std::endl;
			return 0x0000;
		}

		const ext::Terrain& terrain = *((const ext::Terrain*)this->m_terrain);
		pod::Vector3i location = this->m_location;
		while ( !inBounds( x, y, z, this->m_size )  ) {
			if ( x < 0 ) {
				x += this->m_size.x;
				--location.x;
			} else if ( x >= this->m_size.x ) {
				x -= this->m_size.x;
				++location.x;
			}
			if ( y < 0 ) {
				y += this->m_size.y;
				--location.y;
			} else if ( y >= this->m_size.y ) {
				y -= this->m_size.y;
				++location.y;
			}
			if ( z < 0 ) {
				z += this->m_size.z;
				--location.z;
			} else if ( z >= this->m_size.z ) {
				z -= this->m_size.z;
				++location.z;
			}
		}
	/*
		while ( x < 0 ) {
			--location.x;
			x += this->m_size.x;
		}
		while ( x >= this->m_size.x - 1 ) {
			++location.x;
			x -= this->m_size.x - 1;
		}
		while ( y < 0 ) {
			--location.y;
			y += this->m_size.y;
		}
		while ( y >= this->m_size.y - 1 ) {
			++location.y;
			y -= this->m_size.y - 1;
		}
		while ( z < 0 ) {
			--location.z;
			z += this->m_size.z;
		}
		while ( z >= this->m_size.z - 1 ) {
			++location.z;
			z -= this->m_size.z - 1;
		}
	*/
		ext::Region* region = terrain.at( location );
		if ( !region ) {
		//	std::cout << "Out of bounds Region("<< location.x << ", " << location.y << ", " << location.z <<") @ getLight( " << x << ", " << y << ", " << z << " ) " << std::endl;
			return 0xFFFF;
		}
		return region->getComponent<ext::TerrainGenerator>().getLight( x, y, z );
	}

/*
	if ( !this->m_voxels.lighting.raw.empty() ) {
		std::size_t i = this->wrapPosition( x, y, z, this->m_voxels.lighting.swizzle );
		return this->m_voxels.lighting.raw[i];
	}
*/
	std::size_t i = 0;
	for ( auto& _ : this->m_voxels.lighting.rle ) {
		for ( std::size_t __ = 0; __ < _.length; ++__ ) {
			auto v = this->unwrapIndex( i++, this->m_voxels.lighting.swizzle );
			if ( v.x == x && v.y == y && v.z == z ) {
				return _.value;
			}
		}
	}
	return 0x0000;
}
ext::TerrainVoxel::light_t ext::TerrainGenerator::getLight( const pod::Vector3i& location ) const {
	return this->getLight( location.x, location.y, location.z );
}
pod::Vector3ui ext::TerrainGenerator::unwrapIndex( std::size_t i, Swizzle swizzle ) const {
	// ordered from first to change to last to change
	/*
		result.x = i % this->m_size.x;
		result.y = ( i / this->m_size.x ) % this->m_size.y;
		result.z = i / ( this->m_size.x * this->m_size.y );
	*/
	pod::Vector3ui size;
	switch ( swizzle ) {
		case Swizzle::YZX:
			size = {
				this->m_size.x,
				this->m_size.z,
				this->m_size.y,
			};
			return {
				i % size.x,
				i / ( size.x * size.y ),
				( i / size.x ) % size.y,
			};
		break;
		case Swizzle::ZXY:
			size = {
				this->m_size.y,
				this->m_size.x,
				this->m_size.z,
			};
			return {
				( i / size.x ) % size.y,
				i % size.x,
				i / ( size.x * size.y ),
			};
		break;
		case Swizzle::XYZ:
		default:
			size = {
				this->m_size.z,
				this->m_size.y,
				this->m_size.x,
			};
			return {
				( i / size.x ) % size.y,
				i / ( size.x * size.y ),
				i % size.x,
			};
		break;
	}
}

// (y, z, x) -> (x, y, z)
std::size_t ext::TerrainGenerator::wrapPosition( uint x, uint y, uint z, Swizzle swizzle ) const {
	// ordered from first to change to last to change
	pod::Vector3ui input;
	pod::Vector3ui size;
	switch ( swizzle ) {
		case Swizzle::YZX:
			input = {
				x,
				z,
				y,
			};
			size = {
				this->m_size.x,
				this->m_size.z,
				this->m_size.y,
			};
		break;
		case Swizzle::ZXY:
			input = {
				y,
				x,
				z,
			};
			size = {
				this->m_size.y,
				this->m_size.x,
				this->m_size.z,
			};
		break;
		case Swizzle::XYZ:
			input = {
				z,
				y,
				x,
			};
			size = {
				this->m_size.z,
				this->m_size.y,
				this->m_size.x,
			};
		break;
	}
	return size.y * size.x * input.z + size.x * input.y + input.x;
//	return size.z * size.x * input.y + size.x * input.z + input.x;
//	return (x * this->m_size.y * this->m_size.z) + (z * this->m_size.y) + y;
}
std::size_t ext::TerrainGenerator::wrapPosition( const pod::Vector3ui& position, Swizzle swizzle ) const {
	return this->wrapPosition( position.x, position.y, position.z, swizzle );
}

void ext::TerrainGenerator::wrapVoxel() { if ( this->m_voxels.id.raw.empty() ) return;
	this->m_voxels.id.rle.clear();

	ext::TerrainVoxel::uid_t raw_id[this->m_size.x][this->m_size.y][this->m_size.z];
	this->m_voxels.id.swizzle = ext::TerrainGenerator::DEFAULT_SWIZZLE;
	
	for ( std::size_t i = 0; i < this->m_size.x * this->m_size.y * this->m_size.z; ++i ) {
		auto position = this->unwrapIndex( i, this->m_voxels.id.swizzle );
		raw_id[position.x][position.y][position.z] = this->m_voxels.id.raw[i];
	}

	std::vector<ext::TerrainVoxel::uid_t> buffer;
	buffer.reserve( this->m_size.x * this->m_size.y * this->m_size.z );

	// use YZX
	if ( SWIZZLE_OPTIMIZATION ) {
		pod::RLE<ext::TerrainVoxel::uid_t>::string_t xyz, yzx, zxy;
		{
			for ( uint x = 0; x < this->m_size.x; x++ ) 
			for ( uint y = 0; y < this->m_size.y; y++ ) 
			for ( uint z = 0; z < this->m_size.z; z++ )
				buffer.push_back( raw_id[x][y][z] );
			xyz = uf::rle::encode( buffer );
		}
		buffer.clear();
		buffer.reserve( this->m_size.x * this->m_size.y * this->m_size.z );
		{
			for ( uint y = 0; y < this->m_size.y; y++ ) 
			for ( uint z = 0; z < this->m_size.z; z++ )
			for ( uint x = 0; x < this->m_size.x; x++ ) 
				buffer.push_back( raw_id[x][y][z] );
			yzx = uf::rle::encode( buffer );
		}
		buffer.clear();
		buffer.reserve( this->m_size.x * this->m_size.y * this->m_size.z );
		{
			for ( uint z = 0; z < this->m_size.z; z++ )
			for ( uint x = 0; x < this->m_size.x; x++ ) 
			for ( uint y = 0; y < this->m_size.y; y++ ) 
				buffer.push_back( raw_id[x][y][z] );
			zxy = uf::rle::encode( buffer );
		}

		// xyz
		if ( xyz.size() <= yzx.size() && xyz.size() <= zxy.size() )  {
			this->m_voxels.id.swizzle = Swizzle::XYZ;
			this->m_voxels.id.rle = xyz;
		// yzx
		} else if ( yzx.size() <= xyz.size() && yzx.size() <= zxy.size() ) {
			this->m_voxels.id.swizzle = Swizzle::YZX;
			this->m_voxels.id.rle = yzx;
		} else if ( zxy.size() <= xyz.size() && zxy.size() <= yzx.size() ) {
			this->m_voxels.id.swizzle = Swizzle::ZXY;
			this->m_voxels.id.rle = zxy;
		} else {
			this->m_voxels.id.swizzle = Swizzle::YZX;
			this->m_voxels.id.rle = yzx;
		}
	} else {
		for ( uint y = 0; y < this->m_size.y; y++ ) 
		for ( uint z = 0; z < this->m_size.z; z++ )
		for ( uint x = 0; x < this->m_size.x; x++ ) 
			buffer.push_back( raw_id[x][y][z] );
		this->m_voxels.id.swizzle = Swizzle::YZX;
		this->m_voxels.id.rle = uf::rle::encode( buffer );
	}
}
void ext::TerrainGenerator::wrapLight() { if ( this->m_voxels.lighting.raw.empty() ) return;
	this->m_voxels.lighting.rle.clear();

	ext::TerrainVoxel::light_t raw_lighting[this->m_size.x][this->m_size.y][this->m_size.z];
	this->m_voxels.lighting.swizzle = ext::TerrainGenerator::DEFAULT_SWIZZLE;
	
	for ( std::size_t i = 0; i < this->m_size.x * this->m_size.y * this->m_size.z; ++i ) {
		auto position = this->unwrapIndex( i, this->m_voxels.lighting.swizzle );
		raw_lighting[position.x][position.y][position.z] = this->m_voxels.lighting.raw[i];
	}

	std::vector<ext::TerrainVoxel::light_t> buffer;
	buffer.reserve( this->m_size.x * this->m_size.y * this->m_size.z );

	// use YZX
	if ( SWIZZLE_OPTIMIZATION ) {
		pod::RLE<ext::TerrainVoxel::light_t>::string_t xyz, yzx, zxy;
		{
			for ( uint x = 0; x < this->m_size.x; x++ ) 
			for ( uint y = 0; y < this->m_size.y; y++ ) 
			for ( uint z = 0; z < this->m_size.z; z++ )
				buffer.push_back( raw_lighting[x][y][z] );
			xyz = uf::rle::encode( buffer );
		}
		buffer.clear();
		buffer.reserve( this->m_size.x * this->m_size.y * this->m_size.z );
		{
			for ( uint y = 0; y < this->m_size.y; y++ ) 
			for ( uint z = 0; z < this->m_size.z; z++ )
			for ( uint x = 0; x < this->m_size.x; x++ ) 
				buffer.push_back( raw_lighting[x][y][z] );
			yzx = uf::rle::encode( buffer );
		}
		buffer.clear();
		buffer.reserve( this->m_size.x * this->m_size.y * this->m_size.z );
		{
			for ( uint z = 0; z < this->m_size.z; z++ )
			for ( uint x = 0; x < this->m_size.x; x++ ) 
			for ( uint y = 0; y < this->m_size.y; y++ ) 
				buffer.push_back( raw_lighting[x][y][z] );
			zxy = uf::rle::encode( buffer );
		}

		// xyz
		if ( xyz.size() <= yzx.size() && xyz.size() <= zxy.size() )  {
			this->m_voxels.lighting.swizzle = Swizzle::XYZ;
			this->m_voxels.lighting.rle = xyz;
		// yzx
		} else if ( yzx.size() <= xyz.size() && yzx.size() <= zxy.size() ) {
			this->m_voxels.lighting.swizzle = Swizzle::YZX;
			this->m_voxels.lighting.rle = yzx;
		} else if ( zxy.size() <= xyz.size() && zxy.size() <= yzx.size() ) {
			this->m_voxels.lighting.swizzle = Swizzle::ZXY;
			this->m_voxels.lighting.rle = zxy;
		} else {
			this->m_voxels.lighting.swizzle = Swizzle::YZX;
			this->m_voxels.lighting.rle = yzx;
		}
	} else {
		for ( uint y = 0; y < this->m_size.y; y++ ) 
		for ( uint z = 0; z < this->m_size.z; z++ )
		for ( uint x = 0; x < this->m_size.x; x++ ) 
			buffer.push_back( raw_lighting[x][y][z] );
		this->m_voxels.lighting.swizzle = Swizzle::YZX;

		this->m_voxels.lighting.rle = uf::rle::encode( buffer );
	}
}
void ext::TerrainGenerator::unwrapVoxel() {
	ext::TerrainVoxel::uid_t raw_id[this->m_size.x][this->m_size.y][this->m_size.z];

	this->m_voxels.id.raw.clear();
	this->m_voxels.id.raw.reserve(this->m_size.x * this->m_size.y * this->m_size.z);

	std::size_t i = 0;
	for ( auto& _ : this->m_voxels.id.rle ) {
		for ( std::size_t __ = 0; __ < _.length; ++__ ) {
			auto v = this->unwrapIndex( i++, this->m_voxels.id.swizzle );
			raw_id[v.x][v.y][v.z] = _.value;
		}
	}
	for ( uint y = 0; y < this->m_size.y; y++ ) 
	for ( uint z = 0; z < this->m_size.z; z++ )
	for ( uint x = 0; x < this->m_size.x; x++ ) 
		this->m_voxels.id.raw.push_back( raw_id[x][y][z] );
}
void ext::TerrainGenerator::unwrapLight() {
	ext::TerrainVoxel::light_t raw_lighting[this->m_size.x][this->m_size.y][this->m_size.z];

	this->m_voxels.lighting.raw.clear();
	this->m_voxels.lighting.raw.reserve(this->m_size.x * this->m_size.y * this->m_size.z);

	std::size_t i = 0;
	for ( auto& _ : this->m_voxels.lighting.rle ) {
		for ( std::size_t __ = 0; __ < _.length; ++__ ) {
			auto v = this->unwrapIndex( i++, this->m_voxels.lighting.swizzle );
			raw_lighting[v.x][v.y][v.z] = _.value;
		}
	}
	for ( uint y = 0; y < this->m_size.y; y++ ) 
	for ( uint z = 0; z < this->m_size.z; z++ )
	for ( uint x = 0; x < this->m_size.x; x++ ) 
		this->m_voxels.lighting.raw.push_back( raw_lighting[x][y][z] );
}

void ext::TerrainGenerator::setVoxel( int x, int y, int z, const ext::TerrainVoxel::uid_t& voxel, bool update ){
	// check oob
	if ( !inBounds( x, y, z, this->m_size )  ) {
		if ( !this->m_terrain ) return;

		const ext::Terrain& terrain = *((const ext::Terrain*)this->m_terrain);
		pod::Vector3i location = this->m_location;
		while ( !inBounds( x, y, z, this->m_size )  ) {
			if ( x < 0 ) {
				x += this->m_size.x;
				--location.x;
			} else if ( x >= this->m_size.x ) {
				x -= this->m_size.x;
				++location.x;
			}
			if ( y < 0 ) {
				y += this->m_size.y;
				--location.y;
			} else if ( y >= this->m_size.y ) {
				y -= this->m_size.y;
				++location.y;
			}
			if ( z < 0 ) {
				z += this->m_size.z;
				--location.z;
			} else if ( z >= this->m_size.z ) {
				z -= this->m_size.z;
				++location.z;
			}
		}
	/*
		while ( x < 0 ) {
			--location.x;
			x += this->m_size.x;
		}
		while ( x >= this->m_size.x - 1 ) {
			++location.x;
			x -= this->m_size.x - 1;
		}
		while ( y < 0 ) {
			--location.y;
			y += this->m_size.y;
		}
		while ( y >= this->m_size.y - 1 ) {
			++location.y;
			y -= this->m_size.y - 1;
		}
		while ( z < 0 ) {
			--location.z;
			z += this->m_size.z;
		}
		while ( z >= this->m_size.z - 1 ) {
			++location.z;
			z -= this->m_size.z - 1;
		}
	*/
		ext::Region* region = terrain.at( location );
		if ( !region ) return;
		region->getComponent<ext::TerrainGenerator>().setVoxel( x, y, z, voxel, update );
		return;
	}

	// set modified
	if ( this->m_terrain ) {
		const ext::Terrain& terrain = *((const ext::Terrain*)this->m_terrain);
		ext::Region* region = terrain.at( this->m_location );
		if ( region ) region->getComponent<uf::Serializer>()["region"]["modified"] = true;
	}

	std::size_t i = this->wrapPosition(x, y, z, this->m_voxels.id.swizzle);
	this->m_voxels.id.raw[i] = voxel;

	this->wrapVoxel();
}
void ext::TerrainGenerator::setVoxel( const pod::Vector3i& location, const ext::TerrainVoxel::uid_t& light, bool update ){
	return this->setVoxel( location.x, location.y, location.z, light, update );
}
void ext::TerrainGenerator::setLight( int x, int y, int z, const ext::TerrainVoxel::light_t& light, bool update ){
	// check oob
	if ( !inBounds( x, y, z, this->m_size )  ) {
		if ( !this->m_terrain ) return;

		const ext::Terrain& terrain = *((const ext::Terrain*)this->m_terrain);
		pod::Vector3i location = this->m_location;
		while ( !inBounds( x, y, z, this->m_size )  ) {
			if ( x < 0 ) {
				x += this->m_size.x;
				--location.x;
			} else if ( x >= this->m_size.x ) {
				x -= this->m_size.x;
				++location.x;
			}
			if ( y < 0 ) {
				y += this->m_size.y;
				--location.y;
			} else if ( y >= this->m_size.y ) {
				y -= this->m_size.y;
				++location.y;
			}
			if ( z < 0 ) {
				z += this->m_size.z;
				--location.z;
			} else if ( z >= this->m_size.z ) {
				z -= this->m_size.z;
				++location.z;
			}
		}
	/*
		while ( x < 0 ) {
			--location.x;
			x += this->m_size.x;
		}
		while ( x >= this->m_size.x - 1 ) {
			++location.x;
			x -= this->m_size.x - 1;
		}
		while ( y < 0 ) {
			--location.y;
			y += this->m_size.y;
		}
		while ( y >= this->m_size.y - 1 ) {
			++location.y;
			y -= this->m_size.y - 1;
		}
		while ( z < 0 ) {
			--location.z;
			z += this->m_size.z;
		}
		while ( z >= this->m_size.z - 1 ) {
			++location.z;
			z -= this->m_size.z - 1;
		}
	*/
		ext::Region* region = terrain.at( location );
		if ( !region ) {
		//	std::cout << "Out of bounds Region("<< location.x << ", " << location.y << ", " << location.z <<") @ setLight( " << x << ", " << y << ", " << z << " ) " << std::endl;
			return;
		}
		region->getComponent<ext::TerrainGenerator>().setLight( x, y, z, light, update );
		return;
	}

	// set modified
	if ( this->m_terrain ) {
		const ext::Terrain& terrain = *((const ext::Terrain*)this->m_terrain);
		ext::Region* region = terrain.at( this->m_location );
		if ( region ) region->getComponent<uf::Serializer>()["region"]["modified"] = true;
	}

	std::size_t i = this->wrapPosition(x, y, z, this->m_voxels.lighting.swizzle);
//	std::cout << "Set Voxel( " << x << ", " << y << ", " << z << ") ("<< i <<") to level " << light << std::endl;
	this->m_voxels.lighting.raw[i] = light;
	this->wrapLight();
}
void ext::TerrainGenerator::setLight( const pod::Vector3i& location, const ext::TerrainVoxel::light_t& light, bool update ){
	return this->setLight( location.x, location.y, location.z, light, update );
}

void ext::TerrainGenerator::light( int x, int y, int z, const ext::TerrainVoxel::light_t& targetLight ){
	// reset any existing lighting data
/*
	for ( uint i = 0; i < this->m_size.x * this->m_size.y * this->m_size.z; ++i ) {
		auto v = this->unwrapIndex( i, this->m_voxels.id.swizzle );
		this->setLight(v.x, v.y, v.z, 0x90FF );
	}
	this->writeToFile();
	return;
*/

	this->setLight( x,  y,  z, targetLight );

	int lightingMode = 3;
	// REDGREEENBLUE
	if ( lightingMode == 1 ) {
		uint8_t steps = 1;
		struct bfsNode {
			pod::Vector3i position;
			ext::Region* region;
		};
		std::queue<bfsNode> bfs;
		const ext::Terrain& terrain = *((const ext::Terrain*)this->m_terrain);

		{
			bfsNode root;
			root.position = pod::Vector3i{x, y, z},
			root.region = terrain.at( this->m_location );
			bfs.emplace(root);
		}

		size_t i = 0;
		while ( !bfs.empty() ) {
			bfsNode node = bfs.front(); bfs.pop();
			std::cout << "Calls: " << (++i) << " | Nodes: " << bfs.size() << std::endl; 
			if ( !node.region ) continue;
			auto& self = node.region->getComponent<ext::TerrainGenerator>();
			ext::TerrainVoxel voxel = ext::TerrainVoxel::atlas( self.getVoxel( node.position ) );
		//	ext::TerrainVoxel::light_t light = (self.getLight( node.position ) >> 12) & 0xF;
			struct LIGHT {
				ext::TerrainVoxel::light_t r, g, b;
			} light;
			light.r = (self.getLight( node.position ) >> 12) & 0xF;
			light.g = (self.getLight( node.position ) >>  8) & 0xF;
			light.b = (self.getLight( node.position ) >>  4) & 0xF;

			std::vector<pod::Vector3i> positions = {
				node.position + pod::Vector3i{-1, 0, 0 },
				node.position + pod::Vector3i{ 1, 0, 0 },
				node.position + pod::Vector3i{ 0,-1, 0 },
				node.position + pod::Vector3i{ 0, 1, 0 },
				node.position + pod::Vector3i{ 0, 0,-1 },
				node.position + pod::Vector3i{ 0, 0, 1 },
			};
			
			for ( auto& position : positions ) {
				{
					pod::Vector3i location = self.m_location;
					if ( position.x < 0 ) {
						position.x += self.m_size.x;
						--location.x;
					} else if ( position.x >= self.m_size.x ) {
						position.x -= self.m_size.x;
						++location.x;
					}
					if ( position.y < 0 ) {
						position.y += self.m_size.y;
						--location.y;
					} else if ( position.y >= self.m_size.y ) {
						position.y -= self.m_size.y;
						++location.y;
					}
					if ( position.z < 0 ) {
						position.z += self.m_size.z;
						--location.z;
					} else if ( position.z >= self.m_size.z ) {
						position.z -= self.m_size.z;
						++location.z;
					}
				/*
					while ( position.x < 0 ) {
						--location.x;
						position.x += self.m_size.x;
					}
					while ( position.x >= self.m_size.x - 1 ) {
						++location.x;
						position.x -= self.m_size.x;
					}
					while ( position.y < 0 ) {
						--location.y;
						position.y += self.m_size.y;
					}
					while ( position.y >= self.m_size.y - 1 ) {
						++location.y;
						position.y -= self.m_size.y;
					}
					while ( position.z < 0 ) {
						--location.z;
						position.z += self.m_size.z;
					}
					while ( position.z >= self.m_size.z - 1 ) {
						++location.z;
						position.z -= self.m_size.z;
					}
				*/
					node.region = terrain.at( location );
				}
				if ( !node.region ) continue;
				auto& selfGenerator = node.region->getComponent<ext::TerrainGenerator>();
				
				struct {
					ext::TerrainVoxel voxel;
					ext::TerrainVoxel::light_t light;
				} neighbor;

				neighbor.voxel = ext::TerrainVoxel::atlas( selfGenerator.getVoxel( position ) );
				neighbor.light = selfGenerator.getLight( position );
				
			//	ext::TerrainVoxel::light_t targetLight = (neighbor.light >> 12) & 0xF;
				LIGHT targetLight;
				targetLight.r = (neighbor.light >> 12) & 0xF;
				targetLight.g = (neighbor.light >>  8) & 0xF;
				targetLight.b = (neighbor.light >>  4) & 0xF;

				if ( neighbor.voxel.opaque() ) continue;
				if ( targetLight.r + (2 * steps) <= light.r ) {
					selfGenerator.setLight( position, (neighbor.light & 0x0FFF ) | ((light.r - (1 * steps)) << 12), false );
					node.position = position;
					bfs.emplace( node );
				}
				if ( targetLight.g + (2 * steps) <= light.g ) {
					selfGenerator.setLight( position, (neighbor.light & 0xF0FF ) | ((light.g - (1 * steps)) <<  8), false );
					node.position = position;
					bfs.emplace( node );
				}
				if ( targetLight.b + (2 * steps) <= light.b ) {
					selfGenerator.setLight( position, (neighbor.light & 0xFF0F ) | ((light.b - (1 * steps)) <<  4), false );
					node.position = position;
					bfs.emplace( node );
				}
			}
		}
	}
	// REDGREENBLUE
	if ( lightingMode == 2 ) {
		uint8_t steps = 1;
		std::queue<pod::Vector3i> bfs;
		bfs.emplace( pod::Vector3i{x, y, z});

		while ( !bfs.empty() ) {
			pod::Vector3i node = bfs.front(); bfs.pop();
			ext::TerrainVoxel voxel = ext::TerrainVoxel::atlas( this->getVoxel( node ) );
			struct LIGHT {
				ext::TerrainVoxel::light_t r, g, b;
			} light;
			light.r = (this->getLight( node ) >> 12) & 0xF;
			light.g = (this->getLight( node ) >>  8) & 0xF;
			light.b = (this->getLight( node ) >>  4) & 0xF;

			std::vector<pod::Vector3i> positions = {
				node + pod::Vector3i{-1, 0, 0 },
				node + pod::Vector3i{ 1, 0, 0 },
				node + pod::Vector3i{ 0,-1, 0 },
				node + pod::Vector3i{ 0, 1, 0 },
				node + pod::Vector3i{ 0, 0,-1 },
				node + pod::Vector3i{ 0, 0, 1 },
			};
			
			for ( auto& position : positions ) {
				struct {
					ext::TerrainVoxel voxel;
					ext::TerrainVoxel::light_t light;
				} neighbor;

				neighbor.voxel = ext::TerrainVoxel::atlas( this->getVoxel( position ) );
				neighbor.light = this->getLight( position );
				
			//	ext::TerrainVoxel::light_t targetLight = (neighbor.light >> 12) & 0xF;
				LIGHT targetLight;
				 targetLight.r = (neighbor.light >> 12) & 0xF;
				 targetLight.g = (neighbor.light >>  8) & 0xF;
				 targetLight.b = (neighbor.light >>  4) & 0xF;

				if ( neighbor.voxel.opaque() ) continue;
				if ( targetLight.r + (2 * steps) <= light.r ) {
					this->setLight( position, (neighbor.light & 0x0FFF ) | ((light.r - (1 * steps)) << 12), false );
					bfs.emplace( position );
				}
				if ( targetLight.g + (2 * steps) <= light.g ) {
					this->setLight( position, (neighbor.light & 0xF0FF ) | ((light.g - (1 * steps)) <<  8), false );
					bfs.emplace( position );
				}
				if ( targetLight.b + (2 * steps) <= light.b ) {
					this->setLight( position, (neighbor.light & 0xFF0F ) | ((light.b - (1 * steps)) <<  4), false );
					bfs.emplace( position );
				}
			}
		}
	}
	// RED
	if ( lightingMode == 3 ) {
		uint8_t steps = 1;
		std::queue<pod::Vector3i> bfs;
		bfs.emplace( pod::Vector3i{x, y, z});

		while ( !bfs.empty() ) {
			pod::Vector3i node = bfs.front(); bfs.pop();
			ext::TerrainVoxel voxel = ext::TerrainVoxel::atlas( this->getVoxel( node ) );
			ext::TerrainVoxel::light_t light = (this->getLight( node ) >> 12) & 0xF;

			std::vector<pod::Vector3i> positions = {
				node + pod::Vector3i{-1, 0, 0 },
				node + pod::Vector3i{ 1, 0, 0 },
				node + pod::Vector3i{ 0,-1, 0 },
				node + pod::Vector3i{ 0, 1, 0 },
				node + pod::Vector3i{ 0, 0,-1 },
				node + pod::Vector3i{ 0, 0, 1 },
			};
			
			for ( auto& position : positions ) {
				struct {
					ext::TerrainVoxel voxel;
					ext::TerrainVoxel::light_t light;
				} neighbor;
				neighbor.voxel = ext::TerrainVoxel::atlas( this->getVoxel( position ) );
				neighbor.light = this->getLight( position );
				
				ext::TerrainVoxel::light_t targetLight = (neighbor.light >> 12) & 0xF;

				if ( neighbor.voxel.opaque() ) continue;
				if ( targetLight + (2 * steps) <= light ) {
					this->setLight( position, (neighbor.light & 0x0FFF ) | ((light - (1 * steps)) << 12), false );
					bfs.emplace( position );
				}
			}
		}
	}
	// GREEN
	if ( lightingMode == 3 ) {
		uint8_t steps = 1;
		std::queue<pod::Vector3i> bfs;
		bfs.emplace( pod::Vector3i{x, y, z});

		while ( !bfs.empty() ) {
			pod::Vector3i node = bfs.front(); bfs.pop();
			ext::TerrainVoxel voxel = ext::TerrainVoxel::atlas( this->getVoxel( node ) );
			ext::TerrainVoxel::light_t light = (this->getLight( node ) >> 8) & 0xF;

			std::vector<pod::Vector3i> positions = {
				node + pod::Vector3i{-1, 0, 0 },
				node + pod::Vector3i{ 1, 0, 0 },
				node + pod::Vector3i{ 0,-1, 0 },
				node + pod::Vector3i{ 0, 1, 0 },
				node + pod::Vector3i{ 0, 0,-1 },
				node + pod::Vector3i{ 0, 0, 1 },
			};
			
			for ( auto& position : positions ) {
				struct {
					ext::TerrainVoxel voxel;
					ext::TerrainVoxel::light_t light;
				} neighbor;
				neighbor.voxel = ext::TerrainVoxel::atlas( this->getVoxel( position ) );
				neighbor.light = this->getLight( position );
				
				ext::TerrainVoxel::light_t targetLight = (neighbor.light >> 8) & 0xF;

				if ( neighbor.voxel.opaque() ) continue;
				if ( targetLight + (2 * steps) <= light ) {
					this->setLight( position, (neighbor.light & 0xF0FF ) | ((light - (1 * steps)) << 8), false );
					bfs.emplace( position );
				}
			}
		}
	}
	// BLUE
	if ( lightingMode == 3 ) {
		uint8_t steps = 1;
		std::queue<pod::Vector3i> bfs;
		bfs.emplace( pod::Vector3i{x, y, z});

		while ( !bfs.empty() ) {
			pod::Vector3i node = bfs.front(); bfs.pop();
			ext::TerrainVoxel voxel = ext::TerrainVoxel::atlas( this->getVoxel( node ) );
			ext::TerrainVoxel::light_t light = (this->getLight( node ) >> 4) & 0xF;

			std::vector<pod::Vector3i> positions = {
				node + pod::Vector3i{-1, 0, 0 },
				node + pod::Vector3i{ 1, 0, 0 },
				node + pod::Vector3i{ 0,-1, 0 },
				node + pod::Vector3i{ 0, 1, 0 },
				node + pod::Vector3i{ 0, 0,-1 },
				node + pod::Vector3i{ 0, 0, 1 },
			};
			
			for ( auto& position : positions ) {
				struct {
					ext::TerrainVoxel voxel;
					ext::TerrainVoxel::light_t light;
				} neighbor;
				neighbor.voxel = ext::TerrainVoxel::atlas( this->getVoxel( position ) );
				neighbor.light = this->getLight( position );
				
				ext::TerrainVoxel::light_t targetLight = (neighbor.light >> 4) & 0xF;

				if ( neighbor.voxel.opaque() ) continue;
				if ( targetLight + (2 * steps) <= light ) {
					this->setLight( position, (neighbor.light & 0xFF0F ) | ((light - (1 * steps)) << 4), false );
					bfs.emplace( position );
				}
			}
		}
	}

//	this->writeToFile();
}
void ext::TerrainGenerator::light( const pod::Vector3i& location, const ext::TerrainVoxel::light_t& targetLight ){
	return this->light( location.x, location.y, location.z, targetLight );
}
void ext::TerrainGenerator::updateLight(){
//	this->unwrapLight();
	// set lights to original values
/*	
	std::size_t i = 0;
	std::cout << "Updating lights for " << this->m_location.x << ", " << this->m_location.y << ", " << this->m_location.z << std::endl;
	for ( auto& _ : this->m_voxels.id.rle ) {
		for ( std::size_t __ = 0; __ < _.length; ++__ ) {
			ext::TerrainVoxel voxel = ext::TerrainVoxel::atlas( _.value );
			auto v = this->unwrapIndex( i++, this->m_voxels.id.swizzle );
			// this->setLight(v.x, v.y, v.z, voxel.light() );
			this->m_voxels.lighting.raw[i] = AMBIENT_LIGHT;
		}
	}
	//this->wrapLight();
*/
	// set lights

	std::size_t i = 0;
	for ( auto& _ : this->m_voxels.id.rle ) {
		for ( std::size_t __ = 0; __ < _.length; ++__ ) {
			ext::TerrainVoxel voxel = ext::TerrainVoxel::atlas( _.value );
			auto v = this->unwrapIndex( i++, this->m_voxels.id.swizzle );
			if ( voxel.light() == 0x0000 ) continue;
			this->light(v.x, v.y, v.z, voxel.light() );
		}
	}
//	this->wrapLight();
//	this->writeToFile();
}
void ext::TerrainGenerator::rasterize( std::vector<ext::TerrainGenerator::mesh_t::vertex_t>& vertices, const ext::Region& region ){
	if ( this->m_voxels.id.rle.empty() ) return;

	this->writeToFile();
				
	struct { struct { float x, y, z; } position; struct { float u, v, x, y; } uv; } offset;

	const ext::Terrain& terrain = region.getParent<ext::Terrain>();
	const pod::Transform<>& transform = region.getComponent<pod::Transform<>>();
	const pod::Vector3i location = {
		transform.position.x / this->m_size.x,
		transform.position.y / this->m_size.y,
		transform.position.z / this->m_size.z,
	};

	struct { 
		ext::Region* right = NULL;
		ext::Region* left = NULL;
		ext::Region* top = NULL;
		ext::Region* bottom = NULL;
		ext::Region* front = NULL;
		ext::Region* back = NULL;
	} regions;

	regions.right = terrain.at( { location.x + 1, location.y, location.z } );
	regions.left = terrain.at( { location.x - 1, location.y, location.z } );
	regions.top = terrain.at( { location.x, location.y + 1, location.z } );
	regions.bottom = terrain.at( { location.x, location.y - 1, location.z } );
	regions.front = terrain.at( { location.x, location.y, location.z + 1 } );
	regions.back = terrain.at( { location.x, location.y, location.z - 1 } );

	ext::TerrainVoxel::light_t ambientLight = AMBIENT_LIGHT; {
		const uf::Serializer& tMetadata = region.getComponent<uf::Serializer>();
		COLOR color = uint16ToColor( ambientLight );
		color.r *= tMetadata["region"]["light"]["ambient"][0].asFloat();
		color.g *= tMetadata["region"]["light"]["ambient"][1].asFloat();
		color.b *= tMetadata["region"]["light"]["ambient"][2].asFloat();
		ambientLight = colorToUint16( color );
	}

	#define TERRAIN_SHOULD_RENDER_FACE(SIDE)\
		should.SIDE = !neighbor.SIDE.opaque();\
		if ( should.SIDE ) {\
			for ( uint i = 0; i < model.position.SIDE.size() / 3; ++i ) {\
				ext::TerrainGenerator::mesh_t::vertex_t vertex;\
				{\
					pod::Vector3f& p = vertex.position;\
					p.x = model.position.SIDE[i*3+0]; p.y = model.position.SIDE[i*3+1]; p.z = model.position.SIDE[i*3+2];\
					p.x += offset.position.x; p.y += offset.position.y; p.z += offset.position.z;\
				}\
				{\
					pod::Vector2f& p = vertex.uv;\
					p.x = model.uv.SIDE[i*2+0]; p.y = model.uv.SIDE[i*2+1];\
					p.x *= offset.uv.x; p.y *= offset.uv.y;\
					p.x += offset.uv.u; p.y += offset.uv.v;\
				}\
				{\
					pod::Vector3f& p = vertex.normal;\
					p.x = model.normal.SIDE[i*3+0]; p.y = model.normal.SIDE[i*3+1]; p.z = model.normal.SIDE[i*3+2];\
				}\
				{\
					pod::Vector4t<uint8_t>& p = vertex.color;\
					ext::TerrainVoxel::light_t l = light.SIDE;\
					p[0] = (l >> 12) & 0xF;\
					p[1] = (l >>  8) & 0xF;\
					p[2] = (l >>  4) & 0xF;\
					p[3] = (l      ) & 0xF;\
					p[0] = (p[0] << 4) | p[0];\
					p[1] = (p[1] << 4) | p[1];\
					p[2] = (p[2] << 4) | p[2];\
					p[3] = (p[3] << 4) | p[3];\
				}\
				vertices.push_back(vertex);\
			}\
		}

/*
	for ( uint y = 0; y < this->m_size.y; ++y ) {
		for ( uint z = 0; z < this->m_size.z; ++z ) {
			for ( uint x = 0; x < this->m_size.x; ++x ) {
				ext::TerrainVoxel voxel = ext::TerrainVoxel::atlas( this->getVoxel( x, y, z ) );
*/
	std::size_t i = 0;
	for ( auto& _ : this->m_voxels.id.rle ) {
		for ( std::size_t __ = 0; __ < _.length; ++__ ) {
			{
				int x = 0, y = 0, z = 0;
				{
					auto v = this->unwrapIndex( i++, this->m_voxels.id.swizzle );
					x = v.x;
					y = v.y;
					z = v.z;
				}

				ext::TerrainVoxel voxel = ext::TerrainVoxel::atlas( _.value );

				offset.position.x = x - (this->m_size.x / 2.0f);
				offset.position.y = y - (this->m_size.y / 2.0f);
				offset.position.z = z - (this->m_size.z / 2.0f);

				const ext::TerrainVoxel::Model& model = voxel.model();

				// clear block, don't render
				if ( !voxel.opaque() ) continue;

				offset.position.x += transform.position.x;
				offset.position.y += transform.position.y;
				offset.position.z += transform.position.z;
			
				offset.uv.x = 1.0f / ext::TerrainVoxel::size().x;
				offset.uv.y = 1.0f / ext::TerrainVoxel::size().y;

				offset.uv.u = voxel.uv().x * offset.uv.x;
				offset.uv.v = (ext::TerrainVoxel::size().y - 1 - voxel.uv().y) * offset.uv.y;

				struct {
					bool top = true, bottom = true, left = true, right = true, front = true, back = true;
				} should;
				struct {
					ext::TerrainVoxel top = ext::TerrainVoxel::atlas(0), bottom = ext::TerrainVoxel::atlas(0), left = ext::TerrainVoxel::atlas(0), right = ext::TerrainVoxel::atlas(0), front = ext::TerrainVoxel::atlas(0), back = ext::TerrainVoxel::atlas(0);
				} neighbor;
				struct {
					ext::TerrainVoxel::light_t top = 0xffff, bottom = 0xfff, left = 0xffff, right = 0xffff, front = 0xffff, back = 0xffff;
				} light;
			/*
				if ( x > 0 ) neighbor.left = ext::TerrainVoxel::atlas(this->getVoxel(x-1,y,z));
				else if ( regions.left != NULL ) neighbor.left = ext::TerrainVoxel::atlas(regions.left->getComponent<ext::TerrainGenerator>().getVoxel(this->m_size.x-1,y,z));
				if ( x + 1 < this->m_size.x ) neighbor.right = ext::TerrainVoxel::atlas(this->getVoxel(x+1,y,z));
				else if ( regions.right != NULL ) neighbor.right = ext::TerrainVoxel::atlas(regions.right->getComponent<ext::TerrainGenerator>().getVoxel(0,y,z));
				if ( y > 0 ) neighbor.bottom = ext::TerrainVoxel::atlas(this->getVoxel(x,y-1,z));
				else if ( regions.bottom != NULL ) neighbor.bottom = ext::TerrainVoxel::atlas(regions.bottom->getComponent<ext::TerrainGenerator>().getVoxel(x,this->m_size.y-1,z));
				if ( y + 1 < this->m_size.y ) neighbor.top = ext::TerrainVoxel::atlas(this->getVoxel(x,y+1,z));
				else if ( regions.top != NULL ) neighbor.top = ext::TerrainVoxel::atlas(regions.top->getComponent<ext::TerrainGenerator>().getVoxel(x,0,z));
				if ( z > 0 ) neighbor.back = ext::TerrainVoxel::atlas(this->getVoxel(x,y,z-1));
				else if ( regions.back != NULL ) neighbor.back = ext::TerrainVoxel::atlas(regions.back->getComponent<ext::TerrainGenerator>().getVoxel(x,y,this->m_size.z-1));
				if ( z + 1 < this->m_size.z ) neighbor.front = ext::TerrainVoxel::atlas(this->getVoxel(x,y,z+1));
				else if ( regions.front != NULL ) neighbor.front = ext::TerrainVoxel::atlas(regions.front->getComponent<ext::TerrainGenerator>().getVoxel(x,y,0));
			*/
			
				neighbor.left = ext::TerrainVoxel::atlas(this->getVoxel(x-1,y,z));
				light.left = this->getLight(x-1,y,z);

				neighbor.bottom = ext::TerrainVoxel::atlas(this->getVoxel(x,y-1,z));
				light.bottom = this->getLight(x,y-1,z);

				neighbor.back = ext::TerrainVoxel::atlas(this->getVoxel(x,y,z-1));
				light.back = this->getLight(x,y,z-1);

				neighbor.right = ext::TerrainVoxel::atlas(this->getVoxel(x+1,y,z));
				light.right = this->getLight(x+1,y,z);

				neighbor.top = ext::TerrainVoxel::atlas(this->getVoxel(x,y+1,z));
				light.top = this->getLight(x,y+1,z);

				neighbor.front = ext::TerrainVoxel::atlas(this->getVoxel(x,y,z+1));
				light.front = this->getLight(x,y,z+1);

				TERRAIN_SHOULD_RENDER_FACE(right)
				TERRAIN_SHOULD_RENDER_FACE(left)
				TERRAIN_SHOULD_RENDER_FACE(top)
				TERRAIN_SHOULD_RENDER_FACE(bottom)
				TERRAIN_SHOULD_RENDER_FACE(front)
				TERRAIN_SHOULD_RENDER_FACE(back)
			/*
				should.right = !neighbor.right.opaque();
				should.left = !neighbor.left.opaque();
				should.top = !neighbor.top.opaque();
				should.bottom = !neighbor.bottom.opaque();
				should.front = !neighbor.front.opaque();
				should.back = !neighbor.back.opaque();

				if ( should.right ) {
					for ( uint i = 0; i < model.position.right.size() / 3; ++i ) {
						ext::TerrainGenerator::mesh_t::vertex_t vertex;
						{
							pod::Vector3f& p = vertex.position;
							p.x = model.position.right[i*3+0]; p.y = model.position.right[i*3+1]; p.z = model.position.right[i*3+2];
							p.x += offset.position.x; p.y += offset.position.y; p.z += offset.position.z;
						}
						{
							pod::Vector2f& p = vertex.uv;
							p.x = model.uv.right[i*2+0]; p.y = model.uv.right[i*2+1];
							p.x *= offset.uv.x; p.y *= offset.uv.y;
							p.x += offset.uv.u; p.y += offset.uv.v;
						}
						{
							pod::Vector3f& p = vertex.normal;
							p.x = model.normal.right[i*3+0]; p.y = model.normal.right[i*3+1]; p.z = model.normal.right[i*3+2];
						}
						{
							pod::Vector4t<uint8_t>& p = vertex.color;
							
							ext::TerrainVoxel::light_t l = light.right;

							p[0] = (l >> 12) & 0xF; // A/R
							p[1] = (l >>  8) & 0xF; // B/G
							p[2] = (l >>  4) & 0xF; // G/B
							p[3] = (l      ) & 0xF; // R/A

							p[0] = (p[0] << 4) | p[0];
							p[1] = (p[1] << 4) | p[1];
							p[2] = (p[2] << 4) | p[2];
							p[3] = (p[3] << 4) | p[3];
						}
						vertices.push_back(vertex);
					}
				}
				if ( should.left ) {
					for ( uint i = 0; i < model.position.left.size() / 3; ++i ) {
						ext::TerrainGenerator::mesh_t::vertex_t vertex;
						{
							pod::Vector3f& p = vertex.position;
							p.x = model.position.left[i*3+0]; p.y = model.position.left[i*3+1]; p.z = model.position.left[i*3+2];
							p.x += offset.position.x; p.y += offset.position.y; p.z += offset.position.z;
						}
						{
							pod::Vector2f& p = vertex.uv;
							p.x = model.uv.left[i*2+0]; p.y = model.uv.left[i*2+1];
							p.x *= offset.uv.x; p.y *= offset.uv.y;
							p.x += offset.uv.u; p.y += offset.uv.v;
						}
						{
							pod::Vector3f& p = vertex.normal;
							p.x = model.normal.left[i*3+0]; p.y = model.normal.left[i*3+1]; p.z = model.normal.left[i*3+2];
						}
						{
							pod::Vector4t<uint8_t>& p = vertex.color;
							
							ext::TerrainVoxel::light_t l = light.left;

							p[0] = (l >> 12) & 0xF; // A/R
							p[1] = (l >>  8) & 0xF; // B/G
							p[2] = (l >>  4) & 0xF; // G/B
							p[3] = (l      ) & 0xF; // R/A

							p[0] = (p[0] << 4) | p[0];
							p[1] = (p[1] << 4) | p[1];
							p[2] = (p[2] << 4) | p[2];
							p[3] = (p[3] << 4) | p[3];

						//	if ( p[0] > 0 ) std::cout << "LIGHT: " << std::hex << *((uint32_t*) &p[0]) << std::endl;
						//	else std::cout << "BLACK: " << std::hex << (uint32_t) light.left << std::endl;
						}
						vertices.push_back(vertex);
					}
				}
				if ( should.top ) {
					for ( uint i = 0; i < model.position.top.size() / 3; ++i ) {
						ext::TerrainGenerator::mesh_t::vertex_t vertex;
						{
							pod::Vector3f& p = vertex.position;
							p.x = model.position.top[i*3+0]; p.y = model.position.top[i*3+1]; p.z = model.position.top[i*3+2];
							p.x += offset.position.x; p.y += offset.position.y; p.z += offset.position.z;
						}
						{
							pod::Vector2f& p = vertex.uv;
							p.x = model.uv.top[i*2+0]; p.y = model.uv.top[i*2+1];
							p.x *= offset.uv.x; p.y *= offset.uv.y;
							p.x += offset.uv.u; p.y += offset.uv.v;
						}
						{
							pod::Vector3f& p = vertex.normal;
							p.x = model.normal.top[i*3+0]; p.y = model.normal.top[i*3+1]; p.z = model.normal.top[i*3+2];
						}
						{
							pod::Vector4t<uint8_t>& p = vertex.color;
							
							ext::TerrainVoxel::light_t l = light.top;

							p[0] = (l >> 12) & 0xF; // A/R
							p[1] = (l >>  8) & 0xF; // B/G
							p[2] = (l >>  4) & 0xF; // G/B
							p[3] = (l      ) & 0xF; // R/A

							p[0] = (p[0] << 4) | p[0];
							p[1] = (p[1] << 4) | p[1];
							p[2] = (p[2] << 4) | p[2];
							p[3] = (p[3] << 4) | p[3];

						//	if ( p[0] > 0 ) std::cout << "LIGHT: " << std::hex << *((uint32_t*) &p[0]) << std::endl;
						//	else std::cout << "BLACK: " << std::hex << (uint32_t) light.top << std::endl;

						}
						vertices.push_back(vertex);
					}
				}
				if ( should.bottom ) {
					for ( uint i = 0; i < model.position.bottom.size() / 3; ++i ) {
						ext::TerrainGenerator::mesh_t::vertex_t vertex;
						{
							pod::Vector3f& p = vertex.position;
							p.x = model.position.bottom[i*3+0]; p.y = model.position.bottom[i*3+1]; p.z = model.position.bottom[i*3+2];
							p.x += offset.position.x; p.y += offset.position.y; p.z += offset.position.z;
						}
						{
							pod::Vector2f& p = vertex.uv;
							p.x = model.uv.bottom[i*2+0]; p.y = model.uv.bottom[i*2+1];
							p.x *= offset.uv.x; p.y *= offset.uv.y;
							p.x += offset.uv.u; p.y += offset.uv.v;
						}
						{
							pod::Vector3f& p = vertex.normal;
							p.x = model.normal.bottom[i*3+0]; p.y = model.normal.bottom[i*3+1]; p.z = model.normal.bottom[i*3+2];
						}
						{
							pod::Vector4t<uint8_t>& p = vertex.color;
							
							ext::TerrainVoxel::light_t l = light.bottom;

							p[0] = (l >> 12) & 0xF; // A/R
							p[1] = (l >>  8) & 0xF; // B/G
							p[2] = (l >>  4) & 0xF; // G/B
							p[3] = (l      ) & 0xF; // R/A

							p[0] = (p[0] << 4) | p[0];
							p[1] = (p[1] << 4) | p[1];
							p[2] = (p[2] << 4) | p[2];
							p[3] = (p[3] << 4) | p[3];

						//	if ( p[0] > 0 ) std::cout << "LIGHT: " << std::hex << *((uint32_t*) &p[0]) << std::endl;
						//	else std::cout << "BLACK: " << std::hex << (uint32_t) light.bottom << std::endl;

						}
						vertices.push_back(vertex);
					}
				}
				if ( should.front ) {
					for ( uint i = 0; i < model.position.front.size() / 3; ++i ) {
						ext::TerrainGenerator::mesh_t::vertex_t vertex;
						{
							pod::Vector3f& p = vertex.position;
							p.x = model.position.front[i*3+0]; p.y = model.position.front[i*3+1]; p.z = model.position.front[i*3+2];
							p.x += offset.position.x; p.y += offset.position.y; p.z += offset.position.z;
						}
						{
							pod::Vector2f& p = vertex.uv;
							p.x = model.uv.front[i*2+0]; p.y = model.uv.front[i*2+1];
							p.x *= offset.uv.x; p.y *= offset.uv.y;
							p.x += offset.uv.u; p.y += offset.uv.v;
						}
						{
							pod::Vector3f& p = vertex.normal;
							p.x = model.normal.front[i*3+0]; p.y = model.normal.front[i*3+1]; p.z = model.normal.front[i*3+2];
						}
						{
							pod::Vector4t<uint8_t>& p = vertex.color;
							
							ext::TerrainVoxel::light_t l = light.front;

							p[0] = (l >> 12) & 0xF; // A/R
							p[1] = (l >>  8) & 0xF; // B/G
							p[2] = (l >>  4) & 0xF; // G/B
							p[3] = (l      ) & 0xF; // R/A

							p[0] = (p[0] << 4) | p[0];
							p[1] = (p[1] << 4) | p[1];
							p[2] = (p[2] << 4) | p[2];
							p[3] = (p[3] << 4) | p[3];

						//	if ( p[0] > 0 ) std::cout << "LIGHT: " << std::hex << *((uint32_t*) &p[0]) << std::endl;
						//	else std::cout << "BLACK: " << std::hex << (uint32_t) light.front << std::endl;

						}
						vertices.push_back(vertex);
					}
				}
				if ( should.back ) {
					for ( uint i = 0; i < model.position.back.size() / 3; ++i ) {
						ext::TerrainGenerator::mesh_t::vertex_t vertex;
						{
							pod::Vector3f& p = vertex.position;
							p.x = model.position.back[i*3+0]; p.y = model.position.back[i*3+1]; p.z = model.position.back[i*3+2];
							p.x += offset.position.x; p.y += offset.position.y; p.z += offset.position.z;
						}
						{
							pod::Vector2f& p = vertex.uv;
							p.x = model.uv.back[i*2+0]; p.y = model.uv.back[i*2+1];
							p.x *= offset.uv.x; p.y *= offset.uv.y;
							p.x += offset.uv.u; p.y += offset.uv.v;
						}
						{
							pod::Vector3f& p = vertex.normal;
							p.x = model.normal.back[i*3+0]; p.y = model.normal.back[i*3+1]; p.z = model.normal.back[i*3+2];
						}
						{
							pod::Vector4t<uint8_t>& p = vertex.color;
							
							ext::TerrainVoxel::light_t l = light.back;

							p[0] = (l >> 12) & 0xF; // A/R
							p[1] = (l >>  8) & 0xF; // B/G
							p[2] = (l >>  4) & 0xF; // G/B
							p[3] = (l      ) & 0xF; // R/A

							p[0] = (p[0] << 4) | p[0];
							p[1] = (p[1] << 4) | p[1];
							p[2] = (p[2] << 4) | p[2];
							p[3] = (p[3] << 4) | p[3];

						//	if ( p[0] > 0 ) std::cout << "LIGHT: " << std::hex << *((uint32_t*) &p[0]) << std::endl;
						//	else std::cout << "BLACK: " << std::hex << (uint32_t) light.back << std::endl;

						}
						vertices.push_back(vertex);
					}
				}
		*/
			}
		}
	}
}

void ext::TerrainGenerator::vectorize( std::vector<ext::vulkan::ComputeGraphic::Cube>& buffer, const ext::Region& region ) {
	struct {
		pod::Vector3f position;
		struct { float u, v, x, y; } uv;
	} offset;

	const ext::Terrain& terrain = region.getParent<ext::Terrain>();
	const pod::Transform<>& transform = region.getComponent<pod::Transform<>>();
	const pod::Vector3i location = {
		transform.position.x / this->m_size.x,
		transform.position.y / this->m_size.y,
		transform.position.z / this->m_size.z,
	};

	struct { ext::Region* right = NULL; ext::Region* left = NULL; ext::Region* top = NULL; ext::Region* bottom = NULL; ext::Region* front = NULL; ext::Region* back = NULL; } regions;

	regions.right = terrain.at( { location.x + 1, location.y, location.z } );
	regions.left = terrain.at( { location.x - 1, location.y, location.z } );
	regions.top = terrain.at( { location.x, location.y + 1, location.z } );
	regions.bottom = terrain.at( { location.x, location.y - 1, location.z } );
	regions.front = terrain.at( { location.x, location.y, location.z + 1 } );
	regions.back = terrain.at( { location.x, location.y, location.z - 1 } );
/*
	for ( uint x = 0; x < this->m_size.x; ++x ) {
		for ( uint y = 0; y < this->m_size.y; ++y ) {
			for ( uint z = 0; z < this->m_size.z; ++z ) {
				ext::TerrainVoxel voxel = ext::TerrainVoxel::atlas(this->m_voxels.id.raw[x][y][z]);
*/
	std::size_t i = 0;
	for ( auto& _ : this->m_voxels.id.rle ) {
		for ( std::size_t __ = 0; __ < _.length; ++__ ) {
			{
				std::size_t x = 0, y = 0, z = 0;
				{
					auto v = this->unwrapIndex( i++, this->m_voxels.id.swizzle );
					x = v.x;
					y = v.y;
					z = v.z;
				}

				ext::TerrainVoxel voxel = ext::TerrainVoxel::atlas( _.value );
				offset.position.x = x - (this->m_size.x / 2.0f);
				offset.position.y = y - (this->m_size.y / 2.0f);
				offset.position.z = z - (this->m_size.z / 2.0f);

				const ext::TerrainVoxel::Model& model = voxel.model();
				if ( !voxel.opaque() ) continue;

				struct {
					bool top = true, bottom = true, left = true, right = true, front = true, back = true;
				} should;
				struct {
					ext::TerrainVoxel top = ext::TerrainVoxel::atlas(0), bottom = ext::TerrainVoxel::atlas(0), left = ext::TerrainVoxel::atlas(0), right = ext::TerrainVoxel::atlas(0), front = ext::TerrainVoxel::atlas(0), back = ext::TerrainVoxel::atlas(0);
				} neighbor;

				if ( x > 0 ) neighbor.left = ext::TerrainVoxel::atlas(this->getVoxel(x-1,y,z)); else if ( regions.left != NULL ) {
					const ext::Region& left = *regions.left;
					const ext::TerrainGenerator& t = left.getComponent<ext::TerrainGenerator>();
					neighbor.left = ext::TerrainVoxel::atlas(t.getVoxel(t.m_size.x-1,y,z));
				}
				if ( y > 0 ) neighbor.bottom = ext::TerrainVoxel::atlas(this->getVoxel(x,y-1,z)); else if ( regions.bottom != NULL ) {
					const ext::Region& bottom = *regions.bottom;
					const ext::TerrainGenerator& t = bottom.getComponent<ext::TerrainGenerator>();
					neighbor.bottom =ext::TerrainVoxel::atlas( t.getVoxel(x,t.m_size.y-1,z));
				}
				if ( z > 0 ) neighbor.back = ext::TerrainVoxel::atlas(this->getVoxel(x,y,z-1)); else if ( regions.back != NULL ) {
					const ext::Region& back = *regions.back;
					const ext::TerrainGenerator& t = back.getComponent<ext::TerrainGenerator>();
					neighbor.back = ext::TerrainVoxel::atlas(t.getVoxel(x,y,t.m_size.z-1));
				}
				
				if ( x + 1 < this->m_size.x ) neighbor.right = ext::TerrainVoxel::atlas(this->getVoxel(x+1,y,z)); else if ( regions.right != NULL ) {
					const ext::Region& right = *regions.right;
					const ext::TerrainGenerator& t = right.getComponent<ext::TerrainGenerator>();
					neighbor.right = ext::TerrainVoxel::atlas(t.getVoxel(0,y,z));
				}
				if ( y + 1 < this->m_size.y ) neighbor.top = ext::TerrainVoxel::atlas(this->getVoxel(x,y+1,z)); else if ( regions.top != NULL ) {
					const ext::Region& top = *regions.top;
					const ext::TerrainGenerator& t = top.getComponent<ext::TerrainGenerator>();
					neighbor.top = ext::TerrainVoxel::atlas(t.getVoxel(x,0,z));
				}
				if ( z + 1 < this->m_size.z ) neighbor.front = ext::TerrainVoxel::atlas(this->getVoxel(x,y,z+1)); else if ( regions.front != NULL ) {
					const ext::Region& front = *regions.front;
					const ext::TerrainGenerator& t = front.getComponent<ext::TerrainGenerator>();
					neighbor.front = ext::TerrainVoxel::atlas(t.getVoxel(x,y,0));
				}

				should.right = !neighbor.right.opaque();
				should.left = !neighbor.left.opaque();
				should.top = !neighbor.top.opaque();
				should.bottom = !neighbor.bottom.opaque();
				should.front = !neighbor.front.opaque();
				should.back = !neighbor.back.opaque();

				// occluded
				if ( !should.right && !should.left && !should.top && !should.bottom && !should.front && !should.back ) continue;
				if ( !voxel.opaque() ) continue;

				ext::vulkan::ComputeGraphic::Cube cube;
				pod::Vector3f location = offset.position + transform.position;
				cube.position.x = location.x;
				cube.position.y = location.y;
				cube.position.z = location.z;
				cube.position.z *= -1;
				cube.position.w = 0.5f;
				cube.type = voxel.opaque() ? pod::Primitive::CUBE : pod::Primitive::EMPTY;
				buffer.push_back(cube);
			}
		}
	}
}