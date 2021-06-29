#pragma once

#include <uf/utils/math/vector.h>
#include <uf/utils/math/matrix.h>
#include <uf/utils/graphic/descriptor.h>
#include <uf/ext/meshopt/meshopt.h>

#include <functional>
#include <uf/utils/memory/unordered_map.h>

#if UF_USE_VULKAN
	namespace uf {
		namespace renderer = ext::vulkan;
	}
#elif UF_USE_OPENGL
	namespace uf {
		namespace renderer = ext::opengl;
	}
#endif

namespace uf {
	template<typename T, typename U = uf::renderer::index_t>
	class /*UF_API*/ BaseMesh : public BaseGeometry {
	public:
		typedef T vertex_t;
		typedef U index_t;
		typedef vertex_t vertices_t;
		typedef index_t indices_t;
		uf::stl::vector<vertex_t> vertices;
		uf::stl::vector<index_t> indices;

		void initialize( size_t = SIZE_MAX );
		void destroy();

		void expand( bool = true );
		uf::BaseMesh<T,U> simplify( float = 0.2f );
		void optimize( size_t = SIZE_MAX );
		void insert( const uf::BaseMesh<T,U>& mesh );

		virtual void updateDescriptor();
		virtual void resizeVertices( size_t );
		virtual void resizeIndices( size_t );
	};
}

#include "mesh.inl"