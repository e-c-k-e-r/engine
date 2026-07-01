#include <uf/ext/valve/bsp.h>
#include <uf/ext/valve/mdl.h>
#include <uf/ext/valve/vtf.h>
#include <uf/ext/valve/vpk.h>
#include <uf/ext/valve/common.h>
#include <uf/ext/zlib/zlib.h>

#include <uf/utils/mesh/grid.h>
#include <uf/utils/memory/unordered_set.h>

namespace impl {
	struct RGBE {
		uint8_t r, g, b;
		int8_t e;
	};

	constexpr int VBSP_HEADER_LUMPS = 64;
	struct BspLump {
		enum Type {
			LUMP_ENTITIES = 0,
			LUMP_PLANES = 1,
			LUMP_TEXDATA = 2,
			LUMP_VERTICES = 3,
			LUMP_VISIBILITY = 4,
			LUMP_NODES = 5,
			LUMP_TEXINFO = 6,
			LUMP_FACES = 7,
			LUMP_LIGHTING = 8,
			LUMP_OCCLUSION = 9,
			LUMP_LEAFS = 10,
			LUMP_FACEIDS = 11,
			LUMP_EDGES = 12,
			LUMP_SURFEDGES = 13,
			LUMP_MODELS = 14,
			LUMP_WORLDLIGHTS = 15,
			LUMP_LEAFFACES = 16,
			LUMP_LEAFBRUSHES = 17,
			LUMP_BRUSHES = 18,
			LUMP_BRUSHSIDES = 19,
			LUMP_AREAS = 20,
			LUMP_AREAPORTALS = 21,
			LUMP_UNUSED0 = 22, // LUMP_PORTALS | LUMP_PROPCOLLISION
			LUMP_UNUSED1 = 23, // LUMP_CLUSTERS | LUMP_PROPHULLS
			LUMP_UNUSED2 = 24, // LUMP_PORTALVERTS | LUMP_FAKEENTITIES | LUMP_PROPHULLVERTS
			LUMP_UNUSED3 = 25, // LUMP_CLUSTERPORTALS | LUMP_PROPTRIS
			LUMP_DISPINFO = 26,
			LUMP_ORIGINALFACES = 27,
			LUMP_PHYSDISP = 28,
			LUMP_PHYSCOLLIDE = 29,
			LUMP_VERTNORMALS = 30,
			LUMP_VERTNORMALINDICES = 31,
			LUMP_DISP_LIGHTMAP_ALPHAS = 32,
			LUMP_DISP_VERTS = 33,
			LUMP_DISP_LIGHTMAP_SAMPLE_POSITIONS = 34,
			LUMP_GAME_LUMP = 35,
			LUMP_LEAFWATERDATA = 36,
			LUMP_PRIMITIVES = 37,
			LUMP_PRIMVERTS = 38,
			LUMP_PRIMINDICES = 39,
			LUMP_PAKFILE = 40,
			LUMP_CLIPPORTALVERTS = 41,
			LUMP_CUBEMAPS = 42,
			LUMP_TEXDATA_STRING_DATA = 43,
			LUMP_TEXDATA_STRING_TABLE = 44,
			LUMP_OVERLAYS = 45,
			LUMP_LEAFMINDISTTOWATER = 46,
			LUMP_FACE_MACRO_TEXTURE_INFO = 47,
			LUMP_DISP_TRIS = 48,
			LUMP_PHYSCOLLIDESURFACE = 49, // LUMP_PROP_BLOB
			LUMP_WATEROVERLAYS = 50,
			LUMP_LEAF_AMBIENT_INDEX_HDR = 51, // LUMP_LIGHTMAPPAGES
			LUMP_LEAF_AMBIENT_INDEX = 52, // LUMP_LIGHTMAPPAGEINFOS
			LUMP_LIGHTING_HDR = 53,
			LUMP_WORLDLIGHTS_HDR = 54,
			LUMP_LEAF_AMBIENT_LIGHTING_HDR = 55,
			LUMP_LEAF_AMBIENT_LIGHTING = 56,
			LUMP_XZIPPAKFILE = 57,
			LUMP_FACES_HDR = 58,
			LUMP_MAP_FLAGS = 59,
			LUMP_OVERLAY_FADES = 60,
			LUMP_OVERLAY_SYSTEM_LEVELS = 61,
			LUMP_PHYSLEVEL = 62,
			LUMP_DISP_MULTIBLEND = 63,
		};

		int32_t offset;
		int32_t length;
		int32_t version;
		int32_t uncompressedSize;
	};

	struct BspHeader {
		int32_t magic; // 'VBSP'
		int32_t version; // 19 or 20
		BspLump lumps[VBSP_HEADER_LUMPS];
		int32_t mapRevision;
	};


	typedef pod::Vector3f BspVertex;
	typedef pod::Vector2s BspEdge;

	struct BspFace {
		uint16_t planenum;
		uint8_t side;
		uint8_t onNode;
		int32_t firstedge;
		int16_t numedges;
		int16_t texinfo;
		int16_t dispinfo;
		int16_t surfaceFogVolumeID;
		pod::Vector4ub styles;
		int32_t lightofs;
		float area;
		pod::Vector2i lightmapTextureMins;
		pod::Vector2i lightmapTextureSize;
		int32_t origFace;
		uint16_t numPrims;
		uint16_t firstPrimID;
		uint16_t smoothingGroups;
	};

	#pragma pack(push, 1)

	struct BspPlane {
		pod::Vector3f normal;
		float dist;
		int32_t type;
	};

	struct BspNode {
		int32_t planenum;
		int32_t children[2];
		int16_t mins[3], maxs[3];
		uint16_t firstface, numfaces;
		int16_t area;
		int16_t padding;
	};

	struct BspLeaf {
		int32_t contents;
		int16_t cluster;
		int16_t area_flags;
		int16_t mins[3], maxs[3];
		uint16_t firstleafface, numleaffaces;
		uint16_t firstleafbrush, numleafbrushes;
		int16_t leafWaterDataID;
	};

	struct BspDispSubNeighbor {
		uint16_t neighbor;
		uint8_t neighborOrientation;
		uint8_t span;
		uint8_t neighborSpan;
		uint8_t padding;
	};

	struct BspDispNeighbor {
		BspDispSubNeighbor subNeighbors[2];
	};

	struct BspDispCornerNeighbors {
		uint16_t neighbors[4];
		uint8_t numNeighbors;
		uint8_t padding;
	};

	struct BspDispInfo {
		pod::Vector3f startPosition;
		int32_t dispVertStart;
		int32_t dispTriStart;
		int32_t power;
		int32_t minTess;
		float smoothingAngle;
		int32_t contents;
		uint16_t mapFace;
		uint16_t padding;
		int32_t lightmapAlphaStart;
		int32_t lightmapSamplePositionStart;

		BspDispNeighbor edgeNeighbors[4];
		BspDispCornerNeighbors cornerNeighbors[4];
		uint32_t allowedVerts[10];
	};

	#pragma pack(pop)

	struct BspDispVert {
		pod::Vector3f vec;
		float dist;
		float alpha;
	};

	struct BspCubemap {
		pod::Vector3i origin;
		int32_t size;
	};

