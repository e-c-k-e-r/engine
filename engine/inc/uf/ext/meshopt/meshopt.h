#pragma once

#include <uf/config.h>
#include <uf/utils/mesh/mesh.h>

#if UF_USE_MESHOPT
#include <meshoptimizer.h>
#endif

namespace ext {
	namespace meshopt {
		bool UF_API optimize( uf::Mesh&, float simplify = 1.0f, size_t = SIZE_MAX, bool verbose = false );

		template<typename T, typename U = uint32_t>
		bool optimize( uf::Meshlet_T<T,U>& meshlet, float simplify = 1.0f, size_t o = SIZE_MAX, bool verbose = false ) {
		#if UF_USE_MESHOPT
			size_t indicesCount = meshlet.indices.size();
			uf::stl::vector<U> remap(indicesCount);

			size_t verticesCount = meshopt_generateVertexRemap(&remap[0], &meshlet.indices[0], indicesCount, &meshlet.vertices[0], meshlet.vertices.size(), sizeof(T));
			uf::stl::vector<T> vertices(verticesCount);
			uf::stl::vector<U> indices(indicesCount);

			meshopt_remapIndexBuffer(&indices[0], &meshlet.indices[0], indicesCount, &remap[0]);
			meshopt_remapVertexBuffer(&vertices[0], &meshlet.vertices[0], meshlet.vertices.size(), sizeof(T), &remap[0]);

			//
			meshopt_optimizeVertexCache(&indices[0], &indices[0], indicesCount, verticesCount);
			//
			meshopt_optimizeOverdraw(&indices[0], &indices[0], indicesCount, &vertices[0].position.x, verticesCount, sizeof(T), 1.05f);
			//
			meshopt_optimizeVertexFetch(&vertices[0], &indices[0], indicesCount, &vertices[0], verticesCount, sizeof(T));
			
			// almost always causes ID discontinuities
			if ( 0.0f < simplify && simplify < 1.0f ) {
				uf::stl::vector<U> indicesSimplified(indicesCount);

				size_t targetIndices = indicesCount * simplify;
				float targetError = 1e-2f / simplify;

				float realError = 0.0f;
				size_t realIndices = meshopt_simplify(&indicesSimplified[0], &indices[0], indicesCount, &meshlet.vertices[0].position.x, verticesCount, sizeof(T), targetError, realError);
			//	size_t realIndices = meshopt_simplifySloppy(&indicesSimplified[0], &indices[0], indicesCount, &meshlet.vertices[0].position.x, verticesCount, sizeof(T), targetIndices);
				
				if ( verbose ) UF_MSG_DEBUG("[Simplified] indices: {} -> {} | error: {} -> {}", indicesCount, realIndices, targetError, realError);

				indicesCount = realIndices;
				indices.swap( indicesSimplified );
			}

			meshlet.primitive.drawCommand.indices = indicesCount;
			meshlet.primitive.drawCommand.vertices = verticesCount;
		#endif
			return true;
		}
	}
}