#pragma once

#include <uf/ext/opengl/ogl.h>

namespace ext {
	namespace opengl {
		namespace enums {
			namespace Compare {
				typedef GLenum type_t;
				static const type_t NEVER = GL_NEVER;
				static const type_t LESS = GL_LESS;
				static const type_t EQUAL = GL_EQUAL;
				static const type_t LESS_OR_EQUAL = GL_LEQUAL;
				static const type_t GREATER = GL_GREATER;
				static const type_t NOT_EQUAL = GL_NOTEQUAL;
				static const type_t GREATER_OR_EQUAL = GL_GEQUAL;
				static const type_t ALWAYS = GL_ALWAYS;
			}
			namespace Format {
				typedef GLenum type_t;
				#include "enums/format.inl"
			}
			namespace Face {
				typedef GLenum type_t;
				static const type_t CW = GL_CW;
				static const type_t CCW = GL_CCW;
			}
			namespace Layout {
				typedef GLenum type_t;
				static const type_t UNDEFINED = 0;
				static const type_t GENERAL = 1;
				static const type_t COLOR_ATTACHMENT_OPTIMAL = 2;
				static const type_t DEPTH_STENCIL_ATTACHMENT_OPTIMAL = 3;
				static const type_t DEPTH_STENCIL_READ_ONLY_OPTIMAL = 4;
				static const type_t SHADER_READ_ONLY_OPTIMAL = 5;
				static const type_t TRANSFER_SRC_OPTIMAL = 6;
				static const type_t TRANSFER_DST_OPTIMAL = 7;
				static const type_t PREINITIALIZED = 8;
			}
			namespace Image {
				typedef GLenum type_t;
				typedef GLenum viewType_t;
			#if UF_USE_OPENGL_FIXED_FUNCTION
				static const type_t TYPE_1D = 100;
				static const type_t TYPE_2D = GL_TEXTURE_2D;
				static const type_t TYPE_3D = 110;

				static const viewType_t VIEW_TYPE_1D = 120;
				static const viewType_t VIEW_TYPE_2D = GL_TEXTURE_2D;
				static const viewType_t VIEW_TYPE_3D = 130;
				static const viewType_t VIEW_TYPE_CUBE = 140;
				static const viewType_t VIEW_TYPE_1D_ARRAY = 150;
				static const viewType_t VIEW_TYPE_2D_ARRAY = 160;
				static const viewType_t VIEW_TYPE_CUBE_ARRAY = 170;
			#else
				static const type_t TYPE_1D = GL_TEXTURE_1D;
				static const type_t TYPE_2D = GL_TEXTURE_2D;
				static const type_t TYPE_3D = GL_TEXTURE_3D;