	struct BspTexInfo {
		pod::Vector4f textureVecs[2];
		pod::Vector4f lightmapVecs[2];
		int32_t flags;
		int32_t texData;
	};

	struct BspTexData {
		pod::Vector3f reflectivity;
		int32_t nameStringTableID;
		int32_t width, height;
		int32_t view_width, view_height;
	};

	struct BspModel {
		pod::Vector3f mins, maxs;
		pod::Vector3f origin;
		int32_t headnode;
		int32_t firstface, numfaces;
	};

	struct BspGameLump {
		int32_t id;
		uint16_t flags;
		uint16_t version;
		int32_t fileofs;
		int32_t filelen;
	};

	struct BspContext {
		uf::stl::vector<impl::BspVertex> vertices;
		uf::stl::vector<impl::BspEdge> edges;
		uf::stl::vector<int32_t> surfedges;
		uf::stl::vector<impl::BspFace> faces;
		// brush, brushside
		uf::stl::vector<impl::BspPlane> planes;
		uf::stl::vector<impl::BspNode> nodes;
		uf::stl::vector<impl::BspLeaf> leafs;
		uf::stl::vector<uint16_t> leaffaces;
		// leaffbrush
		uf::stl::vector<impl::BspTexInfo> texinfos;
		uf::stl::vector<impl::BspTexData> texdatas;
		uf::stl::vector<int32_t> stringTable;
		uf::stl::string stringData;
		uf::stl::vector<impl::BspModel> models;
		uf::stl::vector<uint8_t> visibility;
		uf::stl::vector<int8_t> entities;
		uf::stl::vector<impl::BspGameLump> gameLumps;
		uf::stl::vector<impl::BspDispInfo> dispinfos;
		uf::stl::vector<impl::BspDispVert> dispverts;
		// disptris
		uf::stl::vector<uint8_t> pakfile;
		uf::stl::vector<impl::BspCubemap> cubemaps;
		// overlay
		uf::stl::vector<uint8_t> lighting;
		// ambient lighting
		// occlusion
		// physics
		// worldlight
		// other

		int32_t skyArea = -1;
		uf::stl::unordered_set<int32_t> skyboxFaces;
		uf::stl::vector<int32_t> modelToMesh;
		uf::stl::vector<int32_t> texdataToMaterial;
		uf::stl::vector<int32_t> cubemapIDs;
		pod::Atlas lightmapAtlas;
	};

	struct BSP_IO {
		uf::stl::string output;
		uf::stl::string target;
		uf::stl::string input;
		uf::stl::string parameter;
		float delay;
		int timesToFire;
	};

	impl::BSP_IO parseIO( const uf::stl::string& output, const uf::stl::string& value ) {
		char delimiter = value.find('\x1B') != uf::stl::string::npos ? '\x1B' : ','; // either ESC or a comma
		auto tokens = uf::string::split( value, delimiter );

		impl::BSP_IO io;
		io.output = output;
		io.target = tokens.size() > 0 ? tokens[0] : "";
		io.input = tokens.size() > 1 ? tokens[1] : "";
		io.parameter = tokens.size() > 2 ? tokens[2] : "";
		io.delay = tokens.size() > 3 ? std::stof(tokens[3]) : 0.0f;
		io.timesToFire = tokens.size() > 4 ? std::stoi(tokens[4]) : -1;

		return io;
	}

	int32_t findLeaf( const impl::BspContext& context, const pod::Vector3f& pos ) {
		if ( context.nodes.empty() || context.planes.empty() || context.leafs.empty() ) return 0;

		int32_t node = 0;
		while ( node >= 0 ) {
			const auto& n = context.nodes[node];
			const auto& p = context.planes[n.planenum];

			float d = uf::vector::dot(pos, p.normal) - p.dist;

			node = n.children[d < 0 ? 1 : 0];
		}
		return -node - 1;
	}

	bool isClusterVisible( const impl::BspContext& context, int32_t visCluster, int32_t testCluster ) {
		if ( context.visibility.size() < 8 || visCluster < 0 || testCluster < 0 ) return true;

		// PVS Header: [num_clusters] [byteofs_pvs[num_clusters]] [byteofs_phs[num_clusters]]
		int32_t numClusters = *(int32_t*)(context.visibility.data());
		if ( visCluster >= numClusters || testCluster >= numClusters ) return false;

		// offset to this cluster's PVS data
		int32_t byteOffset = *(int32_t*)(context.visibility.data() + 4 + visCluster * 8);
		const uint8_t* pvs = context.visibility.data() + byteOffset;

		// decompress RLE
		int32_t clusterCounter = 0;
		while ( clusterCounter <= testCluster ) {
			if ( *pvs == 0 ) {
				clusterCounter += 8 * pvs[1];
				pvs += 2;
			} else {
				if ( clusterCounter + 8 > testCluster ) {
					return (*pvs & (1 << (testCluster - clusterCounter))) != 0;
				}
				clusterCounter += 8;
				pvs++;
			}
		}
		return false;
	}

	int32_t findClosestCubemap( const impl::BspContext& context, const pod::Vector3f& position ) {
		if ( context.cubemapIDs.empty() ) return -1;

		int32_t bestID = context.cubemapIDs[0];
		float minDistance = std::numeric_limits<float>::max();

		for ( size_t i = 0; i < context.cubemaps.size(); ++i ) {
			pod::Vector3f cubePos = impl::convertPos(pod::Vector3f{
				context.cubemaps[i].origin.x,
				context.cubemaps[i].origin.y,
				context.cubemaps[i].origin.z
			});

			float distSq = uf::vector::distanceSquared( position, cubePos );
			if ( distSq < minDistance ) {
				minDistance = distSq;
				bestID = context.cubemapIDs[i];
			}
		}
		return bestID;
	};

	pod::Atlas::hash_t faceHash( size_t i ) {
		return ::fmt::format("face_{}", i);
	}

