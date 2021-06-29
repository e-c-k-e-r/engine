#pragma once

#include <uf/config.h>
#include <uf/ext/ext.h>

#include <uf/utils/math/vector.h>

#include "voxel.h"
#include "region.h"
#include "terrain.h"

#include <uf/utils/noise/noise.h>

#include <uf/utils/graphic/mesh.h>

#include <uf/utils/string/rle.h>

namespace ext {
	class EXT_API TerrainGenerator {
	protected:
		enum Swizzle {
			XYZ, YZX, ZXY
		};
		struct {
			struct {
				uf::stl::vector<ext::TerrainVoxel::uid_t> raw;
				pod::RLE<ext::TerrainVoxel::uid_t>::string_t rle;
		 		Swizzle swizzle = Swizzle::YZX;
			} id;
			struct {
				uf::stl::vector<ext::TerrainVoxel::light_t> raw;
				pod::RLE<ext::TerrainVoxel::light_t>::string_t rle;
		 		Swizzle swizzle = Swizzle::YZX;
			} lighting;
		} m_voxels;

		pod::Vector3ui m_size;
		pod::Vector3i m_location;
		uint m_modulus;
		uint m_subdivisions;
		uf::Entity* m_terrain;
	public:
		typedef uf::ColoredMesh mesh_t;
		static uf::PerlinNoise noise;
		static Swizzle DEFAULT_SWIZZLE;

		void initialize( const pod::Vector3ui& = { 8, 8, 8 }, uint = 1 );
		void destroy();

		void light( int, int, int, const ext::TerrainVoxel::light_t& light );
		void light( const pod::Vector3i&, const ext::TerrainVoxel::light_t& light );

		void writeToFile();
		void updateLight();

		void generate( uf::Object& );
		void rasterize( uf::stl::vector<TerrainGenerator::mesh_t::vertex_t>& vertices, const uf::Object&, bool = false );
	//	void vectorize( uf::stl::vector<uf::renderer::ComputeGraphic::Cube>&, const uf::Object& );
		
		uf::stl::vector<ext::TerrainVoxel::uid_t> getRawVoxels();
		const pod::RLE<ext::TerrainVoxel::uid_t>::string_t& getVoxels() const;

		pod::Vector3i unwrapIndex( int64_t, Swizzle = DEFAULT_SWIZZLE ) const;
		int64_t wrapPosition( int64_t, int64_t, int64_t, Swizzle = DEFAULT_SWIZZLE ) const;
		int64_t wrapPosition( const pod::Vector3i&, Swizzle = DEFAULT_SWIZZLE ) const;

		void unwrapVoxel();
		void unwrapLight();

		void wrapVoxel();
		void wrapLight();

		ext::TerrainVoxel::uid_t getVoxel( int x, int y, int z ) const;
		ext::TerrainVoxel::uid_t getVoxel( const pod::Vector3i& ) const;

		ext::TerrainVoxel::light_t getLight( int x, int y, int z ) const;
		ext::TerrainVoxel::light_t getLight( const pod::Vector3i& ) const;

		void setVoxel( int x, int y, int z, const ext::TerrainVoxel::uid_t&, bool = true );
		void setVoxel( const pod::Vector3i&, const ext::TerrainVoxel::uid_t&, bool = true );

		void setLight( int x, int y, int z, const ext::TerrainVoxel::light_t&, bool = true );
		void setLight( const pod::Vector3i&, const ext::TerrainVoxel::light_t&, bool = true );
	};
}