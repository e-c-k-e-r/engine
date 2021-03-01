#pragma once

#include <uf/ext/vulkan/vk.h>

namespace ext {
	namespace vulkan {
		namespace enums {
			namespace Compare {
				typedef decltype(VK_COMPARE_OP_NEVER) type_t;
				static const type_t NEVER = VK_COMPARE_OP_NEVER;
				static const type_t LESS = VK_COMPARE_OP_LESS;
				static const type_t EQUAL = VK_COMPARE_OP_EQUAL;
				static const type_t LESS_OR_EQUAL = VK_COMPARE_OP_LESS_OR_EQUAL;
				static const type_t GREATER = VK_COMPARE_OP_GREATER;
				static const type_t NOT_EQUAL = VK_COMPARE_OP_NOT_EQUAL;
				static const type_t GREATER_OR_EQUAL = VK_COMPARE_OP_GREATER_OR_EQUAL;
				static const type_t ALWAYS = VK_COMPARE_OP_ALWAYS;
			}
			namespace Format {
				typedef decltype(VK_FORMAT_R32G32_SFLOAT) type_t;
				#include "enums/format.inl"
			}
			namespace Face {
				typedef decltype(VK_FRONT_FACE_CLOCKWISE) type_t;
				static const type_t CW = VK_FRONT_FACE_CLOCKWISE;
				static const type_t CCW = VK_FRONT_FACE_COUNTER_CLOCKWISE;
			}
			namespace Layout {
				typedef decltype(VK_IMAGE_LAYOUT_UNDEFINED) type_t;
				static const type_t UNDEFINED = VK_IMAGE_LAYOUT_UNDEFINED;
				static const type_t GENERAL = VK_IMAGE_LAYOUT_GENERAL;
				static const type_t COLOR_ATTACHMENT_OPTIMAL = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;
				static const type_t DEPTH_STENCIL_ATTACHMENT_OPTIMAL = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;
				static const type_t DEPTH_STENCIL_READ_ONLY_OPTIMAL = VK_IMAGE_LAYOUT_DEPTH_STENCIL_READ_ONLY_OPTIMAL;
				static const type_t SHADER_READ_ONLY_OPTIMAL = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
				static const type_t TRANSFER_SRC_OPTIMAL = VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL;
				static const type_t TRANSFER_DST_OPTIMAL = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL;
				static const type_t PREINITIALIZED = VK_IMAGE_LAYOUT_PREINITIALIZED;
			}
			namespace Image {
				typedef decltype(VK_IMAGE_TYPE_1D) type_t;
				typedef decltype(VK_IMAGE_VIEW_TYPE_1D) viewType_t;
				static const type_t TYPE_1D = VK_IMAGE_TYPE_1D;
				static const type_t TYPE_2D = VK_IMAGE_TYPE_2D;
				static const type_t TYPE_3D = VK_IMAGE_TYPE_3D;