				static const viewType_t VIEW_TYPE_1D = GL_TEXTURE_1D;
				static const viewType_t VIEW_TYPE_2D = GL_TEXTURE_2D;
				static const viewType_t VIEW_TYPE_3D = GL_TEXTURE_3D;
				static const viewType_t VIEW_TYPE_CUBE = GL_TEXTURE_CUBE_MAP;
				static const viewType_t VIEW_TYPE_1D_ARRAY = GL_TEXTURE_1D_ARRAY;
				static const viewType_t VIEW_TYPE_2D_ARRAY = GL_TEXTURE_2D_ARRAY;
				static const viewType_t VIEW_TYPE_CUBE_ARRAY = GL_TEXTURE_CUBE_MAP_ARRAY;
			#endif
			}
			namespace CullMode {
				typedef GLenum type_t;
				static const type_t NONE = 0;
				static const type_t FRONT = GL_FRONT;
				static const type_t BACK = GL_BACK;
				static const type_t BOTH = GL_FRONT_AND_BACK;
			}
			namespace Shader {
				typedef GLenum type_t;
			#if UF_USE_OPENGL_FIXED_FUNCTION
				static const type_t VERTEX = 0x00000001;
				static const type_t TESSELLATION_CONTROL = 0x00000002;
				static const type_t TESSELLATION_EVALUATION = 0x00000003;
				static const type_t GEOMETRY = 0x00000004;
				static const type_t FRAGMENT = 0x00000005;
				static const type_t COMPUTE = 0x00000006;
				static const type_t ALL_GRAPHICS = 0x00000007;
				static const type_t ALL = 0x00000008;
			#else
				static const type_t VERTEX = GL_VERTEX_SHADER;
				static const type_t TESSELLATION_CONTROL = GL_TESS_CONTROL_SHADER;
				static const type_t TESSELLATION_EVALUATION = GL_TESS_EVALUATION_SHADER;
				static const type_t GEOMETRY = GL_GEOMETRY_SHADER;
				static const type_t FRAGMENT = GL_FRAGMENT_SHADER;
				static const type_t COMPUTE = GL_COMPUTE_SHADER;
				static const type_t ALL_GRAPHICS = GLenumerator(VK_SHADER_STAGE_ALL_GRAPHICS);
				static const type_t ALL = GLenumerator(VK_SHADER_STAGE_ALL);
			#endif
			}
			namespace PrimitiveTopology {
				typedef GLenum type_t;
				static const type_t POINT_LIST = GLenumerator(VK_PRIMITIVE_TOPOLOGY_POINT_LIST);
				static const type_t LINE_LIST = GLenumerator(VK_PRIMITIVE_TOPOLOGY_LINE_LIST);
				static const type_t LINE_STRIP = GLenumerator(VK_PRIMITIVE_TOPOLOGY_LINE_STRIP);
				static const type_t TRIANGLE_LIST = GLenumerator(VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST);
				static const type_t TRIANGLE_STRIP = GLenumerator(VK_PRIMITIVE_TOPOLOGY_TRIANGLE_STRIP);
				static const type_t TRIANGLE_FAN = GLenumerator(VK_PRIMITIVE_TOPOLOGY_TRIANGLE_FAN);
				static const type_t LINE_LIST_WITH_ADJACENCY = GLenumerator(VK_PRIMITIVE_TOPOLOGY_LINE_LIST_WITH_ADJACENCY);
				static const type_t LINE_STRIP_WITH_ADJACENCY = GLenumerator(VK_PRIMITIVE_TOPOLOGY_LINE_STRIP_WITH_ADJACENCY);
				static const type_t TRIANGLE_LIST_WITH_ADJACENCY = GLenumerator(VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST_WITH_ADJACENCY);
				static const type_t TRIANGLE_STRIP_WITH_ADJACENCY = GLenumerator(VK_PRIMITIVE_TOPOLOGY_TRIANGLE_STRIP_WITH_ADJACENCY);
				static const type_t PATCH_LIST = GLenumerator(VK_PRIMITIVE_TOPOLOGY_PATCH_LIST);
			}
			namespace PolygonMode {
				typedef GLenum type_t;
				static const type_t FILL = GLenumerator(VK_POLYGON_MODE_FILL);
				static const type_t LINE = GLenumerator(VK_POLYGON_MODE_LINE);
				static const type_t POINT = GLenumerator(VK_POLYGON_MODE_POINT);
			}
			namespace AddressMode {
				typedef GLenum type_t;
				static const type_t CLAMP_TO_EDGE = GLenumerator(VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE);
				static const type_t CLAMP_TO_BORDER = GLenumerator(VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_BORDER);
				static const type_t MIRRORED_REPEAT = GLenumerator(VK_SAMPLER_ADDRESS_MODE_MIRRORED_REPEAT);
				static const type_t REPEAT = GLenumerator(VK_SAMPLER_ADDRESS_MODE_REPEAT);
			}
			namespace Filter {
				typedef GLenum type_t;
				static const type_t NEAREST = GL_NEAREST;
				static const type_t LINEAR = GL_LINEAR;
			}
			namespace Type {
				typedef GLenum type_t;
			#if UF_USE_OPENGL_FIXED_FUNCTION
				static const type_t BYTE = GL_BYTE;
				static const type_t UBYTE = GL_UNSIGNED_BYTE;

