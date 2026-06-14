#include <uf/ext/valve/bsp.h>
#include <uf/ext/valve/mdl.h>
#include <uf/ext/valve/vtf.h>
#include <uf/ext/valve/vpk.h>
#include <uf/ext/valve/common.h>

namespace impl {
#pragma pack(push, 1)
	struct mstudiomesh_t {
		int32_t material;
		int32_t modelindex;
		int32_t numvertices;
		int32_t vertexoffset;
		int32_t flexes;
		int32_t flexindex;
		int32_t materialtype;
		int32_t materialparam;
		int32_t meshid;
		pod::Vector3f center;
		int32_t vertexdata_ptr;
		int32_t vertexdata_lod[8];
		uint8_t pad[32];
	};

	struct mstudiomodel_t {
		char name[64];
		int32_t type;
		float boundingradius;
		int32_t nummeshes;
		int32_t meshindex;
		int32_t numvertices;
		int32_t vertexindex;
		int32_t tangentsindex;
		uint8_t pad[56];
	};

	struct mstudiobodyparts_t {
		int32_t sznameindex;
		int32_t nummodels;
		int32_t base;
		int32_t modelindex;
	};

	struct vertexFileHeader_t {
		int32_t magic;
		int32_t version;
		int32_t checksum;
		int32_t numLODs;
		int32_t numLODVertexes[8];
		int32_t numFixups;
		int32_t fixupTableStart;
		int32_t vertexDataStart;
		int32_t tangentDataStart;
	};

	struct vertexFileFixup_t {
		int32_t lod;
		int32_t sourceVertexID;
		int32_t numVertexes;
	};

	struct mstudioboneweight_t {
		float weight[3];
		int8_t bone[3];
		uint8_t numbones;
	};

	struct mstudiovertex_t {
		mstudioboneweight_t m_BoneWeights;
		pod::Vector3f m_vecPosition;
		pod::Vector3f m_vecNormal;
		pod::Vector2f m_vecTexCoord;
	};

	struct studiohdr_t {
		int32_t magic; // "IDST" (0x54534449)
		int32_t version;
		int32_t checksum;
		char name[64];
		int32_t dataLength;

		pod::Vector3f eyeposition;
		pod::Vector3f illumposition;
		pod::Vector3f hull_min, hull_max;
		pod::Vector3f view_bbmin, view_bbmax;

		int32_t flags;
		int32_t numbones;
		int32_t boneindex;
		int32_t numbonecontrollers;
		int32_t bonecontrollerindex;
		int32_t numhitboxsets;
		int32_t hitboxsetindex;
		int32_t numlocalanim;
		int32_t localanimindex;
		int32_t numlocalseq;
		int32_t localseqindex;
		int32_t activitylistversion;
		int32_t eventsindexed;
		int32_t numtextures;
		int32_t textureindex;
		int32_t numcdtextures;
		int32_t cdtextureindex;
		int32_t numskinref;
		int32_t numskinfamilies;
		int32_t skinindex;
		int32_t numbodyparts;
		int32_t bodypartindex;
		int32_t numlocalattachments;
        int32_t localattachmentindex;
        int32_t numlocalnodes;
        int32_t localnodeindex;
        int32_t localnodenameindex;
        int32_t numflexdesc;
        int32_t flexdescindex;
        int32_t numflexcontrollers;
        int32_t flexcontrollerindex;
        int32_t numflexrules;
        int32_t flexruleindex;
        int32_t numikchains;
        int32_t ikchainindex;
        int32_t nummouths;
        int32_t mouthindex;
        int32_t numlocalposeparameters;
        int32_t localposeparamindex;
        int32_t surfacepropindex;
        int32_t keyvalueindex;
        int32_t keyvaluesize;
        int32_t numlocalikautoplaylocks;
        int32_t localikautoplaylockindex;

        float mass;
        int32_t contents;

		// etc
	};

	struct vtxVertex_t {
		uint8_t boneWeightIndex[3];
		uint8_t numBones;
		uint16_t origMeshVertID;
		int8_t boneID[3];
	};

	struct vtxStrip_t {
		int32_t numIndices;
		int32_t indexOffset;
		int32_t numVerts;
		int32_t vertOffset;
		int16_t numBones;
		uint8_t flags;
		int32_t numBoneStateChanges;
		int32_t boneStateChangeOffset;
	};

	struct vtxStripGroup_t {
		int32_t numVerts;
		int32_t vertOffset;
		int32_t numIndices;
		int32_t indexOffset;
		int32_t numStrips;
		int32_t stripOffset;
		uint8_t flags;
	};

	struct vtxMesh_t {
		int32_t numStripGroups;
		int32_t stripGroupHeaderOffset;
		uint8_t flags;
	};