	void buildDisplacement( const impl::BspContext& context, impl::Meshlet& meshlet, size_t faceID ) {
		const auto& face = context.faces[faceID];
		const auto& info = context.dispinfos[face.dispinfo];
		const auto& texInfo = context.texinfos[face.texinfo];
		int side = (1 << info.power) + 1; // 2^power + 1

		struct CornerData {
			pod::Vector3f pos;
			pod::Vector2f uv;
			pod::Vector2f st;
		};
		uf::stl::vector<CornerData> corners(4);

		float texWidth = 512.0f, texHeight = 512.0f;
		if ( texInfo.texData >= 0 && texInfo.texData < context.texdatas.size() ) {
			texWidth = (float)(context.texdatas[texInfo.texData].width);
			texHeight = (float)(context.texdatas[texInfo.texData].height);
		}

		for ( int i = 0; i < 4; ++i ) {
			int32_t se = context.surfedges[face.firstedge + i];
			uint16_t vIdx = se >= 0 ? context.edges[se].x : context.edges[-se].y;
			pod::Vector3f rawPos = context.vertices[vIdx];
			corners[i].pos = rawPos;

			pod::Vector4f v = rawPos; v.w = 1.0f;

			corners[i].uv.x = uf::vector::dot( v, texInfo.textureVecs[0] ) / texWidth;
			corners[i].uv.y = uf::vector::dot( v, texInfo.textureVecs[1] ) / texHeight;

			corners[i].st.x = (uf::vector::dot( v, texInfo.lightmapVecs[0] ) + 0.5f - face.lightmapTextureMins.x) / (face.lightmapTextureSize.x + 1.0f);
			corners[i].st.y = (uf::vector::dot( v, texInfo.lightmapVecs[1] ) + 0.5f - face.lightmapTextureMins.y) / (face.lightmapTextureSize.y + 1.0f);
		}

		int startIndex = 0;
		float minDist = 999999.0f;
		for ( int i = 0; i < 4; ++i ) {
			float dist = uf::vector::distance( corners[i].pos, info.startPosition );
			if ( dist < minDist ) {
				minDist = dist;
				startIndex = i;
			}
		}
		std::rotate(corners.begin(), corners.begin() + startIndex, corners.end());

		uint32_t startVertexID = meshlet.vertices.size();
		for ( int y = 0; y < side; ++y ) {
			float ty = (float)y / (side - 1);
			pod::Vector3f posE0 = uf::vector::lerp( corners[0].pos, corners[1].pos, ty );
			pod::Vector3f posE1 = uf::vector::lerp( corners[3].pos, corners[2].pos, ty );
			pod::Vector2f uvE0  = uf::vector::lerp( corners[0].uv,  corners[1].uv,  ty );
			pod::Vector2f uvE1  = uf::vector::lerp( corners[3].uv,  corners[2].uv,  ty );
			pod::Vector2f stE0  = uf::vector::lerp( corners[0].st,  corners[1].st,  ty );
			pod::Vector2f stE1  = uf::vector::lerp( corners[3].st,  corners[2].st,  ty );
		/*
			pod::Vector3f posE0 = uf::vector::lerp( corners[0].pos, corners[1].pos, ty );
			pod::Vector3f posE1 = uf::vector::lerp( corners[3].pos, corners[2].pos, ty );
			pod::Vector2f uvE0  = uf::vector::lerp( corners[0].uv,  corners[1].uv,  ty );
			pod::Vector2f uvE1  = uf::vector::lerp( corners[3].uv,  corners[2].uv,  ty );
			pod::Vector2f stE0  = uf::vector::lerp( corners[0].st,  corners[1].st,  ty );
			pod::Vector2f stE1  = uf::vector::lerp( corners[3].st,  corners[2].st,  ty );
		*/

			for ( int x = 0; x < side; ++x ) {
				float tx = (float)x / (side - 1);

				pod::Vector3f basePos = uf::vector::lerp( posE0, posE1, tx );

				pod::Vector4f vBase = basePos;
				vBase.w = 1.0f;

				pod::Vector2f finalUv;
				finalUv.x = uf::vector::dot( vBase, texInfo.textureVecs[0] ) / texWidth;
				finalUv.y = uf::vector::dot( vBase, texInfo.textureVecs[1] ) / texHeight;

				pod::Vector2f finalSt;
				finalSt.x = tx;
				finalSt.y = ty;

				int dispIdx = info.dispVertStart + y * side + x;
				const auto& dVert = context.dispverts[dispIdx];

				auto& vert = meshlet.vertices.emplace_back();
				vert.position = impl::convertPos( basePos + (dVert.vec * dVert.dist) );
				vert.uv = finalUv;

				vert.st = uf::atlas::mapUv( context.lightmapAtlas, finalSt, impl::faceHash( faceID ) );
				vert.color = { 1.0f, 1.0f, 1.0f, dVert.alpha / 255.0f };
			}
		}

		for ( int y = 0; y < side; ++y ) {
			for ( int x = 0; x < side; ++x ) {
				int id = startVertexID + y * side + x;

				pod::Vector3f pL = meshlet.vertices[startVertexID + y * side + std::max(x - 1, 0)].position;
				pod::Vector3f pR = meshlet.vertices[startVertexID + y * side + std::min(x + 1, side - 1)].position;
				pod::Vector3f pD = meshlet.vertices[startVertexID + std::max(y - 1, 0) * side + x].position;
				pod::Vector3f pU = meshlet.vertices[startVertexID + std::min(y + 1, side - 1) * side + x].position;

				pod::Vector3f tangent = uf::vector::normalize(pR - pL);
				pod::Vector3f bitangent = uf::vector::normalize(pU - pD);

				pod::Vector3f normal = uf::vector::normalize(uf::vector::cross(bitangent, tangent));

				pod::Vector3f t = uf::vector::normalize( impl::convertPos( texInfo.textureVecs[0], 1.0f ) );
				pod::Vector3f b = uf::vector::normalize( impl::convertPos( texInfo.textureVecs[1], 1.0f ) );

				t = uf::vector::normalize(t - normal * uf::vector::dot(normal, t));
				float w = (uf::vector::dot( uf::vector::cross(normal, t), b ) < 0.0f) ? -1.0f : 1.0f;

				meshlet.vertices[id].normal = normal;
				meshlet.vertices[id].tangent = { t.x, t.y, t.z, w };
			}
		}

		auto isVertAllowed = [&](int x, int y) -> bool {
			int index = y * side + x;
			int arrayIndex = index / 32;
			int bitIndex = index % 32;
			return ( info.allowedVerts[arrayIndex] & (1 << bitIndex) ) != 0;
		};

		for ( int y = 0; y < side - 1; ++y ) {
			for ( int x = 0; x < side - 1; ++x ) {
				if ( !isVertAllowed(x, y) || !isVertAllowed(x + 1, y) || !isVertAllowed(x, y + 1) || !isVertAllowed(x + 1, y + 1) ) {
					continue;
				}

				uint32_t v0 = startVertexID + y * side + x;
				uint32_t v1 = startVertexID + y * side + (x + 1);
				uint32_t v2 = startVertexID + (y + 1) * side + x;
				uint32_t v3 = startVertexID + (y + 1) * side + (x + 1);

				meshlet.indices.emplace_back(v0); meshlet.indices.emplace_back(v2); meshlet.indices.emplace_back(v1);
				meshlet.indices.emplace_back(v1); meshlet.indices.emplace_back(v2); meshlet.indices.emplace_back(v3);
			}
		}
	}

