#include <uf/ext/valve/bsp.h>
#include <uf/ext/valve/mdl.h>
#include <uf/ext/valve/vtf.h>
#include <uf/ext/valve/common.h>

namespace impl {
#pragma pack(push, 1)
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

		// etc
	};
#pragma pack(pop)
#pragma pack(push, 1)
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

	// read MDL file
	std::ifstream mdlFile(filename, std::ios::binary | std::ios::ate);
	if ( !mdlFile ) {
		UF_MSG_ERROR("Failed to find MDL: {}", filename);
		return false;
	}
	std::streamsize mdlSize = mdlFile.tellg();
	mdlFile.seekg(0, std::ios::beg);
	uf::stl::vector<uint8_t> mdlBuffer(mdlSize);
	mdlFile.read((char*)mdlBuffer.data(), mdlSize);

	const impl::studiohdr_t* mdlHdr = (const impl::studiohdr_t*)mdlBuffer.data();
	if ( mdlHdr->magic != 0x54534449 ) { // "IDST"
		UF_MSG_ERROR("Invalid MDL magic: {}", filename);
		return false;
	}

	// read VVD file
	uf::stl::string vvdPath = filename.substr(0, filename.find_last_of('.')) + ".vvd";
	std::ifstream vvdFile(vvdPath, std::ios::binary | std::ios::ate);
	if ( !vvdFile ) {
		UF_MSG_ERROR("Failed to find VVD: {}", vvdPath);
		return false;
	}
	std::streamsize vvdSize = vvdFile.tellg();
	vvdFile.seekg(0, std::ios::beg);
	uf::stl::vector<uint8_t> vvdBuffer(vvdSize);
	vvdFile.read((char*)vvdBuffer.data(), vvdSize);

	const impl::vertexFileHeader_t* vvdHdr = (const impl::vertexFileHeader_t*)vvdBuffer.data();
	if ( vvdHdr->magic != 0x56534449 || vvdHdr->checksum != mdlHdr->checksum ) {
		UF_MSG_ERROR("VVD magic/checksum mismatch for: {}", vvdPath);
		return false;
	}

	// extract material names from MDL
	uf::stl::vector<uf::stl::string> materials(mdlHdr->numtextures);
	for ( int i = 0; i < mdlHdr->numtextures; ++i ) {
		int32_t texStructOffset = mdlHdr->textureindex + (i * 64);
		int32_t nameOffset = *(int32_t*)(mdlBuffer.data() + texStructOffset);

		materials[i] = (const char*)(mdlBuffer.data() + texStructOffset + nameOffset);
		UF_MSG_INFO("Model Material {}: {}", i, materials[i]);
	}

	// extract LOD0 ertices from VVD
	const impl::mstudiovertex_t* vvdVertices = (const impl::mstudiovertex_t*)(vvdBuffer.data() + vvdHdr->vertexDataStart);
	int numLOD0Verts = vvdHdr->numLODVertexes[0];

	// read VTX file
	uf::stl::string vtxPath = filename.substr(0, filename.find_last_of('.')) + ".dx90.vtx";
	std::ifstream vtxFile(vtxPath, std::ios::binary | std::ios::ate);
	if ( !vtxFile ) return false;

	std::streamsize vtxSize = vtxFile.tellg();
	vtxFile.seekg(0, std::ios::beg);
	uf::stl::vector<uint8_t> vtxBuffer(vtxSize);
	vtxFile.read((char*)vtxBuffer.data(), vtxSize);

	const impl::vtxHeader_t* vtxHdr = (const impl::vtxHeader_t*)vtxBuffer.data();
	if ( vtxHdr->checksum != mdlHdr->checksum ) {
		UF_MSG_ERROR("VTX checksum mismatch for: {}", vtxPath);
		return false;
	}

	// traverse: BodyPart -> Model -> LOD0 -> Mesh -> StripGroup -> Indices
	const impl::vtxBodyPart_t* bodyParts = (const impl::vtxBodyPart_t*)(vtxBuffer.data() + vtxHdr->bodyPartOffset);

	for ( int bp = 0; bp < vtxHdr->numBodyParts; ++bp ) {
		const impl::vtxModel_t* models = (const impl::vtxModel_t*)((uint8_t*)&bodyParts[bp] + bodyParts[bp].modelOffset);

		for ( int m = 0; m < bodyParts[bp].numModels; ++m ) {
			const impl::vtxModelLOD_t* lods = (const impl::vtxModelLOD_t*)((uint8_t*)&models[m] + models[m].lodOffset);
			const impl::vtxModelLOD_t& lod0 = lods[0];

			const impl::vtxMesh_t* meshes = (const impl::vtxMesh_t*)((uint8_t*)&lod0 + lod0.meshOffset);

			for ( int meshID = 0; meshID < lod0.numMeshes; ++meshID ) {
				const impl::vtxMesh_t& mesh = meshes[meshID];

				auto& meshlet = meshlets.emplace_back();
				uf::stl::unordered_map<uint16_t, uint32_t> vertRemap;

				uf::stl::string matName = "missing_texture";
				if ( meshID < materials.size() ) {
					matName = materials[meshID];
				}

				// does not exist, register
				if ( storage.textures.map.count(matName) == 0 ) {
					size_t imageID = graph.images.size();
					auto imgKeyName = graph.images.emplace_back(matName);
					auto& image = storage.images[imgKeyName];

					size_t textureID = graph.textures.size();
					auto texKeyName = graph.textures.emplace_back(matName);
					storage.textures[texKeyName].index = imageID;
					storage.texture2Ds[texKeyName];

					size_t materialID = graph.materials.size();
					auto matKeyName = graph.materials.emplace_back(matName);
					auto& material = storage.materials[matKeyName];
					material.indexAlbedo = textureID;
					material.colorBase = {1.0f, 1.0f, 1.0f, 1.0f};
				}
				
				// meshlet.primitive.instance.materialID = ...;

				const impl::vtxStripGroup_t* stripGroups = (const impl::vtxStripGroup_t*)((uint8_t*)&mesh + mesh.stripGroupHeaderOffset);

				for ( int sg = 0; sg < mesh.numStripGroups; ++sg ) {
					const impl::vtxStripGroup_t& stripGroup = stripGroups[sg];

					const uint16_t* indices = (const uint16_t*)((uint8_t*)&stripGroup + stripGroup.indexOffset);
					const impl::vtxVertex_t* vtxVerts = (const impl::vtxVertex_t*)((uint8_t*)&stripGroup + stripGroup.vertOffset);

					for ( int i = 0; i < stripGroup.numIndices; ++i ) {
						uint16_t localVertIndex = indices[i];
						const impl::vtxVertex_t& vtxVert = vtxVerts[localVertIndex];
						uint16_t originalVvdID = vtxVert.origMeshVertID;

						if ( vertRemap.find(originalVvdID) == vertRemap.end() ) {
							vertRemap[originalVvdID] = meshlet.vertices.size();
							auto& vert = meshlet.vertices.emplace_back();

							const auto& srcVert = vvdVertices[originalVvdID];

							vert.position = impl::convertPos( srcVert.m_vecPosition, 1.0f );
							vert.normal = uf::vector::normalize( impl::convertPos( srcVert.m_vecNormal ) );
							vert.uv = srcVert.m_vecTexCoord;
							vert.color = {1.0f, 1.0f, 1.0f, 1.0f};

							// to-do: bounds calculation
						}

						meshlet.indices.push_back(vertRemap[originalVvdID]);
					}
				}
			}
		}
	}

	return true;
}