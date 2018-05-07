#pragma once

#include <uf/config.h>
#include <uf/ext/ext.h>
#include <uf/engine/entity/entity.h>

#include <uf/gl/shader/shader.h>
#include <uf/gl/mesh/mesh.h>
#include <uf/gl/texture/texture.h>
#include <uf/gl/camera/camera.h>

#include <uf/utils/math/transform.h>
#include <uf/utils/math/physics.h>

#include "../object/object.h"


namespace ext {
	class EXT_API TerrainVoxel {
	public:
		typedef uint uid_t;
	protected:
		ext::TerrainVoxel::uid_t m_uid;
		bool m_opaque;
		pod::Vector2ui m_uv;
	public:
		struct Model {
			struct { std::vector<float> left, right, top, bottom, front, back; } position;
			struct { std::vector<float> left, right, top, bottom, front, back; } uv;
			struct { std::vector<float> left, right, top, bottom, front, back; } normal;
			struct { std::vector<float> left, right, top, bottom, front, back; } color;
		};

		TerrainVoxel( ext::TerrainVoxel::uid_t = 0, bool = false, pod::Vector2ui = {0, 0} );
		TerrainVoxel( const ext::TerrainVoxel& );
		virtual ~TerrainVoxel();

		static pod::Vector2ui size();
		static const std::vector<ext::TerrainVoxel>& atlas();
		static const ext::TerrainVoxel& atlas( ext::TerrainVoxel::uid_t );

		virtual bool opaque() const;
		virtual pod::Vector2ui uv() const;
		virtual ext::TerrainVoxel::uid_t uid() const;
		virtual ext::TerrainVoxel::Model model() const;
	};
	
	class EXT_API TerrainVoxelFloor : public ext::TerrainVoxel  {
	public:
		TerrainVoxelFloor();
	};
	class EXT_API TerrainVoxelWall : public ext::TerrainVoxel  {
	public:
		TerrainVoxelWall();
	};
	class EXT_API TerrainVoxelCeiling : public ext::TerrainVoxel  {
	public:
		TerrainVoxelCeiling();
	};
}