	void addVertex( const impl::BspContext& context, impl::Meshlet& meshlet, uint16_t vertexID, pod::Vector3f pos, pod::Vector3f normal, size_t faceID ) {
		auto& bounds = meshlet.primitive.instance.bounds;
		if ( meshlet.vertices.empty() ) {
			bounds.min = bounds.max = pos;
		} else {
			bounds.min = uf::vector::min( bounds.min, pos );
			bounds.max = uf::vector::max( bounds.max, pos );
		}

		const auto& face = context.faces[faceID];
		pod::Vector4f vertex = context.vertices[vertexID];
		vertex.w = 1; // for dot products

		// add index
		meshlet.indices.emplace_back((uint32_t)(meshlet.vertices.size()));
		// add vertex
		auto& v = meshlet.vertices.emplace_back();
		v.position = pos;
		v.color = { 1.0f, 1.0f, 1.0f, 1.0f };
		v.normal = normal;
		v.tangent = { 1.0f, 0.0f, 0.0f, 1.0f };

		// has texture information
		if ( face.texinfo >= 0 && face.texinfo < context.texinfos.size() ) {
			const auto& info = context.texinfos[face.texinfo];
			if ( info.texData >= 0 && info.texData < context.texdatas.size() ) {
				const auto& data = context.texdatas[info.texData];

				v.uv.x = uf::vector::dot( vertex, info.textureVecs[0] ) / (float) data.width;
				v.uv.y = uf::vector::dot( vertex, info.textureVecs[1] ) / (float) data.height;
			}

			v.st.x = (uf::vector::dot( vertex, info.lightmapVecs[0] ) + 0.5f - face.lightmapTextureMins.x) / (face.lightmapTextureSize.x + 1.0f);
			v.st.y = (uf::vector::dot( vertex, info.lightmapVecs[1] ) + 0.5f - face.lightmapTextureMins.y) / (face.lightmapTextureSize.y + 1.0f);

			v.st = uf::atlas::mapUv( context.lightmapAtlas, v.st, impl::faceHash( faceID ) );

			pod::Vector3f t = uf::vector::normalize( impl::convertPos( info.textureVecs[0], 1.0f ) );
			pod::Vector3f b = uf::vector::normalize( impl::convertPos( info.textureVecs[1], 1.0f ) );

			t = uf::vector::normalize(t - normal * uf::vector::dot(normal, t));
			float w = (uf::vector::dot( uf::vector::cross(normal, t), b ) < 0.0f) ? -1.0f : 1.0f;

			v.tangent = { t.x, t.y, t.z, w };
		}
	};

	template<typename T>
	void extractLump( const uf::stl::vector<uint8_t>& buffer, const impl::BspLump& lump, uf::stl::vector<T>& data ) {
		if ( lump.length == 0 || lump.offset >= buffer.size() ) return;

		size_t count = lump.length / sizeof(T);
		data.resize(count);
		std::copy(buffer.data() + lump.offset, buffer.data() + lump.offset + lump.length, (uint8_t*)(data.data()) );
	}
	template<typename T>
	uf::stl::vector<T> extractLump( const uf::stl::vector<uint8_t>& buffer, const impl::BspLump& lump ) {
		uf::stl::vector<T> data;
		impl::extractLump<T>( buffer, lump, data );
		return data;
	}

	template<>
	void extractLump( const uf::stl::vector<uint8_t>& buffer, const impl::BspLump& lump, uf::stl::vector<impl::BspLeaf>& data ) {
		if ( lump.length == 0 || lump.offset >= buffer.size() ) return;

		int stride = (lump.version == 0) ? 56 : 32;

		size_t count = lump.length / stride;
		data.resize(count);

		const uint8_t* src = buffer.data() + lump.offset;
		for ( size_t i = 0; i < count; ++i ) {
			std::memcpy(&data[i], src + (i * stride), sizeof(impl::BspLeaf));
		}
	}

	template<>
	void extractLump( const uf::stl::vector<uint8_t>& buffer, const impl::BspLump& lump, uf::stl::vector<impl::BspGameLump>& data ) {
		if ( lump.length == 0 || lump.offset >= buffer.size() ) return;

		const uint8_t* glData = buffer.data() + lump.offset;
		int32_t lumpCount = *(const int32_t*)glData;

		data.resize(lumpCount);
		std::copy(glData + 4, glData + 4 + (lumpCount * sizeof(impl::BspGameLump)), (uint8_t*)(data.data()));
	}

	template<>
	uf::stl::vector<impl::BspGameLump> extractLump( const uf::stl::vector<uint8_t>& buffer, const impl::BspLump& lump ) {
		uf::stl::vector<impl::BspGameLump> data;
		impl::extractLump( buffer, lump, data );
		return data;
	}

	void extractLumpString( const uf::stl::vector<uint8_t>& buffer, const impl::BspLump& lump, uf::stl::string& str ) {
		auto data = impl::extractLump<char>( buffer, lump );
		str = uf::stl::string( data.data(), data.size() );
	}
	uf::stl::string extractLumpString( const uf::stl::vector<uint8_t>& buffer, const impl::BspLump& lump ) {
		uf::stl::string str;
		impl::extractLumpString( buffer, lump, str );
		return str;
	}

