#pragma once

#include <uf/config.h>
#include <uf/ext/ext.h>
#include <uf/engine/entity/entity.h>

#include <uf/utils/math/transform.h>
#include <uf/utils/math/physics.h>

#include "../../object/object.h"


namespace ext {
	class EXT_API TerrainVoxel {
	public:
		typedef uint16_t uid_t;
	//	typedef pod::Vector3t<uint8_t> light_t;
		typedef uint16_t light_t;
	protected:
		ext::TerrainVoxel::uid_t m_uid = 0;
		bool m_opaque = false;
		bool m_solid = false;
		pod::Vector2ui m_uv = {0, 0};
		ext::TerrainVoxel::light_t m_light = 0x0000;
	public:
		struct Model {
			struct { std::vector<float> left, right, top, bottom, front, back; } position;
			struct { std::vector<float> left, right, top, bottom, front, back; } uv;
			struct { std::vector<float> left, right, top, bottom, front, back; } normal;
			struct { std::vector<float> left, right, top, bottom, front, back; } color;
		};

		TerrainVoxel( ext::TerrainVoxel::uid_t = 0, bool = false, bool = false, pod::Vector2ui = {0, 0}, ext::TerrainVoxel::light_t = 0x0000 );
		TerrainVoxel( const ext::TerrainVoxel& );
		virtual ~TerrainVoxel();

		static pod::Vector2ui size();
		static const std::vector<ext::TerrainVoxel>& atlas();
		static ext::TerrainVoxel atlas( ext::TerrainVoxel::uid_t );

		virtual bool opaque() const;
		virtual bool solid() const;
		virtual pod::Vector2ui uv() const;
		virtual ext::TerrainVoxel::light_t light() const;
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
	class EXT_API TerrainVoxelPillar : public ext::TerrainVoxel  {
	public:
		TerrainVoxelPillar();
	};
	class EXT_API TerrainVoxelStair : public ext::TerrainVoxel  {
	public:
		TerrainVoxelStair();
	};
	class EXT_API TerrainVoxelLava : public ext::TerrainVoxel  {
	public:
		TerrainVoxelLava();
	};
}