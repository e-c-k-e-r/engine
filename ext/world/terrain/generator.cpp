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
void ext::TerrainGenerator::generate(){ if ( this->m_voxels ) return;
	this->m_voxels = new ext::TerrainVoxel::uid_t**[this->m_size.x];

	struct {
		ext::TerrainVoxel::uid_t floor = ext::TerrainVoxelFloor().uid();
		ext::TerrainVoxel::uid_t wall = ext::TerrainVoxelWall().uid();
		ext::TerrainVoxel::uid_t ceiling = ext::TerrainVoxelCeiling().uid();
	} atlas;

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

			//	if ( y > 3 ) voxel = 0;
				this->m_voxels[x][y][z] = voxel;
			}
		}
	}
}
void ext::TerrainGenerator::rasterize( uf::Mesh& mesh, const ext::Region& region ){
	if ( !this->m_voxels ) this->generate();

	std::vector<float>& position = mesh.getPositions().getVertices();
	std::vector<float>& uv = mesh.getUvs().getVertices();
	std::vector<float>& normal = mesh.getNormals().getVertices();

	struct { struct { float x, y, z; } position; struct { float u, v, x, y; } uv; } offset;

	const ext::Terrain& terrain = region.getParent<ext::Terrain>();
	const pod::Transform<>& transform = region.getComponent<pod::Transform<>>();
	const pod::Vector3i location = {
		(int) (transform.position.x) / this->m_size.x,
		(int) (transform.position.y) / this->m_size.y,
		(int) (transform.position.z) / this->m_size.z,
	};
	struct { ext::Region* right = NULL; ext::Region* left = NULL; ext::Region* top = NULL; ext::Region* bottom = NULL; ext::Region* front = NULL; ext::Region* back = NULL; } regions;
	regions.right = terrain.at( { location.x + 1, location.y, location.z } );
	regions.left = terrain.at( { location.x - 1, location.y, location.z } );
	regions.top = terrain.at( { location.x, location.y + 1, location.z } );
	regions.bottom = terrain.at( { location.x, location.y - 1, location.z } );
	regions.front = terrain.at( { location.x, location.y, location.z + 1 } );
	regions.back = terrain.at( { location.x, location.y, location.z - 1 } );


	for ( uint x = 0; x < this->m_size.x; ++x ) {
		for ( uint y = 0; y < this->m_size.y; ++y ) {
			for ( uint z = 0; z < this->m_size.z; ++z ) {
				offset.position.x = x - (this->m_size.x / 2.0f);
				offset.position.y = y; // - (this->m_size.y / 2.0f);
				offset.position.z = z - (this->m_size.z / 2.0f);

				const ext::TerrainVoxel& voxel = ext::TerrainVoxel::atlas(this->m_voxels[x][y][z]);
				const ext::TerrainVoxel::Model& model = voxel.model();

				if ( !voxel.opaque() ) continue;
			
				offset.uv.x = 1.0f / ext::TerrainVoxel::size().x;
				offset.uv.y = 1.0f / ext::TerrainVoxel::size().y;

				offset.uv.u = voxel.uv().x * offset.uv.x;
				offset.uv.v = (ext::TerrainVoxel::size().y - 1 - voxel.uv().y) * offset.uv.y;

				struct {
					bool top = true, bottom = true, left = true, right = true, front = true, back = true;
				} should;
				struct {
					ext::TerrainVoxel top, bottom, left, right, front, back;
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

	mesh.index();
	mesh.generate();
}