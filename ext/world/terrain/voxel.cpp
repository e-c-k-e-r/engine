#include "voxel.h"

#include "../../ext.h"

#include <uf/utils/io/iostream.h>

ext::TerrainVoxel::TerrainVoxel( ext::TerrainVoxel::uid_t uid, bool opaque, bool solid, pod::Vector2ui uv, ext::TerrainVoxel::light_t light ) : m_uid(uid), m_opaque(opaque), m_solid(solid), m_uv(uv), m_light(light) {

}
ext::TerrainVoxel::TerrainVoxel( const ext::TerrainVoxel& voxel ) : m_uid(voxel.uid()), m_opaque(voxel.opaque()), m_solid(voxel.solid()), m_uv(voxel.uv()), m_light(voxel.light()) {

}
ext::TerrainVoxel::~TerrainVoxel() {

}

pod::Vector2ui ext::TerrainVoxel::size() { return {4, 4}; }

bool ext::TerrainVoxel::opaque() const { return this->m_opaque; }
bool ext::TerrainVoxel::solid() const { return this->m_solid; }
pod::Vector2ui ext::TerrainVoxel::uv() const { return this->m_uv; }
ext::TerrainVoxel::light_t ext::TerrainVoxel::light() const { return this->m_light; }
ext::TerrainVoxel::uid_t ext::TerrainVoxel::uid() const { return this->m_uid; }
ext::TerrainVoxel::Model ext::TerrainVoxel::model() const {
	ext::TerrainVoxel::Model model;
	if ( true ) {
		model.position.left = {
			-0.5f,  0.5f,  0.5f,
			-0.5f,  0.5f, -0.5f,
			-0.5f, -0.5f, -0.5f,
			-0.5f, -0.5f, -0.5f,
			-0.5f, -0.5f,  0.5f,
			-0.5f,  0.5f,  0.5f,
		};
		model.position.right = {
			 0.5f,  0.5f, -0.5f,
			 0.5f,  0.5f,  0.5f,
			 0.5f, -0.5f,  0.5f,
			 0.5f, -0.5f,  0.5f,
			 0.5f, -0.5f, -0.5f,
			 0.5f,  0.5f, -0.5f,
		};
		model.position.top = {
			 0.5f,  0.5f, -0.5f,
			-0.5f,  0.5f, -0.5f,
			-0.5f,  0.5f,  0.5f,
			-0.5f,  0.5f,  0.5f,
			 0.5f,  0.5f,  0.5f,
			 0.5f,  0.5f, -0.5f,
		};
		model.position.bottom = {
			-0.5f, -0.5f, -0.5f,
			 0.5f, -0.5f, -0.5f,
			 0.5f, -0.5f,  0.5f,
			 0.5f, -0.5f,  0.5f,
			-0.5f, -0.5f,  0.5f,
			-0.5f, -0.5f, -0.5f,
		};
		model.position.back = {
			-0.5f,  0.5f, -0.5f,
			 0.5f,  0.5f, -0.5f,
			 0.5f, -0.5f, -0.5f,
			 0.5f, -0.5f, -0.5f,
			-0.5f, -0.5f, -0.5f,
			-0.5f,  0.5f, -0.5f,
		};
		model.position.front = {
			 0.5f,  0.5f,  0.5f,
			-0.5f,  0.5f,  0.5f,
			-0.5f, -0.5f,  0.5f,
			-0.5f, -0.5f,  0.5f,
			 0.5f, -0.5f,  0.5f,
			 0.5f,  0.5f,  0.5f,
		};
		model.uv.left = {
			0.0f, 1.0f,
			1.0f, 1.0f,
			1.0f, 0.0f,

			1.0f, 0.0f,
			0.0f, 0.0f,
			0.0f, 1.0f,
		};
		model.uv.right = {
			0.0f, 1.0f,
			1.0f, 1.0f,
			1.0f, 0.0f,

			1.0f, 0.0f,
			0.0f, 0.0f,
			0.0f, 1.0f,
		};
		model.uv.bottom = {
			0.0f, 1.0f,
			1.0f, 1.0f,
			1.0f, 0.0f,

			1.0f, 0.0f,
			0.0f, 0.0f,
			0.0f, 1.0f,
		};
		model.uv.top = {
			0.0f, 1.0f,
			1.0f, 1.0f,
			1.0f, 0.0f,

			1.0f, 0.0f,
			0.0f, 0.0f,
			0.0f, 1.0f,
		};
		model.uv.back = {
			0.0f, 1.0f,
			1.0f, 1.0f,
			1.0f, 0.0f,

			1.0f, 0.0f,
			0.0f, 0.0f,
			0.0f, 1.0f,
		};
		model.uv.front = {
			0.0f, 1.0f,
			1.0f, 1.0f,
			1.0f, 0.0f,

			1.0f, 0.0f,
			0.0f, 0.0f,
			0.0f, 1.0f,
		};

		model.normal.left = {
			-1.0f, 0.0f, 0.0f,
			-1.0f, 0.0f, 0.0f,
			-1.0f, 0.0f, 0.0f,
			
			-1.0f, 0.0f, 0.0f,
			-1.0f, 0.0f, 0.0f,
			-1.0f, 0.0f, 0.0f,
		};
		model.normal.right = {
			 1.0f, 0.0f, 0.0f,
			 1.0f, 0.0f, 0.0f,
			 1.0f, 0.0f, 0.0f,
			
			 1.0f, 0.0f, 0.0f,
			 1.0f, 0.0f, 0.0f,
			 1.0f, 0.0f, 0.0f,
		};
		model.normal.top = {
			 0.0f, 1.0f, 0.0f,
			 0.0f, 1.0f, 0.0f,
			 0.0f, 1.0f, 0.0f,
			
			 0.0f, 1.0f, 0.0f,
			 0.0f, 1.0f, 0.0f,
			 0.0f, 1.0f, 0.0f,
		};
		model.normal.bottom = {
			 0.0f,-1.0f, 0.0f,
			 0.0f,-1.0f, 0.0f,
			 0.0f,-1.0f, 0.0f,
			
			 0.0f,-1.0f, 0.0f,
			 0.0f,-1.0f, 0.0f,
			 0.0f,-1.0f, 0.0f,
		};
		model.normal.back = {
			 0.0f, 0.0f,-1.0f,
			 0.0f, 0.0f,-1.0f,
			 0.0f, 0.0f,-1.0f,
			
			 0.0f, 0.0f,-1.0f,
			 0.0f, 0.0f,-1.0f,
			 0.0f, 0.0f,-1.0f,
		};
		model.normal.front = {
			 0.0f, 0.0f, 1.0f,
			 0.0f, 0.0f, 1.0f,
			 0.0f, 0.0f, 1.0f,
			
			 0.0f, 0.0f, 1.0f,
			 0.0f, 0.0f, 1.0f,
			 0.0f, 0.0f, 1.0f,
		};
	} else {
		model.position.left = {
			-0.5f, -0.5f, -0.5f,
			-0.5f,  0.5f, -0.5f,
			 0.0f,  0.0f,  0.0f,
			
			-0.5f,  0.5f, -0.5f,
			-0.5f,  0.5f,  0.5f,
			 0.0f,  0.0f,  0.0f,

			-0.5f,  0.5f,  0.5f,
			-0.5f, -0.5f,  0.5f,
			 0.0f,  0.0f,  0.0f,
			
			-0.5f, -0.5f,  0.5f,
			-0.5f, -0.5f, -0.5f,
			 0.0f,  0.0f,  0.0f,
		};
		model.position.right = {
			 0.5f, -0.5f,  0.5f,
			 0.5f,  0.5f,  0.5f,
			 0.0f,  0.0f,  0.0f,

			 0.5f,  0.5f,  0.5f,
			 0.5f,  0.5f, -0.5f,
			 0.0f,  0.0f,  0.0f,

			 0.5f,  0.5f, -0.5f,
			 0.5f, -0.5f, -0.5f,
			 0.0f,  0.0f,  0.0f,
			 
			 0.5f, -0.5f, -0.5f,
			 0.5f, -0.5f,  0.5f,
			 0.0f,  0.0f,  0.0f,
		};
		model.position.bottom = {
			 0.5f, -0.5f,  0.5f,
			 0.5f, -0.5f, -0.5f,
			 0.0f,  0.0f,  0.0f,

			 0.5f, -0.5f, -0.5f,
			-0.5f, -0.5f, -0.5f,
			 0.0f,  0.0f,  0.0f,

			-0.5f, -0.5f, -0.5f,
			-0.5f, -0.5f,  0.5f,
			 0.0f,  0.0f,  0.0f,
			
			-0.5f, -0.5f,  0.5f,
			 0.5f, -0.5f,  0.5f,
			 0.0f,  0.0f,  0.0f,
		};
		model.position.top = {
			-0.5f,  0.5f,  0.5f,
			-0.5f,  0.5f, -0.5f,
			 0.0f,  0.0f,  0.0f,

			-0.5f,  0.5f, -0.5f,
			 0.5f,  0.5f, -0.5f,
			 0.0f,  0.0f,  0.0f,

			 0.5f,  0.5f, -0.5f,
			 0.5f,  0.5f,  0.5f,
			 0.0f,  0.0f,  0.0f,
			 
			 0.5f,  0.5f,  0.5f,
			-0.5f,  0.5f,  0.5f,
			 0.0f,  0.0f,  0.0f,
		};
		model.position.back = {
			 0.5f, -0.5f, -0.5f,
			 0.5f,  0.5f, -0.5f,
			 0.0f,  0.0f,  0.0f,

			 0.5f,  0.5f, -0.5f,
			-0.5f,  0.5f, -0.5f,
			 0.0f,  0.0f,  0.0f,

			-0.5f,  0.5f, -0.5f,
			-0.5f, -0.5f, -0.5f,
			 0.0f,  0.0f,  0.0f,
			
			-0.5f, -0.5f, -0.5f,
			 0.5f, -0.5f, -0.5f,
			 0.0f,  0.0f,  0.0f,
		};
		model.position.front = {
			-0.5f, -0.5f,  0.5f,
			-0.5f,  0.5f,  0.5f,
			 0.0f,  0.0f,  0.0f,

			-0.5f,  0.5f,  0.5f,
			 0.5f,  0.5f,  0.5f,
			 0.0f,  0.0f,  0.0f,

			 0.5f,  0.5f,  0.5f,
			 0.5f, -0.5f,  0.5f,
			 0.0f,  0.0f,  0.0f,
			 
			 0.5f, -0.5f,  0.5f,
			-0.5f, -0.5f,  0.5f,
			 0.0f,  0.0f,  0.0f,
		};
		model.uv.left = {
			0.0f, 0.0f,
			0.0f, 1.0f,
			0.5f, 0.5f,

			0.0f, 1.0f,
			1.0f, 1.0f,
			0.5f, 0.5f,

			1.0f, 1.0f,
			1.0f, 0.0f,
			0.5f, 0.5f,
			
			1.0f, 0.0f,
			0.0f, 0.0f,
			0.5f, 0.5f,
		};
		model.uv.right = {
			0.0f, 0.0f,
			0.0f, 1.0f,
			0.5f, 0.5f,

			0.0f, 1.0f,
			1.0f, 1.0f,
			0.5f, 0.5f,

			1.0f, 1.0f,
			1.0f, 0.0f,
			0.5f, 0.5f,
			
			1.0f, 0.0f,
			0.0f, 0.0f,
			0.5f, 0.5f,
		};
		model.uv.bottom = {
			0.0f, 0.0f,
			0.0f, 1.0f,
			0.5f, 0.5f,

			0.0f, 1.0f,
			1.0f, 1.0f,
			0.5f, 0.5f,

			1.0f, 1.0f,
			1.0f, 0.0f,
			0.5f, 0.5f,
			
			1.0f, 0.0f,
			0.0f, 0.0f,
			0.5f, 0.5f,
		};
		model.uv.top = {
			0.0f, 0.0f,
			0.0f, 1.0f,
			0.5f, 0.5f,

			0.0f, 1.0f,
			1.0f, 1.0f,
			0.5f, 0.5f,

			1.0f, 1.0f,
			1.0f, 0.0f,
			0.5f, 0.5f,
			
			1.0f, 0.0f,
			0.0f, 0.0f,
			0.5f, 0.5f,
		};
		model.uv.back = {
			0.0f, 0.0f,
			0.0f, 1.0f,
			0.5f, 0.5f,

			0.0f, 1.0f,
			1.0f, 1.0f,
			0.5f, 0.5f,

			1.0f, 1.0f,
			1.0f, 0.0f,
			0.5f, 0.5f,
			
			1.0f, 0.0f,
			0.0f, 0.0f,
			0.5f, 0.5f,
		};
		model.uv.front = {
			0.0f, 0.0f,
			0.0f, 1.0f,
			0.5f, 0.5f,

			0.0f, 1.0f,
			1.0f, 1.0f,
			0.5f, 0.5f,

			1.0f, 1.0f,
			1.0f, 0.0f,
			0.5f, 0.5f,
			
			1.0f, 0.0f,
			0.0f, 0.0f,
			0.5f, 0.5f,
		};
		model.normal.left = {
			-1.0f, 0.0f, 0.0f,
			-1.0f, 0.0f, 0.0f,
			-1.0f, 0.0f, 0.0f,
			
			-1.0f, 0.0f, 0.0f,
			-1.0f, 0.0f, 0.0f,
			-1.0f, 0.0f, 0.0f,


			-1.0f, 0.0f, 0.0f,
			-1.0f, 0.0f, 0.0f,
			-1.0f, 0.0f, 0.0f,
		};
		model.normal.right = {
			 1.0f, 0.0f, 0.0f,
			 1.0f, 0.0f, 0.0f,
			 1.0f, 0.0f, 0.0f,
			
			 1.0f, 0.0f, 0.0f,
			 1.0f, 0.0f, 0.0f,
			 1.0f, 0.0f, 0.0f,

			 1.0f, 0.0f, 0.0f,
			 1.0f, 0.0f, 0.0f,
			 1.0f, 0.0f, 0.0f,
		};
		model.normal.top = {
			 0.0f, 1.0f, 0.0f,
			 0.0f, 1.0f, 0.0f,
			 0.0f, 1.0f, 0.0f,
			
			 0.0f, 1.0f, 0.0f,
			 0.0f, 1.0f, 0.0f,
			 0.0f, 1.0f, 0.0f,

			 0.0f, 1.0f, 0.0f,
			 0.0f, 1.0f, 0.0f,
			 0.0f, 1.0f, 0.0f,
		};
		model.normal.bottom = {
			 0.0f,-1.0f, 0.0f,
			 0.0f,-1.0f, 0.0f,
			 0.0f,-1.0f, 0.0f,
			
			 0.0f,-1.0f, 0.0f,
			 0.0f,-1.0f, 0.0f,
			 0.0f,-1.0f, 0.0f,

			 0.0f,-1.0f, 0.0f,
			 0.0f,-1.0f, 0.0f,
			 0.0f,-1.0f, 0.0f,
		};
		model.normal.back = {
			 0.0f, 0.0f,-1.0f,
			 0.0f, 0.0f,-1.0f,
			 0.0f, 0.0f,-1.0f,
			
			 0.0f, 0.0f,-1.0f,
			 0.0f, 0.0f,-1.0f,
			 0.0f, 0.0f,-1.0f,

			 0.0f, 0.0f,-1.0f,
			 0.0f, 0.0f,-1.0f,
			 0.0f, 0.0f,-1.0f,
		};
		model.normal.front = {
			 0.0f, 0.0f, 1.0f,
			 0.0f, 0.0f, 1.0f,
			 0.0f, 0.0f, 1.0f,
			
			 0.0f, 0.0f, 1.0f,
			 0.0f, 0.0f, 1.0f,
			 0.0f, 0.0f, 1.0f,

			 0.0f, 0.0f, 1.0f,
			 0.0f, 0.0f, 1.0f,
			 0.0f, 0.0f, 1.0f,
		};
	}
	return model;
}

