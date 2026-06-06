#include <uf/ext/valve/bsp.h>
#include <uf/ext/valve/mdl.h>
#include <uf/ext/valve/vtf.h>
#include <uf/ext/valve/vpk.h>
#include <uf/ext/valve/common.h>
#include <uf/ext/zlib/zlib.h>

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

	struct BspDispInfo {
		pod::Vector3f startPosition;
		int32_t dispVertStart;
		int32_t dispTriStart;
		int32_t power;
		int32_t minTess;
		int32_t maxTess;
		int32_t smoothingAngle;
		int32_t contents;
		uint16_t mapFace;
		int16_t lightmapAlphaStart;
		int32_t lightmapSamplePositionStart;
		uint8_t padding[130];
	};

	struct BspDispVert {
		pod::Vector3f vec;
		float dist;
		float alpha;
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
		// node, leaf
		// leafface, leaffbrush
		uf::stl::vector<impl::BspTexInfo> texinfos;
		uf::stl::vector<impl::BspTexData> texdatas;
		uf::stl::vector<int32_t> stringTable;
		uf::stl::string stringData;
		uf::stl::vector<impl::BspModel> models;
		// visibility
		uf::stl::vector<int8_t> entities;
		uf::stl::vector<impl::BspGameLump> gameLumps;
		uf::stl::vector<impl::BspDispInfo> dispinfos;
		uf::stl::vector<impl::BspDispVert> dispverts;
		// disptris
		uf::stl::vector<uint8_t> pakfile;
		// cubemaps
		// overlay
		uf::stl::vector<uint8_t> lighting;
		// ambient lighting
		// occlusion
		// physics
		// worldlight
		// other

		uf::stl::vector<int32_t> modelToMesh;
		uf::stl::vector<int32_t> texdataToMaterial;
		pod::Atlas lightmapAtlas;
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

			corners[i].st.x = (uf::vector::dot( v, texInfo.lightmapVecs[0] ) - face.lightmapTextureMins.x) / (face.lightmapTextureSize.x + 1.0f);
			corners[i].st.y = (uf::vector::dot( v, texInfo.lightmapVecs[1] ) - face.lightmapTextureMins.y) / (face.lightmapTextureSize.y + 1.0f);
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

			for ( int x = 0; x < side; ++x ) {
				float tx = (float)x / (side - 1);

				pod::Vector3f basePos = uf::vector::lerp( posE0, posE1, tx );
				pod::Vector2f finalUv = uf::vector::lerp( uvE0,  uvE1,  tx );
				pod::Vector2f finalSt = uf::vector::lerp( stE0,  stE1,  tx );

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

				meshlet.vertices[id].normal = normal;
				meshlet.vertices[id].tangent = tangent;
			}
		}

		for ( int y = 0; y < side - 1; ++y ) {
			for ( int x = 0; x < side - 1; ++x ) {
				uint32_t v0 = startVertexID + y * side + x;
				uint32_t v1 = startVertexID + y * side + (x + 1);
				uint32_t v2 = startVertexID + (y + 1) * side + x;
				uint32_t v3 = startVertexID + (y + 1) * side + (x + 1);

				// might need to flip winding order
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
		
		// has texture information
		if ( face.texinfo >= 0 && face.texinfo < context.texinfos.size() ) {
			const auto& info = context.texinfos[face.texinfo];
			if ( info.texData >= 0 && info.texData < context.texdatas.size() ) {
				const auto& data = context.texdatas[info.texData];

				v.uv.x = uf::vector::dot( vertex, info.textureVecs[0] ) / (float) data.width;
				v.uv.y = uf::vector::dot( vertex, info.textureVecs[1] ) / (float) data.height;
			}

			v.st.x = (uf::vector::dot( vertex, info.lightmapVecs[0] ) - face.lightmapTextureMins.x) / (face.lightmapTextureSize.x + 1.0f);
			v.st.y = (uf::vector::dot( vertex, info.lightmapVecs[1] ) - face.lightmapTextureMins.y) / (face.lightmapTextureSize.y + 1.0f);

			v.st = uf::atlas::mapUv( context.lightmapAtlas, v.st, impl::faceHash( faceID ) );

			v.tangent = uf::vector::normalize( impl::convertPos( info.textureVecs[0], 1) );
		}
	};

	template<typename T>
	uf::stl::vector<T> extractLump( const uf::stl::vector<uint8_t>& buffer, const impl::BspLump& lump ) {
		uf::stl::vector<T> data;
		if ( lump.length == 0 || lump.offset >= buffer.size() ) return data;

		size_t count = lump.length / sizeof(T);
		data.resize(count);
		std::copy(buffer.data() + lump.offset, buffer.data() + lump.offset + lump.length, (uint8_t*)(data.data()) );
		return data;
	}

	template<>
	uf::stl::vector<impl::BspGameLump> extractLump( const uf::stl::vector<uint8_t>& buffer, const impl::BspLump& lump ) {
		uf::stl::vector<impl::BspGameLump> data;
		if ( lump.length == 0 || lump.offset >= buffer.size() ) return data;

		const uint8_t* glData = buffer.data() + lump.offset;
		int32_t lumpCount = *(const int32_t*)glData;

		data.resize(lumpCount);
		std::copy(glData + 4, glData + 4 + (lumpCount * sizeof(impl::BspGameLump)), (uint8_t*)(data.data()));

		return data;
	}

	uf::stl::string extractLumpString( const uf::stl::vector<uint8_t>& buffer, const impl::BspLump& lump ) {
		auto data = impl::extractLump<char>( buffer, lump );
		return uf::stl::string( data.data(), data.size() );
	}

	void processNodes( pod::Graph& graph, const impl::BspContext& context, float scale = impl::sourceToMeters ) {
		size_t lights = 0;

		int32_t spawnID = -1;
		uf::stl::vector<size_t> spawns;

		for ( auto nodeID : graph.root.children ) {
			auto& node = graph.nodes[nodeID];
			auto classname = node.metadata["classname"].as<uf::stl::string>("");
			//UF_MSG_INFO("Entity found: {}", classname);
			node.name = classname;
			
			// parse origin
			auto origin = node.metadata["origin"].as<uf::stl::string>("");
			if ( origin != "" ) {
				auto position = impl::str2vec<pod::Vector3f>( origin );
				node.transform.position = impl::convertPos( position, scale );
			}

			// parse angles
			// to-do: fix oddities
			auto angles = node.metadata["angles"].as<uf::stl::string>("");
			if ( angles != "" ) {
				auto pyr = impl::str2vec<pod::Vector3f>( angles ) * DEG_2_RAD;
				pyr.x = -pyr.x;

				node.transform.orientation = uf::quaternion::euler( pyr );
			}

			// parse model
			auto model = node.metadata["model"].as<uf::stl::string>();
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
                    if ( ext::valve::loadMdl(graph, model) ) {
                        node.mesh = (int32_t)(graph.meshes.size() - 1);
                    }
                } else {
                    node.mesh = (int32_t)std::distance(graph.meshes.begin(), it);
                }
            }

			// parse lighting info
			if ( classname.starts_with("light") ) {
				auto lightKeyName = ::fmt::format( "{}_{}", classname, lights++ );
				auto& light = graph.lights[lightKeyName];
				light.color = { 1.0f, 1.0f, 1.0f };
				light.intensity = 1.0f;
				light.range = 0.0f;

				// read color and intensity
				auto _light = node.metadata["_light"].as<uf::stl::string>("");
				if ( _light != "" ) {
					// to-do: do not use stringstream
					std::istringstream stream(_light);
					light.color = { 255.0f, 255.0f, 255.0f };
					
					stream >> light.color.x >> light.color.y >> light.color.z;
					light.color /= 255.0f;

					if (!(stream >> light.intensity)) light.intensity = 200.0f;
					light.intensity /= 10.0f;
				}

				// to-do: read range
			}

			// parse player spawn info
			if ( classname == "info_player_start" ) {
				spawnID = nodeID;
			} else if ( classname.starts_with("info_player_") ) {
				spawns.emplace_back(nodeID);
			}

			// to-do: add additional parsing
		}

		// no valid spawn
		UF_ASSERT( !(spawnID == -1 && spawns.empty()) ); // to-do: make the engine implicitly spawn the player at origin
		// pick a random candidate if none was found
		if ( spawnID == -1 ) spawnID = uf::stl::random( spawns );
		for ( auto nodeID : spawns ) {
			auto& node = graph.nodes[nodeID];
			if ( nodeID == spawnID ) {
				node.name = "info_player_start"; // mutate into spawn
			} else if ( node.name == "info_player_start" ) {
				node.name = ::fmt::format( "_{}", node.name ); // mutate out of spawn
			}
		}
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
		UF_MSG_ERROR("Invalid VBSP magic number: {}", filename);
		return;
	}

	auto& storage = uf::graph::getStorage( graph );
	graph.name = filename;
	graph.metadata = metadata;
	graph.root.name = "%ROOT%";
	graph.root.index = -1;

	impl::BspContext context;
	context.vertices = impl::extractLump<impl::BspVertex>(buffer, header->lumps[impl::BspLump::LUMP_VERTICES]);
	context.edges = impl::extractLump<impl::BspEdge>(buffer, header->lumps[impl::BspLump::LUMP_EDGES]);
	context.surfedges = impl::extractLump<int32_t>(buffer, header->lumps[impl::BspLump::LUMP_SURFEDGES]);
	context.faces = impl::extractLump<impl::BspFace>(buffer, header->lumps[impl::BspLump::LUMP_FACES]);
	context.texinfos = impl::extractLump<impl::BspTexInfo>(buffer, header->lumps[impl::BspLump::LUMP_TEXINFO]);
	context.texdatas = impl::extractLump<impl::BspTexData>(buffer, header->lumps[impl::BspLump::LUMP_TEXDATA]);
	context.stringTable = impl::extractLump<int32_t>(buffer, header->lumps[impl::BspLump::LUMP_TEXDATA_STRING_TABLE]);
	context.stringData = impl::extractLumpString(buffer, header->lumps[impl::BspLump::LUMP_TEXDATA_STRING_DATA]);
	context.models = impl::extractLump<impl::BspModel>(buffer, header->lumps[impl::BspLump::LUMP_MODELS]);
	context.entities = impl::extractLump<int8_t>(buffer, header->lumps[impl::BspLump::LUMP_ENTITIES]);
	context.gameLumps = impl::extractLump<impl::BspGameLump>(buffer, header->lumps[impl::BspLump::LUMP_GAME_LUMP]);
	context.dispinfos = impl::extractLump<impl::BspDispInfo>(buffer, header->lumps[impl::BspLump::LUMP_DISPINFO]);
	context.dispverts = impl::extractLump<impl::BspDispVert>(buffer, header->lumps[impl::BspLump::LUMP_DISP_VERTS]);
	context.pakfile = impl::extractLump<uint8_t>(buffer, header->lumps[impl::BspLump::LUMP_PAKFILE]);
	context.lighting = impl::extractLump<uint8_t>(buffer, header->lumps[impl::BspLump::LUMP_LIGHTING]);

	context.modelToMesh.assign( context.models.size(), -1 );
	context.texdataToMaterial.assign( context.texdatas.size(), -1 );

	// mount pakfile
	size_t pakfileMount = uf::vfs::mount( ext::zlib::createZipMount(::fmt::format("pakfile://{}", filename), context.pakfile, 1000 ) );	

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

		size_t imageID = graph.images.size();
		auto imgKeyName = graph.images.emplace_back(matName);
		auto& image = storage.images[imgKeyName];

		size_t textureID = graph.textures.size();
		auto texKeyName = graph.textures.emplace_back(matName);
		storage.textures[texKeyName].index = imageID;
		storage.texture2Ds[texKeyName];

		size_t materialID = graph.materials.size();
		context.texdataToMaterial[texDataID] = materialID;

		auto matKeyName = graph.materials.emplace_back(matName);
		auto& material = storage.materials[matKeyName];
		material.indexAlbedo = textureID;
		material.colorBase = {1.0f, 1.0f, 1.0f, 1.0f};

		//UF_MSG_INFO("Material found: {}", matName);
	}

	// read lightmaps
	auto atlasImageID = graph.images.size();
	auto atlasTextureID = graph.textures.size();

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
		uf::atlas::generate( context.lightmapAtlas, 0.0f );
		//UF_MSG_DEBUG("Generated lightmap atlas.");

		auto& imageKey = graph.images.emplace_back("lightmap_atlas");
		auto& textureKey = graph.textures.emplace_back("lightmap_atlas");
		
		storage.images[imageKey] = uf::atlas::get( context.lightmapAtlas );
		storage.textures[textureKey].index = atlasImageID;
		storage.texture2Ds[textureKey];
	}

	// read models
	for ( auto m = 0; m < context.models.size(); ++m ) {
		const auto& model = context.models[m];
		uf::stl::unordered_map<int32_t, impl::Meshlet> meshlets; // group by material IDs

		for ( auto i = 0; i < model.numfaces; ++i ) {
			const auto faceID = model.firstface + i;
			const auto& face = context.faces[faceID];

			if ( face.numedges < 3 ) continue;
			if ( face.texinfo < 0 || face.texinfo >= context.texinfos.size() ) continue;

			int32_t texDataID = context.texinfos[face.texinfo].texData;
			if ( texDataID < 0 || texDataID >= context.texdatas.size() ) continue;

			size_t materialID = context.texdataToMaterial[texDataID];

			// read brush
			auto& meshlet = meshlets[materialID];
			meshlet.primitive.instance.materialID = materialID;
			meshlet.primitive.instance.lightmapID = atlasTextureID;
			
			if ( face.dispinfo != -1 ) {
				impl::buildDisplacement( context, meshlet, faceID );
				continue;
			}

			const auto edgeID = face.firstedge;
			int32_t pivotSurfEdge = context.surfedges[edgeID];
			uint16_t pivotVertID = pivotSurfEdge >= 0 ? context.edges[pivotSurfEdge].x : context.edges[-pivotSurfEdge].y;
			pod::Vector3f p0 = impl::convertPos( context.vertices[pivotVertID] );

			for ( int16_t i = 1; i < face.numedges - 1; ++i ) {
				int32_t se1 = context.surfedges[edgeID + i];
				int32_t se2 = context.surfedges[edgeID + i + 1];

				uint16_t v1 = se1 >= 0 ? context.edges[se1].x : context.edges[-se1].y;
				uint16_t v2 = se2 >= 0 ? context.edges[se2].x : context.edges[-se2].y;

				pod::Vector3f p1 = impl::convertPos( context.vertices[v1] );
				pod::Vector3f p2 = impl::convertPos( context.vertices[v2] );
				pod::Vector3f normal = uf::vector::normalize(uf::vector::cross(p1 - p0, p2 - p0));

				impl::addVertex( context, meshlet, pivotVertID, p0, normal, faceID );
				impl::addVertex( context, meshlet, v1, p1, normal, faceID );
				impl::addVertex( context, meshlet, v2, p2, normal, faceID );
			}
		}

		if ( meshlets.empty() ) continue;

		auto meshName = ::fmt::format("model_{}", m);
		context.modelToMesh[m] = graph.meshes.size();

		graph.meshes.emplace_back(meshName);
		graph.primitives.emplace_back(meshName);

		auto& mesh = storage.meshes[meshName];
		auto& primitives = storage.primitives[meshName];
		storage.instanceAddresses[meshName] = {};

		mesh.compile( meshlets, primitives );
	}

	// read entities
	{
		bool parsing = false;
		uf::stl::unordered_map<uf::stl::string, uf::stl::string> dict;
		
		uf::stl::string string( (const char*) context.entities.data() );
		uf::stl::string line;
		std::istringstream stream(string);
		while ( std::getline(stream, line) ) {
			if ( line.find("{") != uf::stl::string::npos ) {
				parsing = true;
				dict.clear();
			} else if ( line.find("}") != uf::stl::string::npos ) {
				parsing = false;
				
				// create node
				auto nodeID = graph.nodes.size();
				auto& node = graph.nodes.emplace_back();
				for ( const auto& [k, v] : dict ) node.metadata[k] = v; // store as metadata for later parsing

				// add node as child
				graph.root.children.emplace_back( nodeID );
			} else if ( parsing ) {
				uf::stl::string key, value;
				if ( impl::parseKeyValue(line, key, value) ) dict[key] = value;
			}
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

					node.metadata["classname"] = "prop_static";
					node.metadata["model"] = dict[type];
					node.metadata["origin"] = ::fmt::format("{} {} {}", origin.x, origin.y, origin.z);
					node.metadata["angles"] = ::fmt::format("{} {} {}", angles.x, angles.y, angles.z);

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
		auto& image = storage.images[matName];
		
		if ( !ext::valve::loadVmt( vmt, vmtPath ) ) goto PEETAH;
		if ( !vmt["$basetexture"].is<uf::stl::string>() ) goto PEETAH;

		vtfPath = ::fmt::format("materials/{}.vtf", vmt["$basetexture"].as<uf::stl::string>());
		if ( !ext::valve::loadVtf( image, vtfPath ) ) goto PEETAH;
		continue;
	PEETAH:
		image.loadFromBuffer( missing_pixels, { 2, 2 }, 8, 4 );
	}

	graph.metadata["exporter"]["unwrap"] = false; // not necessary to unwrap

	uf::graph::postprocess( graph );

	// unmount pakfile
	uf::vfs::unmount( pakfileMount );
}