				static const type_t SHORT = GL_SHORT;
				static const type_t USHORT = GL_UNSIGNED_SHORT;
				
				static const type_t INT = GL_INT;
				static const type_t UINT = GL_UNSIGNED_INT;

				static const type_t HALF = GL_FLOAT;
				static const type_t FLOAT = GL_FLOAT;
				static const type_t DOUBLE = GL_DOUBLE;
				static const type_t FIXED = 0;
			#else
				static const type_t BYTE = GL_BYTE;
				static const type_t UBYTE = GL_UNSIGNED_BYTE;

				static const type_t SHORT = GL_SHORT;
				static const type_t USHORT = GL_UNSIGNED_SHORT;
				
				static const type_t INT = GL_INT;
				static const type_t UINT = GL_UNSIGNED_INT;

				static const type_t HALF = GL_HALF_FLOAT;
				static const type_t FLOAT = GL_FLOAT;
				static const type_t DOUBLE = GL_DOUBLE;
				static const type_t FIXED = GL_FIXED;
			#endif
			}
			namespace Buffer {
				typedef GLenum type_t;
				static const type_t TRANSFER_SRC 	= 0b0000000000000001; //0x00000001;
				static const type_t TRANSFER_DST 	= 0b0000000000000010; //0x00000002;
				static const type_t UNIFORM_TEXEL 	= 0b0000000000000100; //0x00000004;
				static const type_t STORAGE_TEXEL 	= 0b0000000000001000; //0x00000008;
				static const type_t UNIFORM 		= 0b0000000000010000; //0x00000010;
				static const type_t STORAGE 		= 0b0000000000100000; //0x00000020;
				static const type_t INDEX 			= 0b0000000001000000; //0x00000040;
				static const type_t VERTEX 			= 0b0000000010000000; //0x00000080;
				static const type_t INDIRECT 		= 0b0000000100000000; //0x00000100;
				
				static const type_t STREAM 			= 0b0000001000000000; //
				static const type_t STATIC 			= 0b0000010000000000; //
				static const type_t DYNAMIC 		= 0b0000100000000000; //
				static const type_t DRAW 			= 0b0001000000000000; //
				static const type_t READ 			= 0b0010000000000000; //
				static const type_t COPY 			= 0b0100000000000000; //
			}
			namespace Command {
				typedef size_t type_t;
				static const type_t CLEAR 					= 1;
				static const type_t VIEWPORT 				= 2;
				static const type_t VARIANT 				= 3;
				static const type_t BIND_BUFFER 			= 4;
				static const type_t BIND_GRAPHIC_BUFFER 	= 5;
				static const type_t BIND_TEXTURE 			= 6;
				static const type_t BIND_PIPELINE 			= 7;
				static const type_t DRAW 					= 8;
				
				static const type_t GENERATE_TEXTURE 		= 9;
				static const type_t GENERATE_BUFFER 		= 10;
			}
		}
		template<typename T>
		ext::opengl::enums::Type::type_t typeToEnum() {
			if ( TYPE(T) == TYPE(int8_t) ) return ext::opengl::enums::Type::BYTE;
			if ( TYPE(T) == TYPE(uint8_t) ) return ext::opengl::enums::Type::UBYTE;
			if ( TYPE(T) == TYPE(int16_t) ) return ext::opengl::enums::Type::SHORT;
			if ( TYPE(T) == TYPE(uint16_t) ) return ext::opengl::enums::Type::USHORT;
			if ( TYPE(T) == TYPE(int32_t) ) return ext::opengl::enums::Type::INT;
			if ( TYPE(T) == TYPE(uint32_t) ) return ext::opengl::enums::Type::UINT;
			if ( TYPE(T) == TYPE(float) ) return ext::opengl::enums::Type::FLOAT;
			if ( TYPE(T) == TYPE(double) ) return ext::opengl::enums::Type::DOUBLE;
			return 0;
		}
	}
}