				static const viewType_t VIEW_TYPE_1D = VK_IMAGE_VIEW_TYPE_1D;
				static const viewType_t VIEW_TYPE_2D = VK_IMAGE_VIEW_TYPE_2D;
				static const viewType_t VIEW_TYPE_3D = VK_IMAGE_VIEW_TYPE_3D;
				static const viewType_t VIEW_TYPE_CUBE = VK_IMAGE_VIEW_TYPE_CUBE;
				static const viewType_t VIEW_TYPE_1D_ARRAY = VK_IMAGE_VIEW_TYPE_1D_ARRAY;
				static const viewType_t VIEW_TYPE_2D_ARRAY = VK_IMAGE_VIEW_TYPE_2D_ARRAY;
				static const viewType_t VIEW_TYPE_CUBE_ARRAY = VK_IMAGE_VIEW_TYPE_CUBE_ARRAY;
			}
			namespace CullMode {
				typedef decltype(VK_CULL_MODE_NONE) type_t;
				static const type_t NONE = VK_CULL_MODE_NONE;
				static const type_t FRONT = VK_CULL_MODE_FRONT_BIT;
				static const type_t BACK = VK_CULL_MODE_BACK_BIT;
				static const type_t BOTH = VK_CULL_MODE_FRONT_AND_BACK;
			}
			namespace Shader {
				typedef decltype(VK_SHADER_STAGE_VERTEX_BIT) type_t;
				static const type_t VERTEX = VK_SHADER_STAGE_VERTEX_BIT;
				static const type_t TESSELLATION_CONTROL = VK_SHADER_STAGE_TESSELLATION_CONTROL_BIT;
				static const type_t TESSELLATION_EVALUATION = VK_SHADER_STAGE_TESSELLATION_EVALUATION_BIT;
				static const type_t GEOMETRY = VK_SHADER_STAGE_GEOMETRY_BIT;
				static const type_t FRAGMENT = VK_SHADER_STAGE_FRAGMENT_BIT;
				static const type_t COMPUTE = VK_SHADER_STAGE_COMPUTE_BIT;
				static const type_t ALL_GRAPHICS = VK_SHADER_STAGE_ALL_GRAPHICS;
				static const type_t ALL = VK_SHADER_STAGE_ALL;
			}
			namespace PrimitiveTopology {
				typedef decltype(VK_PRIMITIVE_TOPOLOGY_POINT_LIST) type_t;
				static const type_t POINT_LIST = VK_PRIMITIVE_TOPOLOGY_POINT_LIST;
				static const type_t LINE_LIST = VK_PRIMITIVE_TOPOLOGY_LINE_LIST;
				static const type_t LINE_STRIP = VK_PRIMITIVE_TOPOLOGY_LINE_STRIP;
				static const type_t TRIANGLE_LIST = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST;
				static const type_t TRIANGLE_STRIP = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_STRIP;
				static const type_t TRIANGLE_FAN = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_FAN;
				static const type_t LINE_LIST_WITH_ADJACENCY = VK_PRIMITIVE_TOPOLOGY_LINE_LIST_WITH_ADJACENCY;
				static const type_t LINE_STRIP_WITH_ADJACENCY = VK_PRIMITIVE_TOPOLOGY_LINE_STRIP_WITH_ADJACENCY;
				static const type_t TRIANGLE_LIST_WITH_ADJACENCY = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST_WITH_ADJACENCY;
				static const type_t TRIANGLE_STRIP_WITH_ADJACENCY = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_STRIP_WITH_ADJACENCY;
				static const type_t PATCH_LIST = VK_PRIMITIVE_TOPOLOGY_PATCH_LIST;
			}
			namespace PolygonMode {
				typedef decltype(VK_POLYGON_MODE_FILL) type_t;
				static const type_t FILL = VK_POLYGON_MODE_FILL;
				static const type_t LINE = VK_POLYGON_MODE_LINE;
				static const type_t POINT = VK_POLYGON_MODE_POINT;
			}
			namespace AddressMode {
				typedef decltype(VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE) type_t;
				static const type_t CLAMP_TO_EDGE = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE;
				static const type_t CLAMP_TO_BORDER = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_BORDER;
				static const type_t MIRRORED_REPEAT = VK_SAMPLER_ADDRESS_MODE_MIRRORED_REPEAT;
				static const type_t REPEAT = VK_SAMPLER_ADDRESS_MODE_REPEAT;
			}
			namespace Filter {
				typedef decltype(VK_FILTER_NEAREST) type_t;
				static const type_t NEAREST = VK_FILTER_NEAREST;
				static const type_t LINEAR = VK_FILTER_LINEAR;
			}
			namespace Type {
				typedef size_t type_t;
				static const type_t BYTE = 1;
				static const type_t UBYTE = 2;

				static const type_t SHORT = 3;
				static const type_t USHORT = 4;
				
				static const type_t INT = 5;
				static const type_t UINT = 6;

				static const type_t HALF = 7;
				static const type_t FLOAT = 8;
				static const type_t DOUBLE = 9;
				static const type_t FIXED = 10;
			}
			namespace Buffer {
				typedef decltype(VK_BUFFER_USAGE_TRANSFER_SRC_BIT) type_t;
				static const type_t TRANSFER_SRC = VK_BUFFER_USAGE_TRANSFER_SRC_BIT;
				static const type_t TRANSFER_DST = VK_BUFFER_USAGE_TRANSFER_DST_BIT;
				static const type_t UNIFORM_TEXEL = VK_BUFFER_USAGE_UNIFORM_TEXEL_BUFFER_BIT;
				static const type_t STORAGE_TEXEL = VK_BUFFER_USAGE_STORAGE_TEXEL_BUFFER_BIT;
				static const type_t UNIFORM = VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT;
				static const type_t STORAGE = VK_BUFFER_USAGE_STORAGE_BUFFER_BIT;
				static const type_t INDEX = VK_BUFFER_USAGE_INDEX_BUFFER_BIT;
				static const type_t VERTEX = VK_BUFFER_USAGE_VERTEX_BUFFER_BIT;
				static const type_t INDIRECT = VK_BUFFER_USAGE_INDIRECT_BUFFER_BIT;

				static const type_t STREAM = {};
				static const type_t STATIC = {};
				static const type_t DYNAMIC = {};
				static const type_t DRAW = {};
				static const type_t READ = {};
				static const type_t COPY = {};
			}
		}
		template<typename T>
		ext::vulkan::enums::Type::type_t typeToEnum() {
			if ( typeid(T) == typeid(int8_t) ) return ext::vulkan::enums::Type::BYTE;
			if ( typeid(T) == typeid(uint8_t) ) return ext::vulkan::enums::Type::UBYTE;
			if ( typeid(T) == typeid(int16_t) ) return ext::vulkan::enums::Type::SHORT;
			if ( typeid(T) == typeid(uint16_t) ) return ext::vulkan::enums::Type::USHORT;
			if ( typeid(T) == typeid(int32_t) ) return ext::vulkan::enums::Type::INT;
			if ( typeid(T) == typeid(uint32_t) ) return ext::vulkan::enums::Type::UINT;
			if ( typeid(T) == typeid(float) ) return ext::vulkan::enums::Type::FLOAT;
			if ( typeid(T) == typeid(double) ) return ext::vulkan::enums::Type::DOUBLE;
			return 0;
		}
	}
}