	struct vtxModelLOD_t {
		int32_t numMeshes;
		int32_t meshOffset;
		float switchPoint;
	};

	struct vtxModel_t {
		int32_t numLODs;
		int32_t lodOffset;
	};

	struct vtxBodyPart_t {
		int32_t numModels;
		int32_t modelOffset;
	};

	struct vtxHeader_t {
		int32_t version;
		int32_t vertCacheSize;
		uint16_t maxBonesPerStrip;
		uint16_t maxBonesPerTri;
		int32_t maxBonesPerVert;
		int32_t checksum;
		int32_t numLODs;
		int32_t materialReplacementListOffset;
		int32_t numBodyParts;
		int32_t bodyPartOffset;
	};
#pragma pack(pop)
}

bool ext::valve::loadMdl( pod::Graph& graph, const uf::stl::string& filename ) {
	auto& storage = uf::graph::getStorage( graph );
	uf::stl::vector<impl::Meshlet> meshlets;

	// Read MDL file
	uf::stl::vector<uint8_t> mdlBuffer;
	if ( !uf::io::readAsBuffer(mdlBuffer, filename) ) {
		UF_MSG_ERROR("Failed to find MDL: {}", filename);
		return false;
	}

	const impl::studiohdr_t* mdlHdr = (const impl::studiohdr_t*)mdlBuffer.data();
	if ( mdlHdr->magic != 0x54534449 ) { // "IDST"
		UF_MSG_ERROR("Invalid MDL magic: {}", filename);
		return false;
	}

	// Read metadata
	graph.metadata["valve"]["models"][filename]["mass"] = mdlHdr->mass;

	// Read VVD file
	uf::stl::string vvdPath = filename.substr(0, filename.find_last_of('.')) + ".vvd";
	uf::stl::vector<uint8_t> vvdBuffer;
	if ( !uf::io::readAsBuffer(vvdBuffer, vvdPath) ) {
		UF_MSG_ERROR("Failed to find VVD: {}", vvdPath);
		return false;
	}

	const impl::vertexFileHeader_t* vvdHdr = (const impl::vertexFileHeader_t*)vvdBuffer.data();
	if ( vvdHdr->magic != 0x56534449 || vvdHdr->checksum != mdlHdr->checksum ) {
		UF_MSG_ERROR("VVD magic/checksum mismatch for: {}", vvdPath);
		return false;
	}

	// extract material directories (cdtextures)
	uf::stl::vector<uf::stl::string> cdmaterials(mdlHdr->numcdtextures);
	for ( int i = 0; i < mdlHdr->numcdtextures; ++i ) {
		int32_t cdOffset = *(int32_t*)(mdlBuffer.data() + mdlHdr->cdtextureindex + (i * 4));
		uf::stl::string cdPath = (const char*)(mdlBuffer.data() + cdOffset);

		std::replace(cdPath.begin(), cdPath.end(), '\\', '/');
		std::transform(cdPath.begin(), cdPath.end(), cdPath.begin(), ::tolower);
		cdmaterials[i] = cdPath;
	}

	// extract material names from MDL and resolve their full relative paths
	uf::stl::vector<uf::stl::string> materials(mdlHdr->numtextures);
	for ( int i = 0; i < mdlHdr->numtextures; ++i ) {
		int32_t texStructOffset = mdlHdr->textureindex + (i * 64);
		int32_t nameOffset = *(int32_t*)(mdlBuffer.data() + texStructOffset);
		uf::stl::string baseName = (const char*)(mdlBuffer.data() + texStructOffset + nameOffset);
		std::transform(baseName.begin(), baseName.end(), baseName.begin(), ::tolower);

		materials[i] = baseName;

		for ( const auto& cd : cdmaterials ) {
			uf::stl::string attempt = cd + baseName;
			if ( uf::vfs::exists("materials/" + attempt + ".vmt") ) {
				materials[i] = attempt;
				break;
			}
		}
	}

	// extract LOD0 vertices from VVD
	const impl::mstudiovertex_t* vvdVertices = (const impl::mstudiovertex_t*)(vvdBuffer.data() + vvdHdr->vertexDataStart);
	const pod::Vector4f* vvdTangents = nullptr;
	if ( vvdHdr->tangentDataStart != 0 ) vvdTangents = (const pod::Vector4f*)(vvdBuffer.data() + vvdHdr->tangentDataStart);

	uf::stl::vector<impl::mstudiovertex_t> lod0Vertices;
	uf::stl::vector<pod::Vector4f> lod0Tangents;

	if ( vvdHdr->numFixups == 0 ) {
		lod0Vertices.assign(vvdVertices, vvdVertices + vvdHdr->numLODVertexes[0]);
		if (vvdTangents) lod0Tangents.assign(vvdTangents, vvdTangents + vvdHdr->numLODVertexes[0]);

	} else {
		lod0Vertices.resize(vvdHdr->numLODVertexes[0]);
		if (vvdTangents) lod0Tangents.resize(vvdHdr->numLODVertexes[0]);

		const impl::vertexFileFixup_t* fixups = (const impl::vertexFileFixup_t*)(vvdBuffer.data() + vvdHdr->fixupTableStart);
		int targetIndex = 0;
		for ( int i = 0; i < vvdHdr->numFixups; ++i ) {
			if ( fixups[i].lod >= 0 ) {
				std::memcpy(&lod0Vertices[targetIndex], &vvdVertices[fixups[i].sourceVertexID], fixups[i].numVertexes * sizeof(impl::mstudiovertex_t));
				if ( vvdTangents ) std::memcpy(&lod0Tangents[targetIndex], &vvdTangents[fixups[i].sourceVertexID], fixups[i].numVertexes * sizeof(pod::Vector4f));
				targetIndex += fixups[i].numVertexes;
			}
		}
	}
	/*
	*/

	// read VTX file
	uf::stl::string vtxPath = filename.substr(0, filename.find_last_of('.')) + ".dx90.vtx";
	uf::stl::vector<uint8_t> vtxBuffer;
	if ( !uf::io::readAsBuffer(vtxBuffer, vtxPath)) return false;

	const impl::vtxHeader_t* vtxHdr = (const impl::vtxHeader_t*)vtxBuffer.data();
	if ( vtxHdr->checksum != mdlHdr->checksum ) {
		UF_MSG_ERROR("VTX checksum mismatch for: {}", vtxPath);
		return false;
	}

	// traverse: BodyPart -> Model -> LOD0 -> Mesh -> StripGroup -> Indices
	const impl::vtxBodyPart_t* bodyParts = (const impl::vtxBodyPart_t*)(vtxBuffer.data() + vtxHdr->bodyPartOffset);
	const impl::mstudiobodyparts_t* mdlBodyParts = (const impl::mstudiobodyparts_t*)((uint8_t*)mdlHdr + mdlHdr->bodypartindex);

	for ( int bp = 0; bp < vtxHdr->numBodyParts; ++bp ) {
		const impl::vtxModel_t* models = (const impl::vtxModel_t*)((uint8_t*)&bodyParts[bp] + bodyParts[bp].modelOffset);
		const impl::mstudiomodel_t* mdlModels = (const impl::mstudiomodel_t*)((uint8_t*)&mdlBodyParts[bp] + mdlBodyParts[bp].modelindex);

		for ( int m = 0; m < bodyParts[bp].numModels; ++m ) {
			const impl::vtxModelLOD_t* lods = (const impl::vtxModelLOD_t*)((uint8_t*)&models[m] + models[m].lodOffset);
			const impl::vtxModelLOD_t& lod0 = lods[0];

			const impl::vtxMesh_t* meshes = (const impl::vtxMesh_t*)((uint8_t*)&lod0 + lod0.meshOffset);
			const impl::mstudiomesh_t* mdlMeshes = (const impl::mstudiomesh_t*)((uint8_t*)&mdlModels[m] + mdlModels[m].meshindex);

			for ( int meshID = 0; meshID < lod0.numMeshes; ++meshID ) {
				const impl::vtxMesh_t& mesh = meshes[meshID];
				const impl::mstudiomesh_t& mdlMesh = mdlMeshes[meshID];

				auto& meshlet = meshlets.emplace_back();
				uf::stl::unordered_map<uint16_t, uint32_t> vertRemap;

				const impl::vtxStripGroup_t* stripGroups = (const impl::vtxStripGroup_t*)((uint8_t*)&mesh + mesh.stripGroupHeaderOffset);
				for ( int sg = 0; sg < mesh.numStripGroups; ++sg ) {
					const impl::vtxStripGroup_t& stripGroup = stripGroups[sg];

					const uint16_t* indices = (const uint16_t*)((uint8_t*)&stripGroup + stripGroup.indexOffset);
					const impl::vtxVertex_t* vtxVerts = (const impl::vtxVertex_t*)((uint8_t*)&stripGroup + stripGroup.vertOffset);

					for ( int i = 0; i < stripGroup.numIndices; i += 3 ) {
						uint32_t tri[3];
						for ( int j = 0; j < 3; ++j ) {
							uint16_t localVertIndex = indices[i + j];
							const impl::vtxVertex_t& vtxVert = vtxVerts[localVertIndex];
							uint16_t originalVvdID = vtxVert.origMeshVertID;

							if ( vertRemap.find(originalVvdID) == vertRemap.end() ) {
								vertRemap[originalVvdID] = meshlet.vertices.size();
								auto& vert = meshlet.vertices.emplace_back();

								const auto& srcVert = lod0Vertices[mdlMesh.vertexoffset + originalVvdID];

								vert.position = impl::convertPos( srcVert.m_vecPosition );
								vert.normal = uf::vector::normalize( impl::convertPos( srcVert.m_vecNormal, 1.0f ) );
								if ( !lod0Tangents.empty() ) {
									vert.tangent = uf::vector::normalize( impl::convertPos( lod0Tangents[mdlMesh.vertexoffset + originalVvdID], 1.0f ) );
								} else {
									vert.tangent = {};
								}

								vert.uv = srcVert.m_vecTexCoord;
								vert.color = {1.0f, 1.0f, 1.0f, 1.0f};
								vert.joints.x = srcVert.m_BoneWeights.numbones > 0 ? std::max<int8_t>(0, srcVert.m_BoneWeights.bone[0]) : 0;
								vert.joints.y = srcVert.m_BoneWeights.numbones > 1 ? std::max<int8_t>(0, srcVert.m_BoneWeights.bone[1]) : 0;
								vert.joints.z = srcVert.m_BoneWeights.numbones > 2 ? std::max<int8_t>(0, srcVert.m_BoneWeights.bone[2]) : 0;
								vert.joints.w = 0;

								vert.weights.x = srcVert.m_BoneWeights.numbones > 0 ? srcVert.m_BoneWeights.weight[0] : 1.0f;
								vert.weights.y = srcVert.m_BoneWeights.numbones > 1 ? srcVert.m_BoneWeights.weight[1] : 0.0f;
								vert.weights.z = srcVert.m_BoneWeights.numbones > 2 ? srcVert.m_BoneWeights.weight[2] : 0.0f;
								vert.weights.w = 0.0f;

								// Bounds calculation
								auto& bounds = meshlet.primitive.instance.bounds;
								if ( vertRemap.size() == 1 ) {
									bounds.min = bounds.max = vert.position;
								} else {
									bounds.min = uf::vector::min( bounds.min, vert.position );
									bounds.max = uf::vector::max( bounds.max, vert.position );
								}
							}

							meshlet.indices.push_back(tri[j] = vertRemap[originalVvdID]);
						}

						if ( lod0Tangents.empty() ) {
							auto& v0 = meshlet.vertices[tri[0]];
							auto& v1 = meshlet.vertices[tri[1]];
							auto& v2 = meshlet.vertices[tri[2]];

							pod::Vector3f edge1 = v1.position - v0.position;
							pod::Vector3f edge2 = v2.position - v0.position;
							pod::Vector2f duv1 = v1.uv - v0.uv;
							pod::Vector2f duv2 = v2.uv - v0.uv;

							float f = 1.0f / (duv1.x * duv2.y - duv2.x * duv1.y);
							pod::Vector3f tangent = {
								f * (duv2.y * edge1.x - duv1.y * edge2.x),
								f * (duv2.y * edge1.y - duv1.y * edge2.y),
								f * (duv2.y * edge1.z - duv1.y * edge2.z)
							};

							v0.tangent += tangent;
							v1.tangent += tangent;
							v2.tangent += tangent;
						}
					}
				}

				if ( lod0Tangents.empty() ) {
					for ( auto& vert : meshlet.vertices ) {
						vert.tangent = uf::vector::normalize(vert.tangent - vert.normal * uf::vector::dot(vert.normal, vert.tangent));
					}
				}

				size_t materialID = 0;
				uf::stl::string matName = "missing_texture";
				if ( mdlMesh.material < materials.size() ) matName = materials[mdlMesh.material];
				if ( auto it = std::find(graph.materials.begin(), graph.materials.end(), matName); it != graph.materials.end() ) {
					materialID = (int32_t)(std::distance(graph.materials.begin(), it));
				} else {
					int32_t textureID = -1;
					materialID = impl::addMaterial( graph, matName, textureID );
					storage.materials[matName].indexAlbedo = textureID;
				}

				meshlet.primitive.instance.materialID = materialID;
			}
		}
	}

	if ( meshlets.empty() ) {
		UF_MSG_DEBUG("Empty mesh={}", filename);
		return false;
	}

	auto meshName = filename;
	graph.meshes.emplace_back(meshName);
	graph.primitives.emplace_back(meshName);

	auto& mesh = storage.meshes[meshName];
	auto& primitives = storage.primitives[meshName];

	mesh.compile( meshlets, primitives );

	return true;
}