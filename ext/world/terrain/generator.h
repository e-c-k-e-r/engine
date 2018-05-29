#pragma once

#include <uf/config.h>
#include <uf/ext/ext.h>

#include <uf/utils/math/vector.h>
#include <uf/gl/mesh/mesh.h>

#include "voxel.h"
#include "region.h"
#include "terrain.h"

namespace ext {
	class EXT_API TerrainGenerator {
	protected:
		ext::TerrainVoxel::uid_t*** m_voxels;
		pod::Vector3ui m_size;
	public:
		void initialize( const pod::Vector3ui& = { 8, 8, 8 } );
		void destroy();

		void generate();
		void rasterize( uf::Mesh&, const ext::Region&, bool = true, bool = true );
		ext::TerrainVoxel::uid_t*** getVoxels();
	};
}