	void processNodes( pod::Graph& graph, const impl::BspContext& context, float scale = impl::sourceToMeters ) {
		size_t lights = 0;

		int32_t spawnID = -1;
		uf::stl::vector<size_t> spawns;
		uf::stl::unordered_map<uf::stl::string, size_t> targets;

		for ( auto nodeID : graph.root.children ) {
			auto& node = graph.nodes[nodeID];
			auto& metadata = node.metadata["valve"];

			if ( !ext::json::isObject( metadata ) ) continue;

			auto classname = metadata["classname"].as<uf::stl::string>("");
			//UF_MSG_INFO("Entity found: {}", classname);
			node.name = classname;
			node.mesh = -1;
			
			// parse origin
			auto origin = metadata["origin"].as<uf::stl::string>("");
			if ( origin != "" ) {
				auto position = impl::str2vec<pod::Vector3f>( origin );
				node.transform.position = impl::convertPos( position, scale );
			}

			// parse angles
			auto angles = metadata["angles"].as<uf::stl::string>("");
			if ( angles != "" ) {
				auto pyr = impl::str2vec<pod::Vector3f>( angles ) * -DEG_2_RAD;
				node.transform.orientation = uf::quaternion::euler( pyr );
			}

			// parse model
			auto model = metadata["model"].as<uf::stl::string>();
			if ( classname == "worldspawn" ) {
				node.mesh = context.modelToMesh[0]; // implicitly bind to model 0
			} else if ( model.starts_with("*") ) {
				int modelID = std::stoi( model.substr(1) );
				if ( 0 <= modelID && modelID < context.modelToMesh.size() ) {
					node.mesh = context.modelToMesh[modelID];
				}
			} else if ( model.length() > 4 && model.ends_with(".mdl") ) {
				auto it = std::find(graph.meshes.begin(), graph.meshes.end(), model);
				if ( it == graph.meshes.end() ) {
					auto meshID = graph.meshes.size();
					if ( ext::valve::loadMdl(graph, model) ) {
						node.mesh = meshID;
					} else {
						uf::stl::string model = "models/error.mdl";
						auto it = std::find(graph.meshes.begin(), graph.meshes.end(), model);

						if ( it != graph.meshes.end() ) {
							node.mesh = (int32_t)std::distance(graph.meshes.begin(), it);
						} else if ( ext::valve::loadMdl( graph, model ) ) {
							node.mesh = (int32_t)(graph.meshes.size() - 1);
						} else {
							node.mesh = -1;
						}
					}
				} else {
					node.mesh = (int32_t)std::distance(graph.meshes.begin(), it);
				}
			}

			// parse lighting info
			if ( classname.starts_with("light") ) {
				auto lightKeyName = ::fmt::format( "{}_{}", classname, nodeID );
				auto& light = graph.lights[lightKeyName];
				light.color = { 1.0f, 1.0f, 1.0f };
				light.intensity = 200.0f;
				light.range = 0.0f;

				// read color and intensity
				auto _light = metadata["_light"].as<uf::stl::string>("");
				if ( _light != "" ) {
					// to-do: do not use stringstream
					std::istringstream stream(_light);
					light.color = { 255.0f, 255.0f, 255.0f };
					
					stream >> light.color.x >> light.color.y >> light.color.z;
					light.color /= 255.0f;

					if (!(stream >> light.intensity)) light.intensity = 200.0f;
				}
				// to-do: read range
				light.intensity *= 0.2f; // scale down
			}

			// parse door
			if ( classname.starts_with("func_door") ) {
				auto& metadataDoor = metadata["door"];

				float scale = impl::sourceToMeters;
				if ( classname == "func_door" ) {
					auto movedirStr = metadata["movedir"].as<uf::stl::string>("");
					if ( movedirStr == "" && metadata["angle"].as<float>() ) {
						float ang = metadata["angle"].as<float>();
						if ( ang == -1 ) movedirStr = "-90 0 0";
						else if ( ang == -2 ) movedirStr = "90 0 0";
						else movedirStr = ::fmt::format("0 {} 0", ang);
					}

					if ( movedirStr != "" ) {
						auto pyr = impl::str2vec<pod::Vector3f>( movedirStr );

						float pitch = pyr.x * DEG_2_RAD;
						float yaw   = pyr.y * DEG_2_RAD;

						pod::Vector3f slideDir;
						slideDir.x = cos(yaw) * cos(pitch);
						slideDir.z = -(sin(yaw) * cos(pitch));
						slideDir.y = -sin(pitch);

						metadataDoor["direction"] = uf::vector::encode( uf::vector::normalize(slideDir) );
					}
				} else if ( classname == "func_door_rotating" ) {
					scale = 1.0f;
					metadataDoor["distance"] = metadata["distance"].as<float>(90.0f);

					int flags = metadataDoor["spawnflags"].as<int>();
					pod::Vector3f axis = {0, 1, 0};
					if ( flags & 64 ) axis = {0, 0, 1};
					if ( flags & 128 ) axis = {1, 0, 0};

					metadataDoor["axis"] = uf::vector::encode( axis );
				}
				
				metadataDoor["speed"] = metadata["speed"].as<float>(100.0f) * scale;
				metadataDoor["wait"] = metadata["wait"].as<float>(4.0f);
				metadataDoor["lip"] = metadata["lip"].as<float>(8.0f) * scale;
				metadataDoor["spawnflags"] = metadata["spawnflags"].as<int>(0);
			}

			// parse parent
			auto targetname = metadata["targetname"].as<uf::stl::string>("");
			if ( targetname != "" ) {
				targets[targetname] = nodeID;
			}

			// to-do: add additional parsing
		}

		// re-parent entities
		uf::stl::vector<int32_t> newChildren;
		for ( auto nodeID : graph.root.children ) {
			auto& node = graph.nodes[nodeID];
			auto& metadata = node.metadata["valve"];

			auto parentname = metadata["parentname"].as<uf::stl::string>("");
			if ( parentname != "" && targets.count(parentname) > 0 ) {
				auto parentID = targets[parentname];
				auto& parentNode = graph.nodes[parentID];
				parentNode.children.emplace_back(nodeID);

				node.transform = uf::transform::relative( parentNode.transform, node.transform );
			} else {
				newChildren.emplace_back(nodeID);
			}
		}
		graph.root.children = newChildren;
	}
}

