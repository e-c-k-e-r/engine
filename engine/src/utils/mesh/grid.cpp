#include <uf/utils/mesh/grid.h>

namespace {
	inline bool isInside( const pod::Vector3f& p, const pod::Vector3f& min, const pod::Vector3f& max ) {
	//	return ( min.x <= p.x && p.x <= max.x && min.y <= p.y && p.y <= max.y && min.z <= p.z && p.z <= max.z );
		return (p.x <= max.x && p.x >= min.x) && (p.y <= max.y && p.y >= min.y) && (p.z <= max.z && p.z >= min.z);
	}

	inline pod::Vector3ui cellCoords(const pod::Vector3f& p, const pod::Vector3f& min, const pod::Vector3f& piece, const pod::Vector3ui& divs) {
		pod::Vector3f rel = (p - min) / piece;
		return {
			std::min<uint32_t>(std::max<int>(0, (int)rel.x), divs.x - 1),
			std::min<uint32_t>(std::max<int>(0, (int)rel.y), divs.y - 1),
			std::min<uint32_t>(std::max<int>(0, (int)rel.z), divs.z - 1)
		};
	}

	namespace {
		// to-do: just use the physics copy of AABB
		inline bool intersectTriangleAABB(const pod::Vector3f& v0, const pod::Vector3f& v1, const pod::Vector3f& v2, const pod::Vector3f& aabbMin, const pod::Vector3f& aabbMax) {
			pod::Vector3f boxCenter = (aabbMin + aabbMax) * 0.5f;
			pod::Vector3f boxExtents = (aabbMax - aabbMin) * 0.5f;

			pod::Vector3f tv0 = v0 - boxCenter;
			pod::Vector3f tv1 = v1 - boxCenter;
			pod::Vector3f tv2 = v2 - boxCenter;

			pod::Vector3f e0 = tv1 - tv0;
			pod::Vector3f e1 = tv2 - tv1;
			pod::Vector3f e2 = tv0 - tv2;

			pod::Vector3f triMin = uf::vector::min(uf::vector::min(tv0, tv1), tv2);
			pod::Vector3f triMax = uf::vector::max(uf::vector::max(tv0, tv1), tv2);
			if (triMin.x > boxExtents.x || triMax.x < -boxExtents.x) return false;
			if (triMin.y > boxExtents.y || triMax.y < -boxExtents.y) return false;
			if (triMin.z > boxExtents.z || triMax.z < -boxExtents.z) return false;

			pod::Vector3f normal = uf::vector::cross(e0, e1);
			float planeD = uf::vector::dot(normal, tv0);
			float r = boxExtents.x * std::abs(normal.x) + boxExtents.y * std::abs(normal.y) + boxExtents.z * std::abs(normal.z);
			if (std::abs(planeD) > r) return false;

			float p0, p1, p2, rad;
			auto testAxis = [&](const pod::Vector3f& axis) {
				p0 = uf::vector::dot(tv0, axis);
				p1 = uf::vector::dot(tv1, axis);
				p2 = uf::vector::dot(tv2, axis);
				rad = boxExtents.x * std::abs(axis.x) + boxExtents.y * std::abs(axis.y) + boxExtents.z * std::abs(axis.z);
				return std::min({p0, p1, p2}) > rad || std::max({p0, p1, p2}) < -rad;
			};

			if (testAxis({0, -e0.z, e0.y})) return false;
			if (testAxis({0, -e1.z, e1.y})) return false;
			if (testAxis({0, -e2.z, e2.y})) return false;

			if (testAxis({e0.z, 0, -e0.x})) return false;
			if (testAxis({e1.z, 0, -e1.x})) return false;
			if (testAxis({e2.z, 0, -e2.x})) return false;

			if (testAxis({-e0.y, e0.x, 0})) return false;
			if (testAxis({-e1.y, e1.x, 0})) return false;
			if (testAxis({-e2.y, e2.x, 0})) return false;

			return true;
		}
	}
}

void uf::meshgrid::print( const uf::meshgrid::Grid& grid ) {
	UF_MSG_DEBUG( "== == == ==");
	float total = 0.0f;
	for ( auto& pair : grid.nodes ) { auto& node = pair.second;
		size_t indices = 0;
		for ( auto& pair2 : node.meshlets ) indices += pair2.second.indices.size();
		float percentage = 100.0f * indices / grid.indices;
		total += percentage;
	}
}
void uf::meshgrid::cleanup( uf::meshgrid::Grid& grid ) {
	uf::stl::vector<pod::Vector3ui> eraseNodes;

	for ( auto& pair : grid.nodes ) { auto& node = pair.second;
		
		uf::stl::vector<size_t> eraseMeshlets;
		for ( auto& pair2 : node.meshlets ) { auto& meshlet = pair2.second;
			if ( !meshlet.indices.empty() ) continue;
			eraseMeshlets.emplace_back(pair2.first);
		}
		for ( auto& e : eraseMeshlets ) node.meshlets.erase(e);
		
		if ( !node.meshlets.empty() ) continue;
		eraseNodes.emplace_back(pair.first);
	}
	
	for ( auto& e : eraseNodes ) grid.nodes.erase(e);
}

