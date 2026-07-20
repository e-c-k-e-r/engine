#include <uf/ext/ttlg/bin.h>
#include <uf/ext/ttlg/common.h>
#include <uf/ext/valve/common.h>
#include <uf/ext/zlib/zlib.h>
#include <uf/utils/memory/reader.h>

namespace impl {
#pragma pack(push, 1)
	struct BinMainHeader {
		char magic[4];
		uint32_t version;
	};

	struct BinHeader {
		char name[8];
		float sphere_rad;
		float max_poly_rad;

		pod::Vector3f bmax;
		pod::Vector3f bmin;
		pod::Vector3f parent_cen;

		uint16_t num_polys;
		uint16_t num_verts;
		uint16_t num_parms;

		uint8_t num_mats;
		uint8_t num_vcalls;
		uint8_t num_vhots;
		uint8_t num_objs;

		uint32_t offset_objs;
		uint32_t offset_mats;
		uint32_t offset_uv;
		uint32_t offset_vhots;
		uint32_t offset_verts;
		uint32_t offset_light;
		uint32_t offset_norms;
		uint32_t offset_poly_list;
		uint32_t offset_nodes;
		uint32_t model_size;

		// version 4 has extra properties
	};

	struct BinMaterial {
		char name[16];
		uint8_t type;
		uint8_t slot_num;
		uint32_t handle_or_color;
		float uvscale_or_ipal;
	};

	struct BinVertex {
		pod::Vector3f position;
	};

	struct BinUV {
		pod::Vector2f uv;
	};

	struct BinPolyHeader {
		uint16_t index;
		uint16_t data;
		uint8_t type;
		uint8_t num_verts;
		uint16_t norm_index;
		float d;
	};

	struct SubObjTransform {
		int32_t parent;
		float min_range;
		float max_range;
		float rot[9];
		pod::Vector3f axle_point;
	};

	struct SubObjectHeader {
		char name[8];
		uint8_t movement;
		SubObjTransform trans;
		int16_t child_sub_obj;
		int16_t next_sub_obj;
		int16_t vhot_start;
		int16_t sub_num_vhots;
		int16_t point_start;
		int16_t sub_num_points;
		int16_t light_start;
		int16_t sub_num_lights;
		int16_t norm_start;
		int16_t sub_num_norms;
		int16_t node_start;
		int16_t sub_num_nodes;
	};
#pragma pack(pop)
}

namespace impl {
	void computeTransforms( uf::stl::vector<impl::SubObjectHeader>& subObjects, uf::stl::vector<pod::Matrix4f>& transforms, uf::stl::vector<pod::Vector3f>& offsets, int16_t nodeIdx = 0, int16_t parentIdx = -1 ) {
		if ( nodeIdx < 0 || nodeIdx >= subObjects.size() ) return;

		const auto& subObj = subObjects[nodeIdx];
		pod::Vector3f offset = subObj.trans.axle_point;
		pod::Matrix4f rot = uf::matrix::identity();

		bool hasRot = false;
		for ( auto i = 0; i < 9; ++i) if ( std::abs(subObj.trans.rot[i]) > 0.0001f ) { hasRot = true; break; }
		// to-do: verify if this is correct
		if ( hasRot ) {
			rot(0,0) = subObj.trans.rot[0]; rot(0,1) = subObj.trans.rot[1]; rot(0,2) = subObj.trans.rot[2];
			rot(1,0) = subObj.trans.rot[3]; rot(1,1) = subObj.trans.rot[4]; rot(1,2) = subObj.trans.rot[5];
			rot(2,0) = subObj.trans.rot[6]; rot(2,1) = subObj.trans.rot[7]; rot(2,2) = subObj.trans.rot[8];
		}

		if ( parentIdx >= 0 ) {
			transforms[nodeIdx] = transforms[parentIdx] * rot;
			offsets[nodeIdx] = uf::matrix::multiply<float>( transforms[parentIdx], offset, 1.0f ) + offsets[parentIdx];
		} else {
			transforms[nodeIdx] = rot;
			offsets[nodeIdx] = offset;
		}

		if ( subObj.child_sub_obj >= 0 ) computeTransforms( subObjects, transforms, offsets, subObj.child_sub_obj, nodeIdx );
		if ( subObj.next_sub_obj >= 0 ) computeTransforms( subObjects, transforms, offsets, subObj.next_sub_obj, parentIdx );
	};
}

