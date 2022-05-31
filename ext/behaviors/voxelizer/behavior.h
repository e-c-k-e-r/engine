#pragma once

#include <uf/config.h>
#include <uf/ext/ext.h>
#include <uf/engine/entity/entity.h>
#include <uf/engine/scene/scene.h>
#include <uf/utils/math/vector.h>
#include <uf/utils/math/matrix.h>

namespace ext {
	namespace VoxelizerBehavior {
		UF_BEHAVIOR_DEFINE_TYPE();
		EXT_BEHAVIOR_DEFINE_TRAITS();
		EXT_BEHAVIOR_DEFINE_FUNCTIONS();
		UF_BEHAVIOR_DEFINE_METADATA(
			pod::Vector3ui fragmentSize = { 0, 0 };
			pod::Vector3ui voxelSize = { 0, 0, 0 };
			pod::Vector3ui dispatchSize = { 0, 0, 0 };
			uf::stl::string renderModeName = "VXGI";
			
			size_t cascades = 0;
			float cascadePower = 0;
			float granularity = 0;
			uint32_t shadows = 0;

			struct {
				pod::Vector3f min = {};
				pod::Vector3f max = {};
				pod::Matrix4f matrix = uf::matrix::identity();
			} extents;
			struct {
				float frequency = 0.0f;
				float timer = 0.0f;
			} limiter;
			bool initialized = false;
		);
	}
}