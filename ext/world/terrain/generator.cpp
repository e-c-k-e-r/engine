#include "generator.h"

#include "../../ext.h"
#include "voxel.h"

#include <uf/engine/entity/entity.h>

#include <uf/gl/shader/shader.h>
#include <uf/gl/mesh/mesh.h>
#include <uf/gl/texture/texture.h>
#include <uf/gl/camera/camera.h>

#include <uf/utils/math/transform.h>
#include <uf/utils/math/physics.h>


void ext::TerrainGenerator::initialize( const pod::Vector3ui& size ){
	this->m_voxels = NULL;
	this->m_size = size;
}
void ext::TerrainGenerator::destroy(){ if ( !this->m_voxels ) return;
	for ( uint x = 0; x < this->m_size.x; ++x ) { 
		for ( uint y = 0; y < this->m_size.y; ++y ) delete[] this->m_voxels[x][y];
		delete[] this->m_voxels[x];
	}
	delete[] this->m_voxels;
	this->m_voxels = NULL;
}
void ext::TerrainGenerator::generate( ext::Region& region ){ if ( this->m_voxels ) return;
/*
	{
		struct {
			ext::TerrainVoxel::uid_t floor = ext::TerrainVoxelFloor().uid();
			ext::TerrainVoxel::uid_t wall = ext::TerrainVoxelWall().uid();
			ext::TerrainVoxel::uid_t ceiling = ext::TerrainVoxelCeiling().uid();
		} atlas;
		this->m_voxels = new ext::TerrainVoxel::uid_t**[this->m_size.x];
		for ( uint x = 0; x < this->m_size.x; ++x ) { this->m_voxels[x] = new ext::TerrainVoxel::uid_t*[this->m_size.y];
			for ( uint y = 0; y < this->m_size.y; ++y ) { this->m_voxels[x][y] = new ext::TerrainVoxel::uid_t[this->m_size.z];
				for ( uint z = 0; z < this->m_size.z; ++z ) { ext::TerrainVoxel::uid_t voxel = 0;	
					if ( x == 0 ) voxel = atlas.wall; if ( x == this->m_size.x - 1 ) voxel = atlas.wall;
					if ( z == 0 ) voxel = atlas.wall; if ( z == this->m_size.z - 1 ) voxel = atlas.wall;

					if ( y == 0 ) voxel = atlas.floor; if ( y == this->m_size.y - 1 ) voxel = atlas.ceiling;

					if ( y > 0 && y < this->m_size.y - 1 ) {
						if ( x > 4 && x < this->m_size.x - 4 ) voxel = 0;
						if ( z > 4 && z < this->m_size.z - 4 ) voxel = 0;
					}

					this->m_voxels[x][y][z] = voxel;
				}
			}
		}
	}
*/

	if ( this->m_size.x < 0 || this->m_size.y < 0 || this->m_size.z < 0 ) return;

	uint half_x = this->m_size.x / 2;
	uint half_y = this->m_size.y / 2;
	uint half_z = this->m_size.z / 2;

	const pod::Transform<>& transform = region.getComponent<pod::Transform<>>();
	const pod::Vector3i location = {
		transform.position.x / this->m_size.x,
		transform.position.y / this->m_size.y,
		transform.position.z / this->m_size.z,
	};

	struct {
		ext::TerrainVoxel::uid_t floor = ext::TerrainVoxelFloor().uid();// = uf::World::atlas.getFromString("Regular Floor").getUid();
		ext::TerrainVoxel::uid_t wall = ext::TerrainVoxelWall().uid();// = uf::World::atlas.getFromString("Regular Wall").getUid();
		ext::TerrainVoxel::uid_t ceiling = ext::TerrainVoxelCeiling().uid();// = uf::World::atlas.getFromString("Regular Ceiling").getUid();
		ext::TerrainVoxel::uid_t stair = ext::TerrainVoxelStair().uid();// = uf::World::atlas.getFromString("Regular Stair").getUid();
		ext::TerrainVoxel::uid_t pillar = ext::TerrainVoxelPillar().uid();// = uf::World::atlas.getFromString("Regular Pillar").getUid();
	} atlas;

	this->m_voxels = new ext::TerrainVoxel::uid_t**[this->m_size.x];
	for ( uint x = 0; x < this->m_size.x; ++x ) { this->m_voxels[x] = new ext::TerrainVoxel::uid_t*[this->m_size.y];
		for ( uint y = 0; y < this->m_size.y; ++y ) { this->m_voxels[x][y] = new ext::TerrainVoxel::uid_t[this->m_size.z];
			for ( uint z = 0; z < this->m_size.z; ++z ) this->m_voxels[x][y][z] = 0;
		}
	}
	if ( location.y <= 0 ) {
		int roomType = rand() % 6;
		if ( location.y == 0 ) roomType = rand() % 2;
		static bool first = false;
		if ( !first ) roomType = 0, first = true;
		for ( uint x = 0; x < this->m_size.x; x++ ) { 
		for ( uint y = 0; y < this->m_size.y; y++ ) { 
		for ( uint z = 0; z < this->m_size.z; z++ ) {
			if ( this->m_voxels == NULL || this->m_voxels[x] == NULL || this->m_voxels[x][y] == NULL ) break;
	// 	Wall
	/*
			if ((( x == 0 || x == 1 ) && ( z == this->m_size.z - 1 || z == this->m_size.z - 2 )) || (( z == 0 || z == 1 ) && ( x == this->m_size.x - 1 || x == this->m_size.x - 2 )))
				this->m_voxels[x][y][z] = atlas.wall;
	*/
	// 	Floor
			if ( y == 0 ) this->m_voxels[x][y][z] = atlas.floor;
	// 	Ceiling
		}
		}
		}

		switch ( roomType ) {
		// Wall-less room
			case 0: break;
		// 	Typical Room
			default:
			for ( uint x = 0; x < this->m_size.x; x++ ) 
			for ( uint y = 0; y < this->m_size.y; y++ ) 
			for ( uint z = 0; z < this->m_size.z; z++ ) if ( y + 1 == this->m_size.y ) this->m_voxels[x][y][z] = rand() % 10 < 8.0f ? 0 : atlas.ceiling;
		// 	North
				for ( uint y = 0; y < this->m_size.y; y++ ) {
					for ( uint x = 0; x < this->m_size.x; x++ ) {
						float randed_wall = rand() % 10;
						if ( !(y < 5 && x>half_x-3&&x<half_x+3) && randed_wall > 4.0f )
							this->m_voxels[x][y][this->m_size.z-1] = atlas.wall;
						else
							this->m_voxels[x][y][this->m_size.z-1] = 0;
					}
				}
		// 	South
				for ( uint y = 0; y < this->m_size.y; y++ ) {
					for ( uint x = 0; x < this->m_size.x; x++ ) {
						float randed_wall = rand() % 10;
						if ( !(y < 5 && x>half_x-3&&x<half_x+3) && randed_wall > 4.0f )
							this->m_voxels[x][y][0] = atlas.wall;
						else
							this->m_voxels[x][y][0] = 0;
					}
				}
		// 	East
				for ( uint y = 0; y < this->m_size.y; y++ ) {
					for ( uint z = 0; z < this->m_size.x; z++ ) {
						float randed_wall = rand() % 10;
						if ( !(y < 5 && z>half_z-3&&z<half_z+3) && randed_wall > 3.0f )
							this->m_voxels[this->m_size.z-1][y][z] = atlas.wall;
						else
							this->m_voxels[this->m_size.z-1][y][z] = 0;
					}
				}
				for ( uint x = 0; x < this->m_size.x; x++ ) { 
				for ( uint z = 0; z < this->m_size.z; z++ ) {
					this->m_voxels[x][0][z] = atlas.floor;
				}
				}
		// 	Default Room
		// 	Hole room
			case 1: 
				for ( uint x = half_x - (half_x / 2); x < half_x + (half_x / 2) + 1; x++ )
				for ( uint z = half_z - (half_z / 2); z < half_z + (half_z / 2) + 1; z++ )
					if ( rand() % 10 > 1.25f ) this->m_voxels[x][0][z] = 0;
			break;
		// 	Pillar
			case 3:
				for ( uint x = half_x - 1; x <= half_x + 1; x++ )
				for ( uint z = half_z - 1; z <= half_z + 1; z++ )
				for ( uint y = 0; y < this->m_size.y; y++ )
					if ( rand() % 10 > 1.25f ) this->m_voxels[x][y][z] = atlas.pillar;
			break; 
		// 	Stair room
			case 4: 
				int randed_direction = rand() % 2;
				int start_x = std::max(half_x - 3, (uint) 0);
				int stop_x = std::min(half_x + 5, this->m_size.x);
				int start_z = -2;
				int stop_z = 2;
				int slope_x = this->m_size.y / ( stop_x - start_x );
				int slope_z = this->m_size.y / ( stop_z - start_z );
			/*	
				for ( uint x = start_x; x <= stop_x; x++ )
				for ( uint z = start_z; z <= stop_z; z++ ) this->m_voxels[x][this->m_size.y - 1][half_z - 1 + z] = 0;
			*/
				switch ( randed_direction ) {
				// Left to Right
					case 0:
						for ( uint x = start_x; x <= stop_x; x++ ) {
							int y = (x - start_x) * slope_x + 1;
							for ( uint z = start_z; z <= stop_z && y < this->m_size.y; z++ )
							for ( int i = y; i >= y - slope_x && i >= 0; i-- )
								if ( rand() % stop_x > 3 ) this->m_voxels[x][i][half_z - 1 + z] = atlas.stair;
						}
					break;
				// Right to Left
					case 1:
						for ( uint x = start_x; x <= stop_x; x++ ) {
							int y = (x - start_x) * slope_x + 3;
							for ( uint z = start_z; z <= stop_z && y < this->m_size.y; z++ )
							for ( int i = y; i >= y - slope_x && i >= 0; i-- )
								if ( rand() % stop_x > 3 ) this->m_voxels[x][this->m_size.y - 1 - i][half_z - 1 + z] = atlas.stair;
						}
					break;
				}
			break;
		}
	}



	for ( uint x = 0; x < this->m_size.x; x++ ) { 
	for ( uint y = 0; y < this->m_size.y; y++ ) { 
	for ( uint z = 0; z < this->m_size.z; z++ ) {
	//	if ( y > 0 ) this->m_voxels[x][y][z] = 0;
	//	if ( x == 0 ) this->m_voxels[x][y][z] = atlas.floor;
	//	if ( y == 0 ) this->m_voxels[x][y][z] = atlas.floor;
	//	if ( z == 0 ) this->m_voxels[x][y][z] = atlas.floor;
	//	if ( x == this->m_size.x - 1 ) this->m_voxels[x][y][z] = atlas.floor;
	//	if ( y == this->m_size.y - 1 ) this->m_voxels[x][y][z] = atlas.floor;
	//	if ( z == this->m_size.z - 1 ) this->m_voxels[x][y][z] = atlas.floor;
	}
	}
	}

}
ext::TerrainVoxel::uid_t*** ext::TerrainGenerator::getVoxels() {
	return this->m_voxels;
}
void ext::TerrainGenerator::rasterize( uf::Mesh& mesh, const ext::Region& region, bool isolated, bool modelMatrixed ){
	if ( !this->m_voxels ) return;

	std::vector<float>& position = mesh.getPositions().getVertices();
	std::vector<float>& uv = mesh.getUvs().getVertices();
	std::vector<float>& normal = mesh.getNormals().getVertices();

	struct { struct { float x, y, z; } position; struct { float u, v, x, y; } uv; } offset;

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

	bool modelless = region.hasComponent<uf::Mesh>();

//	std::cout << "L: " << location.x << ", " << location.y << ", " << location.z << std::endl;
//	std::cout << "T: " << transform.position.x << ", " << transform.position.y << ", " << transform.position.z << std::endl;
//	std::cout << " ==== ===== " << std::endl;

	for ( uint x = 0; x < this->m_size.x; ++x ) {
		for ( uint y = 0; y < this->m_size.y; ++y ) {
			for ( uint z = 0; z < this->m_size.z; ++z ) {
				offset.position.x = x - (this->m_size.x / 2.0f);
				offset.position.y = y - (this->m_size.y / 2.0f);
				offset.position.z = z - (this->m_size.z / 2.0f);

				ext::TerrainVoxel voxel = ext::TerrainVoxel::atlas(this->m_voxels[x][y][z]);
				const ext::TerrainVoxel::Model& model = voxel.model();

				if ( !voxel.opaque() ) continue;

				if ( !modelMatrixed ) {
					offset.position.x += transform.position.x;
					offset.position.y += transform.position.y;
					offset.position.z += transform.position.z;
				}
			
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

				if ( x > 0 ) neighbor.left = ext::TerrainVoxel::atlas(this->m_voxels[x-1][y][z]); else if ( regions.left != NULL ) {
					const ext::Region& left = *regions.left;
					const ext::TerrainGenerator& t = left.getComponent<ext::TerrainGenerator>();
					if ( t.m_voxels != NULL ) neighbor.left = ext::TerrainVoxel::atlas(t.m_voxels[t.m_size.x-1][y][z]);
				}
				if ( y > 0 ) neighbor.bottom = ext::TerrainVoxel::atlas(this->m_voxels[x][y-1][z]); else if ( regions.bottom != NULL ) {
					const ext::Region& bottom = *regions.bottom;
					const ext::TerrainGenerator& t = bottom.getComponent<ext::TerrainGenerator>();
					if ( t.m_voxels != NULL ) neighbor.bottom =ext::TerrainVoxel::atlas( t.m_voxels[x][t.m_size.y-1][z]);
				}
				if ( z > 0 ) neighbor.back = ext::TerrainVoxel::atlas(this->m_voxels[x][y][z-1]); else if ( regions.back != NULL ) {
					const ext::Region& back = *regions.back;
					const ext::TerrainGenerator& t = back.getComponent<ext::TerrainGenerator>();
					if ( t.m_voxels != NULL ) neighbor.back = ext::TerrainVoxel::atlas(t.m_voxels[x][y][t.m_size.z-1]);
				}
				
				if ( x + 1 < this->m_size.x ) neighbor.right = ext::TerrainVoxel::atlas(this->m_voxels[x+1][y][z]); else if ( regions.right != NULL ) {
					const ext::Region& right = *regions.right;
					const ext::TerrainGenerator& t = right.getComponent<ext::TerrainGenerator>();
					if ( t.m_voxels != NULL ) neighbor.right = ext::TerrainVoxel::atlas(t.m_voxels[0][y][z]);
				}
				if ( y + 1 < this->m_size.y ) neighbor.top = ext::TerrainVoxel::atlas(this->m_voxels[x][y+1][z]); else if ( regions.top != NULL ) {
					const ext::Region& top = *regions.top;
					const ext::TerrainGenerator& t = top.getComponent<ext::TerrainGenerator>();
					if ( t.m_voxels != NULL ) neighbor.top = ext::TerrainVoxel::atlas(t.m_voxels[x][0][z]);
				}
				if ( z + 1 < this->m_size.z ) neighbor.front = ext::TerrainVoxel::atlas(this->m_voxels[x][y][z+1]); else if ( regions.front != NULL ) {
					const ext::Region& front = *regions.front;
					const ext::TerrainGenerator& t = front.getComponent<ext::TerrainGenerator>();
					if ( t.m_voxels != NULL ) neighbor.front = ext::TerrainVoxel::atlas(t.m_voxels[x][y][0]);
				}

				should.right = !neighbor.right.opaque();
				should.left = !neighbor.left.opaque();
				should.top = !neighbor.top.opaque();
				should.bottom = !neighbor.bottom.opaque();
				should.front = !neighbor.front.opaque();
				should.back = !neighbor.back.opaque();

				if ( should.right ) {
					for ( uint i = 0; i < model.position.right.size() / 3; ++i ) {
						struct { float x, y, z; } p;
						p.x = model.position.right[i*3+0]; p.y = model.position.right[i*3+1]; p.z = model.position.right[i*3+2];
						p.x += offset.position.x; p.y += offset.position.y; p.z += offset.position.z;
						position.push_back(p.x); position.push_back(p.y); position.push_back(p.z);
					}
					for ( uint i = 0; i < model.uv.right.size() / 2; ++i ) {
						struct { float x, y; } p;
						p.x = model.uv.right[i*2+0]; p.y = model.uv.right[i*2+1];
						p.x *= offset.uv.x; p.y *= offset.uv.y;
						p.x += offset.uv.u; p.y += offset.uv.v;
						uv.push_back(p.x); uv.push_back(p.y);
					}
					for ( uint i = 0; i < model.normal.right.size() / 3; ++i ) {
						struct { float x, y, z; } p;
						p.x = model.normal.right[i*3+0]; p.y = model.normal.right[i*3+1]; p.z = model.normal.right[i*3+2];
						normal.push_back(p.x); normal.push_back(p.y); normal.push_back(p.z);
					}
				}
				if ( should.left ) {
					for ( uint i = 0; i < model.position.left.size() / 3; ++i ) {
						struct { float x, y, z; } p;
						p.x = model.position.left[i*3+0]; p.y = model.position.left[i*3+1]; p.z = model.position.left[i*3+2];
						p.x += offset.position.x; p.y += offset.position.y; p.z += offset.position.z;
						position.push_back(p.x); position.push_back(p.y); position.push_back(p.z);
					}
					for ( uint i = 0; i < model.uv.left.size() / 2; ++i ) {
						struct { float x, y; } p;
						p.x = model.uv.left[i*2+0]; p.y = model.uv.left[i*2+1];
						p.x *= offset.uv.x; p.y *= offset.uv.y;
						p.x += offset.uv.u; p.y += offset.uv.v;
						uv.push_back(p.x); uv.push_back(p.y);
					}
					for ( uint i = 0; i < model.normal.left.size() / 3; ++i ) {
						struct { float x, y, z; } p;
						p.x = model.normal.left[i*3+0]; p.y = model.normal.left[i*3+1]; p.z = model.normal.left[i*3+2];
						normal.push_back(p.x); normal.push_back(p.y); normal.push_back(p.z);
					}
				}
				if ( should.top ) {
					for ( uint i = 0; i < model.position.top.size() / 3; ++i ) {
						struct { float x, y, z; } p;
						p.x = model.position.top[i*3+0]; p.y = model.position.top[i*3+1]; p.z = model.position.top[i*3+2];
						p.x += offset.position.x; p.y += offset.position.y; p.z += offset.position.z;
						position.push_back(p.x); position.push_back(p.y); position.push_back(p.z);
					}
					for ( uint i = 0; i < model.uv.top.size() / 2; ++i ) {
						struct { float x, y; } p;
						p.x = model.uv.top[i*2+0]; p.y = model.uv.top[i*2+1];
						p.x *= offset.uv.x; p.y *= offset.uv.y;
						p.x += offset.uv.u; p.y += offset.uv.v;
						uv.push_back(p.x); uv.push_back(p.y);
					}
					for ( uint i = 0; i < model.normal.top.size() / 3; ++i ) {
						struct { float x, y, z; } p;
						p.x = model.normal.top[i*3+0]; p.y = model.normal.top[i*3+1]; p.z = model.normal.top[i*3+2];
						normal.push_back(p.x); normal.push_back(p.y); normal.push_back(p.z);
					}
				}
				if ( should.bottom ) {
					for ( uint i = 0; i < model.position.bottom.size() / 3; ++i ) {
						struct { float x, y, z; } p;
						p.x = model.position.bottom[i*3+0]; p.y = model.position.bottom[i*3+1]; p.z = model.position.bottom[i*3+2];
						p.x += offset.position.x; p.y += offset.position.y; p.z += offset.position.z;
						position.push_back(p.x); position.push_back(p.y); position.push_back(p.z);
					}
					for ( uint i = 0; i < model.uv.bottom.size() / 2; ++i ) {
						struct { float x, y; } p;
						p.x = model.uv.bottom[i*2+0]; p.y = model.uv.bottom[i*2+1];
						p.x *= offset.uv.x; p.y *= offset.uv.y;
						p.x += offset.uv.u; p.y += offset.uv.v;
						uv.push_back(p.x); uv.push_back(p.y);
					}
					for ( uint i = 0; i < model.normal.bottom.size() / 3; ++i ) {
						struct { float x, y, z; } p;
						p.x = model.normal.bottom[i*3+0]; p.y = model.normal.bottom[i*3+1]; p.z = model.normal.bottom[i*3+2];
						normal.push_back(p.x); normal.push_back(p.y); normal.push_back(p.z);
					}
				}
				if ( should.front ) {
					for ( uint i = 0; i < model.position.front.size() / 3; ++i ) {
						struct { float x, y, z; } p;
						p.x = model.position.front[i*3+0]; p.y = model.position.front[i*3+1]; p.z = model.position.front[i*3+2];
						p.x += offset.position.x; p.y += offset.position.y; p.z += offset.position.z;
						position.push_back(p.x); position.push_back(p.y); position.push_back(p.z);
					}
					for ( uint i = 0; i < model.uv.front.size() / 2; ++i ) {
						struct { float x, y; } p;
						p.x = model.uv.front[i*2+0]; p.y = model.uv.front[i*2+1];
						p.x *= offset.uv.x; p.y *= offset.uv.y;
						p.x += offset.uv.u; p.y += offset.uv.v;
						uv.push_back(p.x); uv.push_back(p.y);
					}
					for ( uint i = 0; i < model.normal.front.size() / 3; ++i ) {
						struct { float x, y, z; } p;
						p.x = model.normal.front[i*3+0]; p.y = model.normal.front[i*3+1]; p.z = model.normal.front[i*3+2];
						normal.push_back(p.x); normal.push_back(p.y); normal.push_back(p.z);
					}
				}
				if ( should.back ) {
					for ( uint i = 0; i < model.position.back.size() / 3; ++i ) {
						struct { float x, y, z; } p;
						p.x = model.position.back[i*3+0]; p.y = model.position.back[i*3+1]; p.z = model.position.back[i*3+2];
						p.x += offset.position.x; p.y += offset.position.y; p.z += offset.position.z;
						position.push_back(p.x); position.push_back(p.y); position.push_back(p.z);
					}
					for ( uint i = 0; i < model.uv.back.size() / 2; ++i ) {
						struct { float x, y; } p;
						p.x = model.uv.back[i*2+0]; p.y = model.uv.back[i*2+1];
						p.x *= offset.uv.x; p.y *= offset.uv.y;
						p.x += offset.uv.u; p.y += offset.uv.v;
						uv.push_back(p.x); uv.push_back(p.y);
					}
					for ( uint i = 0; i < model.normal.back.size() / 3; ++i ) {
						struct { float x, y, z; } p;
						p.x = model.normal.back[i*3+0]; p.y = model.normal.back[i*3+1]; p.z = model.normal.back[i*3+2];
						normal.push_back(p.x); normal.push_back(p.y); normal.push_back(p.z);
					}
				}
			}
		}
	}
	if ( isolated ) {
		mesh.index();
		mesh.generate();
	}
}