bool ext::ttlg::loadBin( pod::Graph& graph, const uf::stl::string& filename ) {
	uf::stl::vector<uint8_t> buffer;
	if ( !uf::io::exists( filename ) ) {
		UF_MSG_ERROR("BIN does not exist: {}", filename);
		return false;
	}
	if ( !uf::io::readAsBuffer( buffer, filename ) ) {
		UF_MSG_ERROR("Failed to read BIN data: {}", filename);
		return false;
	}

	uf::stl::reader reader(buffer, 0, buffer.size());

	const auto* pMainHeader = reader.read<impl::BinMainHeader>();
	if ( !pMainHeader ) {
		UF_MSG_ERROR("Failed to read BIN header: {}", filename);
		return false;
	}
	impl::BinMainHeader mainHeader = *pMainHeader;

	if ( strncmp(mainHeader.magic, "LGMM", 4) == 0 ) {
		UF_MSG_ERROR("Attempting to read LGMM file as an LMGD: {}", filename);
		return false;
	}

	if ( strncmp(mainHeader.magic, "LGMD", 4) != 0 ) {
		UF_MSG_ERROR("Invalid BIN file magic (Expected LGMD): {}", filename);
		return false;
	}

	const auto* pHeader = reader.read<impl::BinHeader>();
	if ( !pHeader ) {
		UF_MSG_ERROR("Failed to parse BIN header: {}", filename);
		return false;
	}
	impl::BinHeader header = *pHeader;

	// skip additional information (to-do: parse)
	if ( mainHeader.version == 4 ) reader.skip(12);

	if ( header.offset_poly_list == 0 || header.offset_poly_list >= buffer.size() ) {
		UF_MSG_ERROR("Invalid BIN file (poly list offset out of bounds): {}", filename);
		return false;
	}

	auto& storage = uf::graph::getStorage( graph );
	uf::stl::unordered_map<int32_t, impl::Meshlet> meshlets;
	uf::stl::vector<int32_t> textureToMaterialId( header.num_mats, -1 );

	uf::stl::vector<impl::BinVertex> vertices;
	uf::stl::vector<impl::BinUV> uvs;
	uf::stl::vector<pod::Vector3f> normals;
	uf::stl::vector<impl::SubObjectHeader> subObjects;
	uf::stl::vector<uf::stl::vector<uint16_t>> subObjectPolys;

	size_t numUVs = 0;
	size_t numNormals = 0;

	// read materials
	if ( header.num_mats > 0 && header.offset_mats > 0 && header.offset_mats < buffer.size() ) {
		uf::stl::reader matReader(buffer, header.offset_mats, buffer.size() - header.offset_mats);
		uf::stl::vector<impl::BinMaterial> binMats;

		if ( matReader.read(header.num_mats, binMats ) ) {
			for ( uint32_t i = 0; i < header.num_mats; ++i ) {
				uf::stl::string matName = uf::stl::string(binMats[i].name);
				if ( matName.empty() ) {
					matName = "missing_texture";
				} else {
					std::transform(matName.begin(), matName.end(), matName.begin(), ::tolower);
					std::replace(matName.begin(), matName.end(), '\\', '/');

					size_t dotPos = matName.find_last_of('.');
					if ( dotPos != uf::stl::string::npos ) matName = matName.substr(0, dotPos);
				}

				auto it = std::find(graph.materials.begin(), graph.materials.end(), matName);
				int32_t matIndex = (mainHeader.version == 4) ? i : binMats[i].slot_num;

				if ( it != graph.materials.end() ) {
					textureToMaterialId[matIndex] = (int32_t)(std::distance(graph.materials.begin(), it));
				} else {
					textureToMaterialId[matIndex] = graph.materials.size();
					graph.materials.emplace_back(matName);
					storage.materials[matName].indexAlbedo = -1;
				}
			}
		}
	}

	// read vertices
	if ( header.num_verts > 0 && header.offset_verts > 0 && header.offset_verts < buffer.size() ) {
		uf::stl::reader vertReader(buffer, header.offset_verts, buffer.size() - header.offset_verts);
		vertReader.read(header.num_verts, vertices);
	}
	// read UVs
	if ( header.offset_uv > 0 && header.offset_uv < buffer.size() ) {
		numUVs = (buffer.size() - header.offset_uv) / sizeof(impl::BinUV);
	}
	if ( numUVs > 0 ) {
		uf::stl::reader uvReader(buffer, header.offset_uv, buffer.size() - header.offset_uv);
		uvReader.read(numUVs, uvs);
	}
	// read normals
	if ( header.offset_norms > 0 && header.offset_norms < buffer.size() ) {
		numNormals = (buffer.size() - header.offset_norms) / sizeof(pod::Vector3f);
	}
	if ( numNormals > 0 ) {
		uf::stl::reader normReader(buffer, header.offset_norms, buffer.size() - header.offset_norms);
		normReader.read(numNormals, normals);
	}
	// read subobject information
	if ( header.num_objs > 0 && header.offset_objs > 0 && header.offset_objs < buffer.size() ) {
		uf::stl::reader objReader(buffer, header.offset_objs, buffer.size() - header.offset_objs);
		objReader.read(header.num_objs, subObjects);
	}

	uf::stl::vector<pod::Matrix4f> transforms( subObjects.size(), uf::matrix::identity() );
	uf::stl::vector<pod::Vector3f> offsets( subObjects.size() );

	// read transforms
	if ( !subObjects.empty() ) impl::computeTransforms( subObjects, transforms, offsets );

	// read subobject faces
	if ( header.offset_nodes > 0 && header.offset_nodes < buffer.size() ) {
		uf::stl::reader nodeReader(buffer, header.offset_nodes, buffer.size() - header.offset_nodes);
		uf::stl::vector<uint16_t> faces;

		while ( !nodeReader.eof() ) {
			faces.clear();

			const auto* pNodeType = nodeReader.peek<uint8_t>();
			if (!pNodeType) break;
			uint8_t nodeType = *pNodeType;

			if ( nodeType == 4 ) {
				subObjectPolys.emplace_back();
				nodeReader.skip(3);
			} else if ( nodeType == 3 ) {
				nodeReader.skip(19);
			} else if ( nodeType == 2 ) {
				nodeReader.skip(17);

				const auto* pNf1 = nodeReader.read<uint16_t>();
				if (!pNf1) break;
				uint16_t nf1 = *pNf1;

				nodeReader.skip(2);
				const auto* pNf2 = nodeReader.read<uint16_t>();
				if (!pNf2) break;
				uint16_t nf2 = *pNf2;

				if ( nodeReader.read(nf1 + nf2, faces) && !subObjectPolys.empty() ) {
					subObjectPolys.back().insert( subObjectPolys.back().end(), faces.begin(), faces.end() );
				}
			} else if (nodeType == 1) {
				nodeReader.skip(17);

				const auto* pNf1 = nodeReader.read<uint16_t>();
				if (!pNf1) break;
				uint16_t nf1 = *pNf1;

				nodeReader.skip(10);
				const auto* pNf2 = nodeReader.read<uint16_t>();
				if (!pNf2) break;
				uint16_t nf2 = *pNf2;

				if ( nodeReader.read(nf1 + nf2, faces) && !subObjectPolys.empty() ) {
					subObjectPolys.back().insert( subObjectPolys.back().end(), faces.begin(), faces.end() );
				}
			} else if (nodeType == 0) {
				nodeReader.skip(17);

				const auto* pNf = nodeReader.read<uint16_t>();
				if (!pNf) break;
				uint16_t nf = *pNf;

				if ( nodeReader.read(nf, faces) && !subObjectPolys.empty() ) {
					subObjectPolys.back().insert(subObjectPolys.back().end(), faces.begin(), faces.end());
				}
			} else {
				break;
			}
		}
	}

	// iterate subobjects
	size_t numParsedSubs = std::min(subObjects.size(), subObjectPolys.size());
	for ( size_t objIdx = 0; objIdx < numParsedSubs; ++objIdx ) {
		const auto& subObj = subObjects[objIdx];
		const auto& polyOffsets = subObjectPolys[objIdx];

		pod::Matrix4f transform = transforms[objIdx];
		pod::Vector3f offsetPos = offsets[objIdx];

		for ( uint16_t polyOffset : polyOffsets ) {
			uint32_t absolutePolyOffset = header.offset_poly_list + polyOffset;
			if (absolutePolyOffset >= buffer.size()) continue;

			uf::stl::reader polyReader(buffer, absolutePolyOffset, buffer.size() - absolutePolyOffset);

			const auto* pPolyHeader = polyReader.read<impl::BinPolyHeader>();
			if (!pPolyHeader) continue;
			impl::BinPolyHeader polyHeader = *pPolyHeader;

			uf::stl::vector<uint16_t> vertIndices;
			polyReader.read(polyHeader.num_verts, vertIndices);

			uf::stl::vector<uint16_t> normIndices;
			polyReader.read(polyHeader.num_verts, normIndices);

			uf::stl::vector<uint16_t> uvIndices;
			bool hasUVs = ((polyHeader.type & 3) == 3);
			if (hasUVs) polyReader.read(polyHeader.num_verts, uvIndices);

			uint32_t localMatID = 0;
			if ( mainHeader.version == 4 ) {
				const auto* matByte = polyReader.read<uint8_t>();
				if (matByte) localMatID = *matByte;
			} else {
				localMatID = polyHeader.data;
			}

			int32_t graphMatID = -1;
			if ( localMatID < textureToMaterialId.size() ) {
				graphMatID = textureToMaterialId[localMatID];
			}
			if ( graphMatID == -1 ) {
				graphMatID = 0;
			}

			auto& meshlet = meshlets[graphMatID];
			meshlet.primitive.instance.materialID = graphMatID;

			uint32_t startVertIdx = meshlet.vertices.size();
			pod::Vector3f polyNormal = {0.f, 1.f, 0.f};
			if (polyHeader.norm_index < normals.size()) polyNormal = normals[polyHeader.norm_index];
			polyNormal = uf::matrix::multiply<float>(transforms[objIdx], polyNormal, 0.0f);

			for (uint8_t v = 0; v < polyHeader.num_verts; ++v) {
				auto& vert = meshlet.vertices.emplace_back();

				uint16_t vIdx = 0;
				if (v < vertIndices.size()) vIdx = vertIndices[v]; // Bounds safety

				if ( vIdx < vertices.size() ) {
					vert.position = impl::convertPos_NewDark( uf::matrix::multiply<float>(transforms[objIdx], vertices[vIdx].position, 1.0f) + offsets[objIdx] );
				}
				vert.normal = impl::convertPos_NewDark(polyNormal, 1.0f);
				vert.color = { 255, 255, 255, 255 };

				if ( hasUVs && v < uvIndices.size() ) {
					uint16_t uvIdx = uvIndices[v];
					if ( uvIdx < uvs.size() ) vert.uv = uvs[uvIdx].uv;
				}
			}

			// triangle fan => triangles
			if (polyHeader.num_verts >= 3) {
				for ( uint8_t v = 1; v < polyHeader.num_verts - 1; ++v ) {
					meshlet.indices.emplace_back(startVertIdx);
					meshlet.indices.emplace_back(startVertIdx + v);
					meshlet.indices.emplace_back(startVertIdx + v + 1);
				}
			}
		}
	}

	if ( meshlets.empty() ) {
		if ( header.num_vhots > 0 || header.num_objs > 0 || header.num_polys == 0 ) {
			UF_MSG_DEBUG("BIN file acts as a dummy node (no polygons, but has VHOTs/Objs): {}", filename);
		} else {
			UF_MSG_WARNING("BIN file contained no valid polygons: {}", filename);
		}
		return false;
	}

	uf::stl::string meshName = filename;
	graph.meshes.emplace_back(meshName);
	graph.primitives.emplace_back(meshName);

	auto& mesh = storage.meshes[meshName];
	auto& primitives = storage.primitives[meshName];

	size_t primitiveID = 0;
	for ( auto& [matID, meshlet] : meshlets ) {
		meshlet.primitive.drawCommand.indices = meshlet.indices.size();
		meshlet.primitive.drawCommand.vertices = meshlet.vertices.size();
		meshlet.primitive.instance.materialID = matID;
		meshlet.primitive.instance.primitiveID = primitiveID++;
		meshlet.primitive.instance.bounds = uf::mesh::bounds( meshlet.vertices );

		uf::mesh::tangents( meshlet.vertices, meshlet.indices );
	}

	mesh.compile( meshlets, primitives );

	return true;
}