//

ext::TerrainVoxelFloor::TerrainVoxelFloor() : ext::TerrainVoxel( 1, true, true, {0, 1}, 0x0000 ) {}
ext::TerrainVoxelWall::TerrainVoxelWall() : ext::TerrainVoxel( 2, true, true, {1, 1}, 0x0000 ) {}
ext::TerrainVoxelCeiling::TerrainVoxelCeiling() : ext::TerrainVoxel( 3, true, true, {2, 1}, 0x0000 ) {}
ext::TerrainVoxelPillar::TerrainVoxelPillar() : ext::TerrainVoxel( 4, true, true, {3, 0}, 0x0000 ) {}
ext::TerrainVoxelStair::TerrainVoxelStair() : ext::TerrainVoxel( 5, true, true, {2, 0}, 0x0000 ) {}
ext::TerrainVoxelLava::TerrainVoxelLava() : ext::TerrainVoxel( 6, true, true, {0, 2}, 0xFFFF ) {}

//
const std::vector<ext::TerrainVoxel>& ext::TerrainVoxel::atlas() {
	static std::vector<ext::TerrainVoxel> atlas;
	if ( atlas.size() == 0 ) {	
		atlas.push_back( ext::TerrainVoxel() );
		atlas.push_back( ext::TerrainVoxelFloor() );
		atlas.push_back( ext::TerrainVoxelWall() );
		atlas.push_back( ext::TerrainVoxelCeiling() );
		atlas.push_back( ext::TerrainVoxelPillar() );
		atlas.push_back( ext::TerrainVoxelStair() );
		atlas.push_back( ext::TerrainVoxelLava() );
	}
	return atlas;
}
ext::TerrainVoxel ext::TerrainVoxel::atlas( ext::TerrainVoxel::uid_t uid ) {
	std::vector<ext::TerrainVoxel> atlas = ext::TerrainVoxel::atlas();
	static ext::TerrainVoxel base;
	for ( std::vector<ext::TerrainVoxel>::iterator it = atlas.begin(); it != atlas.end(); ++it ) {
		if ( it->uid() == uid ) return *it;
	}
	return base;
}