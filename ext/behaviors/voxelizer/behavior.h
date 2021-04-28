#pragma once

#include <uf/config.h>
#include <uf/ext/ext.h>
#include <uf/engine/entity/entity.h>
#include <uf/engine/scene/scene.h>
#include <uf/utils/math/vector.h>
#include <uf/utils/math/matrix.h>

namespace ext {
	namespace VoxelizerBehavior {
		UF_BEHAVIOR_DEFINE_TYPE;
		void attach( uf::Object& );
		void initialize( uf::Object& );
		void tick( uf::Object& );
		void render( uf::Object& );
		void destroy( uf::Object& );
		struct Metadata {
			pod::Vector3ui fragmentSize = { 0, 0 };
			pod::Vector3ui voxelSize = { 0, 0, 0 };
			pod::Vector3ui dispatchSize = { 0, 0, 0 };
			std::string renderModeName = "VXGI";
			struct {
				pod::Vector3f min = {};
				pod::Vector3f max = {};
				pod::Matrix4f matrix = uf::matrix::identity();
			} extents;
			struct {
				float limiter = 0.0f;
				float timer = 0.0f;
			} renderer;
			bool initialized = false;
		};
	}
}