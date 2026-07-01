#pragma once

#include <uf/config.h>
#include <uf/utils/math/shapes.h>
#include <uf/utils/mesh/mesh.h>
#include <uf/utils/mesh/clip.h>
#include <uf/engine/graph/graph.h>
#include <limits>

namespace uf {
	namespace meshgrid {
		struct Node {
			struct {
				pod::Vector3f min = {};
				pod::Vector3f max = {};
			} extents;
			struct {
				pod::Vector3f min = {  std::numeric_limits<float>::max(),  std::numeric_limits<float>::max(),  std::numeric_limits<float>::max() };
				pod::Vector3f max = { -std::numeric_limits<float>::max(), -std::numeric_limits<float>::max(), -std::numeric_limits<float>::max() };
			} effectiveExtents;

			pod::Vector3ui id = {};
			uf::stl::unordered_map<size_t, uf::Meshlet_T<>> meshlets;
		};
		struct Grid {
			pod::Vector3ui divisions = {1,1,1};
			int indices = 0;

			struct {
				pod::Vector3f min = {  std::numeric_limits<float>::max(),  std::numeric_limits<float>::max(),  std::numeric_limits<float>::max() };
				pod::Vector3f max = { -std::numeric_limits<float>::max(), -std::numeric_limits<float>::max(), -std::numeric_limits<float>::max() };
				
				pod::Vector3f center = {};
				pod::Vector3f corner = {};
				
				pod::Vector3f size = {};
				pod::Vector3f piece = {};
			} extents;

		//	uf::stl::vector<Node> nodes;
			uf::stl::unordered_map<pod::Vector3ui, Node> nodes;
		};

		void UF_API print( const Grid& );
		void UF_API cleanup( Grid& );

		void UF_API calculate( uf::meshgrid::Grid&, float = std::numeric_limits<float>::epsilon() );
		void UF_API calculate( uf::meshgrid::Grid&, int, float = std::numeric_limits<float>::epsilon() );
		void UF_API calculate( uf::meshgrid::Grid&, const pod::Vector3ui&, float = std::numeric_limits<float>::epsilon() );

		void UF_API partition( uf::meshgrid::Grid&, const void*, size_t, const void*, size_t, const pod::Primitive& );
		void UF_API partition( uf::meshgrid::Grid&, uf::Mesh&, const pod::Primitive& );
		
		template<typename T, typename U = uint32_t>
		void UF_API partition( uf::meshgrid::Grid& grid, const uf::stl::vector<T>& vertices, const uf::stl::vector<U>& indices, const pod::Primitive& primitive ) {
			auto vertexDescriptor = T::descriptor.front();

			for ( auto& descriptor : T::descriptor ) if ( descriptor.name == "position" ) { vertexDescriptor = descriptor; break; }
			UF_ASSERT( vertexDescriptor.name == "position" );

			return partition( grid, 
				vertices.data(), sizeof(T),
				indices.data(), sizeof(U),
				primitive
			);
		}

		template<typename T, typename U = uint32_t>
		uf::stl::vector<uf::Meshlet_T<T,U>> UF_API partition(
			uf::meshgrid::Grid& grid,
			const uf::stl::vector<uf::Meshlet_T<T,U>>& meshlets,
			float eps = std::numeric_limits<float>::epsilon(),
			bool clip = false,
			bool cleanup = false
		) {
			size_t atlasID = 0;

			uf::stl::vector<uf::Meshlet_T<T,U>> partitioned;
			for ( auto& meshlet : meshlets ) {
				grid.extents.min = uf::vector::min( grid.extents.min, meshlet.primitive.instance.bounds.min );
				grid.extents.max = uf::vector::max( grid.extents.max, meshlet.primitive.instance.bounds.max );
			}

			uf::meshgrid::calculate( grid, eps );

			for ( auto& meshlet : meshlets ) {
				uf::meshgrid::partition<T,U>( grid, meshlet.vertices, meshlet.indices, meshlet.primitive );
			}
			if ( cleanup ) {
				uf::meshgrid::cleanup( grid );
			}

			for ( auto& [ _, node ] : grid.nodes ) {
				++atlasID;
				for ( auto& [ __, mlet ] : node.meshlets ) {
					if ( mlet.indices.empty() ) continue;

					auto& meshlet = meshlets[mlet.primitive.instance.primitiveID];
					
					size_t primitiveID = partitioned.size();
					auto& slice = partitioned.emplace_back();

					uf::stl::unordered_map<U, U> indexMap;
					for ( U globalID : mlet.indices ) {
						if ( indexMap.find(globalID) == indexMap.end() ) {
							indexMap[globalID] = (U)(slice.vertices.size());
							slice.vertices.emplace_back(meshlet.vertices[globalID]);
						}
						slice.indices.emplace_back( indexMap[globalID] );
					}

					if ( clip ) {
						node.effectiveExtents.min = node.extents.min;
						node.effectiveExtents.max = node.extents.max;
						uf::shapes::clip<T,U>( slice.vertices, slice.indices, pod::AABB{ node.extents.min, node.extents.max } );
						// blender outputs poopy tangents that don't get fixed here
						// better to recalculate them before slicing anyways
						// uf::mesh::tangents<T,U>( slice.vertices, slice.indices );
					}

					slice.primitive.instance = meshlet.primitive.instance;
					slice.primitive.instance.materialID = meshlet.primitive.instance.materialID;
					slice.primitive.instance.primitiveID = primitiveID;
					slice.primitive.instance.meshID = meshlet.primitive.instance.meshID;
					slice.primitive.instance.objectID = 0;
					slice.primitive.instance.auxID = atlasID;
					slice.primitive.instance.bounds.min = node.effectiveExtents.min;
					slice.primitive.instance.bounds.max = node.effectiveExtents.max;
					slice.primitive.instance.bounds.center = (node.effectiveExtents.max + node.effectiveExtents.min) * 0.5f;
					slice.primitive.instance.bounds.extent = (node.effectiveExtents.max - node.effectiveExtents.min) * 0.5f;

					slice.primitive.drawCommand.indices = slice.indices.size();
					slice.primitive.drawCommand.instances = 1;
					slice.primitive.drawCommand.indexID = 0;
					slice.primitive.drawCommand.vertexID = 0;
					slice.primitive.drawCommand.instanceID = 0;
					slice.primitive.drawCommand.auxID = atlasID; // meshlet.primitive.instance.meshID;
					slice.primitive.drawCommand.vertices = slice.vertices.size();
				}
			}

			return partitioned;
		}
	}
}