void uf::meshgrid::calculate( uf::meshgrid::Grid& grid, int divisions, float padding ){
	grid.divisions = {divisions, divisions, divisions};
	return calculate( grid, padding );
}
void uf::meshgrid::calculate( uf::meshgrid::Grid& grid, const pod::Vector3ui& divisions, float padding ){
	grid.divisions = divisions; 
	return calculate( grid, padding );
}
void uf::meshgrid::calculate( uf::meshgrid::Grid& grid, float padding ){
	grid.extents.min -= pod::Vector3f{ padding, padding, padding };
	grid.extents.max += pod::Vector3f{ padding, padding, padding };

	grid.extents.size = grid.extents.max - grid.extents.min;
	grid.extents.piece = grid.extents.size / grid.divisions;

	grid.nodes.clear();
	grid.nodes.reserve(grid.divisions.x * grid.divisions.y * grid.divisions.z);

	for (uint32_t z = 0; z < grid.divisions.z; ++z) {
		for (uint32_t y = 0; y < grid.divisions.y; ++y) {
			for (uint32_t x = 0; x < grid.divisions.x; ++x) {
				Node node;
				node.id = { x, y, z };
				node.extents.min = grid.extents.min + grid.extents.piece * pod::Vector3f{ x, y, z };
				node.extents.max = node.extents.min + grid.extents.piece;
				grid.nodes[{x,y,z}] = std::move(node);
			}
		}
	}
}

void uf::meshgrid::partition(uf::meshgrid::Grid& grid,
	const void* pPointer, size_t pStride,
	const void* iPointer, size_t iStride,
	const pod::Primitive& primitive)
{
	size_t pFirst = primitive.drawCommand.vertexID;
	size_t pCount = primitive.drawCommand.vertices;
	size_t iFirst = primitive.drawCommand.indexID;
	size_t iCount = primitive.drawCommand.indices;

	struct Triangle {
		pod::Vector3ui indices;
		pod::Vector3f verts[3];
	};

	const uint8_t* idxBase = reinterpret_cast<const uint8_t*>(iPointer);
	const uint8_t* vtxBase = reinterpret_cast<const uint8_t*>(pPointer);

	for (size_t i = 0; i < iCount; i += 3) {
		Triangle tri{};
		auto readIndex = [&](size_t offs) -> uint32_t {
			switch (iStride) {
				case 1: return *(const uint8_t*)(idxBase + iStride * (iFirst + offs));
				case 2: return *(const uint16_t*)(idxBase + iStride * (iFirst + offs));
				default: return *(const uint32_t*)(idxBase + iStride * (iFirst + offs));
			}
		};
		tri.indices[0] = readIndex(i);
		tri.indices[1] = readIndex(i+1);
		tri.indices[2] = readIndex(i+2);

		for (int v = 0; v < 3; v++) {
			tri.verts[v] = *(const pod::Vector3f*)(vtxBase + pStride * (pFirst + tri.indices[v]));
		}

		pod::Vector3f triMin = uf::vector::min(uf::vector::min(tri.verts[0], tri.verts[1]), tri.verts[2]);
		pod::Vector3f triMax = uf::vector::max(uf::vector::max(tri.verts[0], tri.verts[1]), tri.verts[2]);

		pod::Vector3ui minCell = ::cellCoords(triMin, grid.extents.min, grid.extents.piece, grid.divisions);
		pod::Vector3ui maxCell = ::cellCoords(triMax, grid.extents.min, grid.extents.piece, grid.divisions);

		for (uint32_t z = minCell.z; z <= maxCell.z; z++) {
			for (uint32_t y = minCell.y; y <= maxCell.y; y++) {
				for (uint32_t x = minCell.x; x <= maxCell.x; x++) {
					pod::Vector3ui cid{x,y,z};
					auto& node = grid.nodes[cid];

					if ( !intersectTriangleAABB(tri.verts[0], tri.verts[1], tri.verts[2], node.extents.min, node.extents.max) ) {
						continue;
					}

					auto& meshlet = node.meshlets[primitive.instance.primitiveID];
					meshlet.primitive = primitive;

					meshlet.indices.emplace_back(tri.indices[0]);
					meshlet.indices.emplace_back(tri.indices[1]);
					meshlet.indices.emplace_back(tri.indices[2]);

					for (int v = 0; v < 3; v++) {
						node.effectiveExtents.min = uf::vector::min(node.effectiveExtents.min, tri.verts[v]);
						node.effectiveExtents.max = uf::vector::max(node.effectiveExtents.max, tri.verts[v]);
					}
				}
			}
		}
	}

	grid.indices += iCount;
}
void uf::meshgrid::partition( uf::meshgrid::Grid& grid, uf::Mesh& mesh, const pod::Primitive& primitive ) {
	uf::Mesh::Input vertexInput = mesh.vertex;
	uf::Mesh::Input indexInput = mesh.index;

	uf::Mesh::Attribute vertexAttribute = mesh.vertex.attributes.front();
	uf::Mesh::Attribute indexAttribute = mesh.index.attributes.front();

	for ( auto& attribute : mesh.vertex.attributes ) if ( attribute.descriptor.name == "position" ) { vertexAttribute = attribute; break; }
	UF_ASSERT( vertexAttribute.descriptor.name == "position" );

	return partition( grid, 
		static_cast<uint8_t*>(vertexAttribute.pointer) + vertexAttribute.offset, vertexAttribute.stride,
		static_cast<uint8_t*>(indexAttribute.pointer) + indexAttribute.offset, indexAttribute.stride,
		primitive
	);
}