void ext::valve::loadBsp( pod::Graph& graph, const uf::stl::string& filename, const uf::Serializer& metadata ) {
	uf::stl::vector<uint8_t> buffer;
	if ( !uf::io::readAsBuffer(buffer, filename) ) {
		UF_MSG_ERROR("Failed to read BSP data: {}", filename);
		return;
	}

	const impl::BspHeader* header = (const impl::BspHeader*)(buffer.data());
	if ( header->magic != 0x50534256 ) {
		UF_MSG_ERROR("Invalid VBSP magic number: {:X}", header->magic);
		return;
	}

	if ( !graph.storage ) graph.storage = new pod::Graph::Storage();
	auto& storage = uf::graph::getStorage( graph );
	graph.name = filename;
	graph.metadata = metadata;
	graph.root.name = "%ROOT%";
	graph.root.index = -1;

	impl::BspContext context;
	impl::extractLump<impl::BspVertex>(buffer, header->lumps[impl::BspLump::LUMP_VERTICES], context.vertices);
	impl::extractLump<impl::BspEdge>(buffer, header->lumps[impl::BspLump::LUMP_EDGES], context.edges);
	impl::extractLump<int32_t>(buffer, header->lumps[impl::BspLump::LUMP_SURFEDGES], context.surfedges);
	impl::extractLump<impl::BspFace>(buffer, header->lumps[impl::BspLump::LUMP_FACES], context.faces);
	impl::extractLump<impl::BspPlane>(buffer, header->lumps[impl::BspLump::LUMP_PLANES], context.planes);
	impl::extractLump<impl::BspNode>(buffer, header->lumps[impl::BspLump::LUMP_NODES], context.nodes);
	impl::extractLump<impl::BspLeaf>(buffer, header->lumps[impl::BspLump::LUMP_LEAFS], context.leafs);
	impl::extractLump<uint16_t>(buffer, header->lumps[impl::BspLump::LUMP_LEAFFACES], context.leaffaces);
	impl::extractLump<impl::BspTexInfo>(buffer, header->lumps[impl::BspLump::LUMP_TEXINFO], context.texinfos);
	impl::extractLump<impl::BspTexData>(buffer, header->lumps[impl::BspLump::LUMP_TEXDATA], context.texdatas);
	impl::extractLump<int32_t>(buffer, header->lumps[impl::BspLump::LUMP_TEXDATA_STRING_TABLE], context.stringTable);
	impl::extractLumpString(buffer, header->lumps[impl::BspLump::LUMP_TEXDATA_STRING_DATA], context.stringData);
	impl::extractLump<impl::BspModel>(buffer, header->lumps[impl::BspLump::LUMP_MODELS], context.models);
	impl::extractLump<uint8_t>(buffer, header->lumps[impl::BspLump::LUMP_VISIBILITY], context.visibility);
	impl::extractLump<int8_t>(buffer, header->lumps[impl::BspLump::LUMP_ENTITIES], context.entities);
	impl::extractLump<impl::BspGameLump>(buffer, header->lumps[impl::BspLump::LUMP_GAME_LUMP], context.gameLumps);
	impl::extractLump<impl::BspDispInfo>(buffer, header->lumps[impl::BspLump::LUMP_DISPINFO], context.dispinfos);
	impl::extractLump<impl::BspDispVert>(buffer, header->lumps[impl::BspLump::LUMP_DISP_VERTS], context.dispverts);
	impl::extractLump<uint8_t>(buffer, header->lumps[impl::BspLump::LUMP_PAKFILE], context.pakfile);
	impl::extractLump<impl::BspCubemap>(buffer, header->lumps[impl::BspLump::LUMP_CUBEMAPS], context.cubemaps);
	impl::extractLump<uint8_t>(buffer, header->lumps[impl::BspLump::LUMP_LIGHTING], context.lighting);

	context.modelToMesh.assign( context.models.size(), -1 );
	context.texdataToMaterial.assign( context.texdatas.size(), -1 );

	// mount pakfile
	size_t pakfileMount = uf::vfs::mount( ext::zlib::createZipMount(::fmt::format("pakfile://{}", filename), context.pakfile, 1000 ) );	

	// deduce mapname
	uf::stl::string mapName = filename; {
		auto slashPos = mapName.find_last_of("/\\");
		if (slashPos != uf::stl::string::npos) mapName = mapName.substr(slashPos + 1);
		auto dotPos = mapName.find_last_of('.');
		if (dotPos != uf::stl::string::npos) mapName = mapName.substr(0, dotPos);
	}

	// load baked cubemaps
	for ( const auto& cube : context.cubemaps ) {
		auto matName = ::fmt::format("maps/{}/c{}_{}_{}", mapName, cube.origin.x, cube.origin.y, cube.origin.z);
		auto vtfPath = ::fmt::format("materials/{}.vtf", matName);
		graph.metadata["cubemaps"].emplace_back( matName );

		int32_t textureID = -1;
		impl::addMaterial( graph, matName, textureID );

		auto& image = storage.images[matName].data;
		auto& texture = storage.images[matName].handle;
		if ( ext::valve::loadVtf( image, vtfPath ) ) {
			context.cubemapIDs.emplace_back( storage.materials[matName].indexCubemap = textureID );
		}
	}

	// read materials
	for ( int32_t texDataID = 0; texDataID < context.texdatas.size(); ++texDataID ) {
		const auto& data = context.texdatas[texDataID];
		uf::stl::string matName = "missing_texture";

		// lookup material name
		if ( data.nameStringTableID >= 0 && data.nameStringTableID < context.stringTable.size() ) {
			int32_t offset = context.stringTable[data.nameStringTableID];
			if ( offset >= 0 && offset < context.stringData.size() ) {
				matName = uf::string::lowercase( context.stringData.c_str() + offset );
			}
		}

		int32_t textureID = -1;
		context.texdataToMaterial[texDataID] = impl::addMaterial( graph, matName, textureID );
		storage.materials[matName].indexAlbedo = textureID;
	}

	// read lightmaps
	auto atlasImageID = graph.images.size();
	auto atlasTextureID = graph.textures.size();

	for ( size_t i = 3; i < context.lighting.size(); i += 4 ) {
		int8_t exp = (int8_t)context.lighting[i];
		context.lighting[i] = (uint8_t)(exp + 128);
	}
	for ( auto faceID = 0; faceID < context.faces.size(); ++faceID ) {
		const auto& face = context.faces[faceID];
		if ( face.lightofs == -1 || context.lighting.empty() ) continue;
		
		size_t width  = face.lightmapTextureSize.x + 1;
		size_t height = face.lightmapTextureSize.y + 1;
	

		uf::Image image;
		image.loadFromBuffer( (uint8_t*)(context.lighting.data() + face.lightofs), { width, height }, 8, 4 );

		uf::atlas::add( context.lightmapAtlas, image, impl::faceHash( faceID ) );
	}
	
	{	
		UF_MSG_DEBUG("Generating new lightmap atlas...");
		uf::atlas::generate( context.lightmapAtlas, 1 );
		//UF_MSG_DEBUG("Generated lightmap atlas.");

		auto& imageKey = graph.images.emplace_back("lightmap_atlas");
		auto& textureKey = graph.textures.emplace_back("lightmap_atlas");
		
		storage.images[imageKey].data = uf::atlas::get( context.lightmapAtlas );
		storage.textures[textureKey].index = atlasImageID;
	}

	// read entities
	{
		bool parsing = false;
		uf::stl::unordered_map<uf::stl::string, uf::stl::string> dict;
		uf::stl::vector<impl::BSP_IO> connections;
		
		uf::stl::string string( (const char*) context.entities.data() );
		uf::stl::string line;
		std::istringstream stream(string);
		while ( std::getline(stream, line) ) {
			if ( line.find("{") != uf::stl::string::npos ) {
				parsing = true;
				dict.clear();
				connections.clear();
			} else if ( line.find("}") != uf::stl::string::npos ) {
				parsing = false;
				
				// create node
				auto nodeID = graph.nodes.size();
				auto& node = graph.nodes.emplace_back();
				auto& metadata = node.metadata["valve"];
				for ( const auto& [k, v] : dict ) metadata[k] = impl::processValue( v ); // store as metadata for later parsing

				// insert IO
				if ( !connections.empty() ) {
					if ( ext::json::isNull( metadata["connections"] ) ) {
						ext::json::reserve( metadata["connections"], connections.size() );
					}
					for ( auto& io : connections ) {
						auto& entry = metadata["connections"].emplace_back();
						entry["output"] = io.output;
						entry["target"] = io.target;
						entry["input"] = io.input;
						entry["parameter"] = io.parameter;
						entry["delay"] = io.delay;
						entry["times"] = io.timesToFire;
					}
				}

				// add node as child
				graph.root.children.emplace_back( nodeID );
			} else if ( parsing ) {
				uf::stl::string key, value;
				if ( impl::parseKeyValue(line, key, value) ) {
					if ( key.starts_with("On") ) {
						connections.emplace_back( impl::parseIO( key, value ) );
						continue;
					}
					dict[key] = value;
				}
			}
		}
	}

	// prepare for segregating skybox from worldspawn
	{
		pod::Vector3f skyCameraOrigin = {NAN, NAN, NAN};
		for ( auto& node : graph.nodes ) {
			auto& metadataValve = node.metadata["valve"];
			if ( metadataValve["classname"].as<uf::stl::string>("") != "sky_camera" ) continue;
			auto originStr = metadataValve["origin"].as<uf::stl::string>("");
			if ( originStr == "" ) continue;
			skyCameraOrigin = impl::str2vec<pod::Vector3f>( originStr );
			break;
		}

		if ( uf::vector::isValid( skyCameraOrigin ) ) {
			int32_t skyLeafIdx = impl::findLeaf(context, skyCameraOrigin);

			context.skyArea = context.leafs[skyLeafIdx].area_flags & 0x01FF;

			for ( const auto& leaf : context.leafs ) {
				int16_t leafArea = leaf.area_flags & 0x01FF;

				if ( leafArea == context.skyArea ) {
					for ( int i = 0; i < leaf.numleaffaces; ++i ) {
						context.skyboxFaces.insert(context.leaffaces[leaf.firstleafface + i]);
					}
				}
			}
		}
	}

	// read models
	for ( auto m = 0; m < context.models.size(); ++m ) {
		const auto& model = context.models[m];
		uf::stl::unordered_map<int32_t, impl::Meshlet> meshlets; // group by material IDs
		uf::stl::unordered_map<int32_t, impl::Meshlet> skyboxMeshlets; // segregate skybox geometry

		for ( auto i = 0; i < model.numfaces; ++i ) {
			const auto faceID = model.firstface + i;
			const auto& face = context.faces[faceID];

			if ( face.numedges < 3 ) continue;
			if ( face.texinfo < 0 || face.texinfo >= context.texinfos.size() ) continue;

			int32_t texDataID = context.texinfos[face.texinfo].texData;
			if ( texDataID < 0 || texDataID >= context.texdatas.size() ) continue;

			size_t materialID = context.texdataToMaterial[texDataID];
			auto& matName = graph.materials[materialID];

			// deduce which group to use
			bool isSkybox = (m == 0 && context.skyboxFaces.count(faceID) > 0);
			if ( !isSkybox && m == 0 && context.skyArea != -1 ) {
				int32_t se0 = context.surfedges[face.firstedge];
				int32_t se1 = context.surfedges[face.firstedge + 1];
				int32_t se2 = context.surfedges[face.firstedge + 2];

				uint16_t v0 = se0 >= 0 ? context.edges[se0].x : context.edges[-se0].y;
				uint16_t v1 = se1 >= 0 ? context.edges[se1].x : context.edges[-se1].y;
				uint16_t v2 = se2 >= 0 ? context.edges[se2].x : context.edges[-se2].y;

				pod::Vector3f p0 = context.vertices[v0];
				pod::Vector3f p1 = context.vertices[v1];
				pod::Vector3f p2 = context.vertices[v2];

				pod::Vector3f normal = uf::vector::normalize(uf::vector::cross(p1 - p0, p2 - p0));

				pod::Vector3f testPos = p0 + normal * 2.0f;

				int32_t testLeafIdx = impl::findLeaf(context, testPos);
				if ( testLeafIdx >= 0 && testLeafIdx < context.leafs.size() ) {
					int16_t testArea = context.leafs[testLeafIdx].area_flags & 0x01FF;
					if ( testArea == context.skyArea ) {
						isSkybox = true;
					}
				}
			}

			// read brush
			auto& meshlet = (isSkybox ? skyboxMeshlets : meshlets)[materialID];
			meshlet.primitive.instance.materialID = materialID;
			
			if ( 0 <= face.lightofs ) {
				meshlet.primitive.instance.lightmapID = atlasTextureID;
			}

			if ( !context.cubemapIDs.empty() ) {
				int32_t pivotSurfEdge = context.surfedges[face.firstedge];
				uint16_t pivotVertID = pivotSurfEdge >= 0 ? context.edges[pivotSurfEdge].x : context.edges[-pivotSurfEdge].y;
				pod::Vector3f p0 = impl::convertPos( context.vertices[pivotVertID] );

				meshlet.primitive.instance.cubemapID = impl::findClosestCubemap( context, p0 );
			}
			
			if ( face.dispinfo != -1 ) {
				impl::buildDisplacement( context, meshlet, faceID );
				continue;
			}

			
			const auto edgeID = face.firstedge;
			int32_t pivotSurfEdge = context.surfedges[edgeID];
			uint16_t pivotVertID = pivotSurfEdge >= 0 ? context.edges[pivotSurfEdge].x : context.edges[-pivotSurfEdge].y;
			pod::Vector3f p0 = impl::convertPos( context.vertices[pivotVertID] );

		/*
			// some faces are wrong when doing it this way
			const auto& plane = context.planes[face.planenum];
			pod::Vector3f faceNormal = uf::vector::normalize( impl::convertPos( plane.normal, 1.0f ) );
			if ( face.side != 0 ) faceNormal = -faceNormal;
		*/

			pod::Vector3f faceNormal = {0.0f, 0.0f, 0.0f};
			for ( int16_t i = 1; i < face.numedges - 1; ++i ) {
				int32_t se1 = context.surfedges[edgeID + i];
				int32_t se2 = context.surfedges[edgeID + i + 1];

				uint16_t v1 = se1 >= 0 ? context.edges[se1].x : context.edges[-se1].y;
				uint16_t v2 = se2 >= 0 ? context.edges[se2].x : context .edges[-se2].y;

				pod::Vector3f p1 = impl::convertPos( context.vertices[v1] );
				pod::Vector3f p2 = impl::convertPos( context.vertices[v2] );

				faceNormal += uf::vector::cross(p1 - p0, p2 - p0);
			}
			faceNormal = uf::vector::normalize(faceNormal);

			for ( int16_t i = 1; i < face.numedges - 1; ++i ) {
				int32_t se1 = context.surfedges[edgeID + i];
				int32_t se2 = context.surfedges[edgeID + i + 1];

				uint16_t v1 = se1 >= 0 ? context.edges[se1].x : context.edges[-se1].y;
				uint16_t v2 = se2 >= 0 ? context.edges[se2].x : context.edges[-se2].y;

				pod::Vector3f p1 = impl::convertPos( context.vertices[v1] );
				pod::Vector3f p2 = impl::convertPos( context.vertices[v2] );

				impl::addVertex( context, meshlet, pivotVertID, p0, faceNormal, faceID );
				impl::addVertex( context, meshlet, v1, p1, faceNormal, faceID );
				impl::addVertex( context, meshlet, v2, p2, faceNormal, faceID );
			}
		}

		if ( !meshlets.empty() ) {
			auto meshName = ::fmt::format("model_{}", m);
			context.modelToMesh[m] = graph.meshes.size();

			graph.meshes.emplace_back(meshName);
			graph.primitives.emplace_back(meshName);

			auto& mesh = storage.meshes[meshName];
			auto& primitives = storage.primitives[meshName];

			// recompute bounds from min/max to center-extent
			for ( auto& [ _, meshlet ] : meshlets ) {
				auto& bounds = meshlet.primitive.instance.bounds;
				bounds.center = ( bounds.min + bounds.max ) * 0.5f;
				bounds.extent = ( bounds.min - bounds.max ) * 0.5f;
			}

			// slice worldspawn
			if ( false && m == 0 ) {
				uf::meshgrid::Grid grid;
				grid.divisions = {8, 8, 8};
				auto mlets = uf::stl::values( meshlets );
				auto partitioned = uf::meshgrid::partition( grid, mlets, EPS, true, true );
				mesh.compile( partitioned, primitives );
			} else {
				mesh.compile( meshlets, primitives );
			}
		}

		if ( !skyboxMeshlets.empty() ) {
			auto meshName = ::fmt::format("model_{}_skybox", m);
			graph.meshes.emplace_back(meshName);
			graph.primitives.emplace_back(meshName);

			auto& mesh = storage.meshes[meshName];
			auto& primitives = storage.primitives[meshName];
			mesh.compile( skyboxMeshlets, primitives );

			// create a skybox node for this
			auto nodeID = graph.nodes.size();
			auto& node = graph.nodes.emplace_back();
			node.name = "skybox";
			node.mesh = (int32_t)(graph.meshes.size() - 1);
			graph.root.children.emplace_back(nodeID);
		}
	}

	// read static props
	for ( const auto& item : context.gameLumps ) {
		if ( item.id == 1936749168 ) { // 'sprp' (Static Props)
			const uint8_t* sprpData = buffer.data() + item.fileofs;
			int offset = 0;

			int32_t entries = *(const int32_t*)(sprpData + offset); offset += 4;
			uf::stl::vector<uf::stl::string> dict(entries);
			for ( int32_t d = 0; d < entries; ++d ) {
				dict[d] = uf::stl::string((const char*)(sprpData + offset), 128).c_str();
				offset += 128;
			}

			int32_t leafEntries = *(const int32_t*)(sprpData + offset); offset += 4;
			offset += leafEntries * sizeof(uint16_t);

			int32_t propEntries = *(const int32_t*)(sprpData + offset); offset += 4;

			int propStride = 0;
			switch ( item.version ) {
				case 4: propStride = 56; break;
				case 5: propStride = 60; break;
				case 6: propStride = 64; break;
				case 7: propStride = 68; break;
				case 8: propStride = 68; break;
				case 9: propStride = 72; break;
				case 10: propStride = 76; break;
				case 11: propStride = 80; break;
				default:
					UF_MSG_WARNING("Unknown static prop version: {}", item.version);
					propStride = (item.filelen - offset) / propEntries;
					break;
			}

			for ( int32_t p = 0; p < propEntries; ++p ) {
				const uint8_t* propData = sprpData + offset + (p * propStride);

				pod::Vector3f origin = *(const pod::Vector3f*)(propData + 0);
				pod::Vector3f angles = *(const pod::Vector3f*)(propData + 12);
				uint16_t type = *(const uint16_t*)(propData + 24);

				if ( type < dict.size() ) {
					auto nodeID = graph.nodes.size();
					auto& node = graph.nodes.emplace_back();
					auto& metadata = node.metadata["valve"];

					metadata["classname"] = "prop_static";
					metadata["model"] = dict[type];
					metadata["origin"] = ::fmt::format("{} {} {}", origin.x, origin.y, origin.z);
					metadata["angles"] = ::fmt::format("{} {} {}", angles.x, angles.y, angles.z);
					
					if ( context.skyArea != -1 ) {
						int32_t propLeafIdx = impl::findLeaf(context, origin);
						if ( propLeafIdx >= 0 && propLeafIdx < context.leafs.size() ) {
							int16_t propArea = context.leafs[propLeafIdx].area_flags & 0x01FF;
							if ( propArea == context.skyArea ) {
								metadata["skyboxed"] = true;
							}
						}
					}

					graph.root.children.emplace_back( nodeID );
				}
			}
			break; // no need to keep searching
		}
	}

	impl::processNodes( graph, context );

	// load materials
	uf::stl::vector<uint8_t> missing_pixels = { 255, 0, 255, 255, 0, 0, 0, 255, 0, 0, 0, 255, 255, 0, 255, 255 };
	for ( auto matName : graph.materials ) {
		uf::Serializer vmt;
		auto vmtPath = ::fmt::format("materials/{}.vmt", matName);
		auto vtfPath = ::fmt::format("materials/{}.vtf", matName);
		auto& image = storage.images[matName].data;
		auto& material = storage.materials[matName];

		if ( !image.getPixels().empty() ) continue;		
		if ( !ext::valve::loadVmt( vmt, vmtPath ) ) goto PEETAH;

		material.factorMetallic = vmt["$metalness"].as<float>(0.0f);
		material.factorRoughness = vmt["$roughness"].as<float>(1.0f);

		if ( vmt["$phong"].as<int>(0) == 1 ) {
			material.factorRoughness = std::min(material.factorRoughness, 0.5f);
		}
		if ( vmt["$translucent"].as<int>(0) == 1 ) {
			material.modeAlpha = pod::Material::AlphaMode::BLEND;
		}
		if ( vmt["$alphatest"].as<int>(0) == 1 ) {
			material.modeAlpha = pod::Material::AlphaMode::MASK;
			material.factorAlphaCutoff = vmt["$alphatestreference"].as<float>(0.5f);
		}
		if ( vmt["$nocull"].as<int>(0) == 1 ) {
			material.modeCull = pod::Material::CullMode::NONE;
		}

		if ( vmt["$selfillum"].as<int>(0) == 1 ) {
			material.colorEmissive = { 1.0f, 1.0f, 1.0f, 1.0f };
			material.modeAlpha = pod::Material::AlphaMode::EMISSIVE;
		}
		// cubemap
		if ( vmt["$envmap"].as<uf::stl::string>() != "" ) {
			auto matName = uf::string::lowercase(vmt["$envmap"].as<uf::stl::string>());
			auto vtfPath = ::fmt::format("materials/{}.vtf", matName);

			// retrieve
			if ( matName == "env_cubemap" ) {
				// is handled on an instance level
			} else if ( storage.images.map.count(matName) > 0 ) {
				material.indexCubemap = storage.textures[matName].index;
			} else {
				int32_t textureID = -1;
				impl::addMaterial( graph, matName, textureID );
				graph.metadata["cubemaps"].emplace_back( matName );
				
				auto& image = storage.images[matName].data;
				auto& texture = storage.images[matName].handle;
				texture.viewType = uf::renderer::enums::Image::VIEW_TYPE_CUBE;
				
				if ( ext::valve::loadVtf( image, vtfPath ) ) {
					material.indexCubemap = textureID;
				}
			}
		}
		// bumpmap
		if ( vmt["$ssbump"].as<int>(0) == 1 ) {
			// to-do: handle bumpmaps
		// normal map
		} else if ( vmt["$bumpmap"].is<uf::stl::string>() ) {
			auto matName = uf::string::lowercase(vmt["$bumpmap"].as<uf::stl::string>());
			auto vtfPath = ::fmt::format("materials/{}.vtf", matName);

			// retrieve
			if ( storage.images.map.count(matName) > 0 ) {
				material.indexNormal = storage.textures[matName].index;
			} else {
				int32_t textureID = -1;
				impl::addMaterial( graph, matName, textureID );
				
				auto& image = storage.images[matName].data;
				if ( ext::valve::loadVtf( image, vtfPath ) ) {
					material.indexNormal = textureID;
				}
			}
		}
		// metallic/roughness/occlusion map
		if ( vmt["$mrao"].is<uf::stl::string>() ) {
			auto matName = uf::string::lowercase(vmt["$mrao"].as<uf::stl::string>());
			auto vtfPath = ::fmt::format("materials/{}.vtf", matName);

			// retrieve
			if ( storage.images.map.count(matName) > 0 ) {
				material.indexMetallicRoughness = storage.textures[matName].index;
			} else {
				int32_t textureID = -1;
				impl::addMaterial( graph, matName, textureID );
				
				auto& image = storage.images[matName].data;
				if ( ext::valve::loadVtf( image, vtfPath ) ) material.indexMetallicRoughness = textureID;
			}
		}
		// albedo map
		if ( vmt["$basetexture"].is<uf::stl::string>() ) {
			vtfPath = ::fmt::format("materials/{}.vtf", vmt["$basetexture"].as<uf::stl::string>());
			if ( ext::valve::loadVtf( image, vtfPath ) ) {
				continue;
			}
		}

	PEETAH:
		image.loadFromBuffer( missing_pixels, { 2, 2 }, 8, 4 );
	}

	// disable exporting if loaded from a VPK
	if ( filename.starts_with("valve://") ) graph.metadata["exporter"]["enabled"] = false;
	graph.metadata["exporter"]["unwrap"] = false; // not necessary to unwrap

	uf::graph::postprocess( graph );

	// unmount pakfile
	uf::vfs::unmount( pakfileMount );
}