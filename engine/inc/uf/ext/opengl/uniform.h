#pragma once

#include <uf/utils/math/matrix.h>

namespace pod {
	struct UF_API Uniform {
		pod::Matrix4f modelView = uf::matrix::identity();
		pod::Matrix4f projection = uf::matrix::identity();

		pod::Vector4f color = pod::Vector4f{1.0f, 1.0f, 1.0f, 1.0f};
	};
}