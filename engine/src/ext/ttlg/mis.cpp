#include <uf/ext/ttlg/common.h>
#include <uf/ext/ttlg/mis.h>
#include <uf/ext/ttlg/bin.h>
#include <uf/ext/ttlg/pcx.h>

#include <uf/ext/valve/common.h>
#include <uf/ext/zlib/zlib.h>
#include <uf/utils/mesh/grid.h>
#include <uf/utils/memory/unordered_set.h>

namespace impl {
#pragma pack(push, 1)
	struct DarkDBHeader {
		uint32_t inv_offset;
		uint32_t zero;
		uint32_t one;
		uint8_t zeros[256];
		uint32_t dead_beef;  // 0xEFBEADDE
	};

	struct DarkDBInvItem {
		char name[12];
		uint32_t offset;
		uint32_t length;
	};

	struct DarkDBChunkHeader {
		char name[12];
		uint32_t version_high;
		uint32_t version_low;
		uint32_t zero;
	};

	struct WRHeader {
		uint32_t unk;
		uint32_t numCells;
	};

	struct WRCellHeader {
		uint8_t numVertices;
		uint8_t numPolygons;
		uint8_t numTextured;
		uint8_t numPortals;
		uint8_t numPlanes;
		uint8_t mediaType;
		uint8_t cellFlags;
		uint32_t nxn;
		uint16_t polymapSize;
		uint8_t numAnimLights;
		uint8_t flowGroup;
		pod::Vector3f center;
		float radius;
	};

	struct WRPolygon {
		uint8_t flags;
		uint8_t count;
		uint8_t plane;
		uint8_t unk;
		uint16_t tgtCell;
		uint8_t unk1;
		uint8_t unk2;
	};

	struct WRPolygonTexturing {
		pod::Vector3f axisU;
		pod::Vector3f axisV;
		int16_t u;
		int16_t v;
		uint8_t txt;
		uint8_t originVertex;
		uint16_t unk;
		float scale;
		pod::Vector3f center;
	};

	struct WRLightInfo {
		int16_t u;
		int16_t v;
		uint16_t lx;
		uint8_t ly;
		uint8_t lx8;
		uint32_t staticLmapPtr;
		uint32_t dynamicLmapPtr;
		uint32_t animflags;
	};

	struct WRPlane {
		pod::Vector3f normal;
		float d;
	};

	struct DarkDBTXLIST_Header {
		uint32_t length;
		uint32_t txt_count;
		uint32_t fam_count;
	};

	struct DarkDBTXLIST_fam {
		char name[16];
	};

	struct DarkDBTXLIST_texture {
		uint8_t one;
		uint8_t fam;
		uint16_t zero;
		char name[16];
	};

	struct DarkDBObjVec_Header {
		int32_t minID;
		int32_t maxID;
	};

	struct PropertyEntry {
		int32_t objectId;
		uint32_t size;
	};

	struct LinkData {
		int32_t sourceId;
		int32_t destId;
		uint16_t flavor;
	};

	struct DarkPartitionedLink {
		uint32_t id;
		int32_t src;
		int32_t dest;
		uint16_t flavor;
	};

	// ordered in the way they're read per OpenDarkEngine
	struct PropertyPosition {
		pod::Vector3f position;
		int32_t cell;
		int16_t heading;
		int16_t pitch;
		int16_t bank;
	};

	struct PropertyLight {
		float brightness;
		float hue;
		float saturation;
		float z_offset;
		float radius;
	};

	struct PropertyAmbient {
		char schemaName[16];
		uint32_t flags;
		float volume;
		float radius;
	};

	// ?
	struct PropertyAnimLight {
		uint32_t mode;
		float millis;
		pod::Vector3f color;
		float brightness;
		float radius;
	};
#pragma pack(pop)

	// do not load these
	uf::stl::unordered_set<uf::stl::string> modelBlacklist = {
		"fx_particle.bin",
		"spark_.bin"
	};

	struct DarkContext {
		uf::stl::vector<uint8_t> buffer;

		uf::stl::unordered_map<uf::stl::string, impl::DarkDBInvItem> inventory;
		uf::stl::vector<uf::stl::string> families;

		uf::stl::unordered_map<int32_t, uf::stl::string> archetypes;
		uf::stl::unordered_map<int32_t, int32_t> parentMap;
		uf::stl::unordered_map<int32_t, uf::stl::string> modelNames;
		uf::stl::unordered_map<int32_t, uf::stl::string> customNames;
		
		uf::stl::vector<bool> objects;
		struct {
			uf::stl::unordered_map<int32_t, PropertyPosition> position;
			uf::stl::unordered_map<int32_t, PropertyLight> light;
			uf::stl::unordered_map<int32_t, PropertyAmbient> ambient;
			uf::stl::unordered_map<int32_t, uf::stl::vector<uf::stl::string>> script;
		} properties;

		uf::stl::vector<LinkData> links;
		uf::stl::unordered_map<uint16_t, uf::stl::string> linkFlavorNames;

		uf::stl::unordered_map<uf::stl::string, uf::stl::vector<uint8_t>> palettes;
		uf::stl::vector<int32_t> textureToMaterialId;
		pod::Atlas lightmapAtlas;

		template<typename T>
		bool findInheritedProperty(int32_t objId, const uf::stl::unordered_map<int32_t, T>& propMap, T& outValue) const {
			auto it = propMap.find(objId);
			if ( it != propMap.end() ) {
				outValue = it->second;
				return true;
			}
			auto pit = parentMap.find(objId);
			if ( pit != parentMap.end() && pit->second != 0 ) {
				return findInheritedProperty(pit->second, propMap, outValue);
			}
			return false;
		}
	};

	pod::Atlas::hash_t darkFaceHash(size_t cellIdx, size_t polyIdx) {
		return ::fmt::format("c_{}_p_{}", cellIdx, polyIdx);
	}

	uf::stl::string getStringFromOffset( const uf::stl::vector<uint8_t>& buffer, uint32_t offset, size_t maxScan = 128 ) {
		if ( offset >= buffer.size()) return "";
		size_t max_len = std::min<size_t>(maxScan, buffer.size() - offset);
		const char* ptr = (const char*)(buffer.data() + offset);
		return uf::stl::string(ptr, strnlen( ptr, max_len ));
	}

	void parseRelations( impl::DarkContext& ctx, const impl::DarkDBInvItem& item ) {
		auto& buffer = ctx.buffer;
		uint32_t offset = item.offset + sizeof(impl::DarkDBChunkHeader);
		uint32_t endOffset = item.offset + item.length;

		uint16_t flavorId = 1;
		while ( offset + 32 <= endOffset ) {
			uf::stl::string relName = uf::stl::string((const char*)(buffer.data() + offset));
			if ( !relName.empty() ) ctx.linkFlavorNames[flavorId] = relName;
			flavorId++;
			offset += 32;
		}
	}

	void parsePartitionedLinks( impl::DarkContext& ctx, const impl::DarkDBInvItem& item ) {
		auto& buffer = ctx.buffer;
		uint32_t offset = item.offset + sizeof(impl::DarkDBChunkHeader);
		uint32_t endOffset = item.offset + item.length;

		size_t count = (endOffset - offset) / sizeof(impl::DarkPartitionedLink);

		uf::stl::vector<impl::DarkPartitionedLink> chunkLinks;
		if ( !impl::readArray( buffer, offset, count, chunkLinks ) ) return;
		for ( const auto& l : chunkLinks ) {
			ctx.links.push_back({l.src, l.dest, l.flavor}); // to-do: emplace_back
		}
	}

	void parseProperty( impl::DarkContext& ctx, const uf::stl::string& propName ) {
		auto& buffer = ctx.buffer;
		auto& inventory = ctx.inventory;

		auto fullName = ::fmt::format( "P${}", propName );
		if ( inventory.count( fullName ) == 0 ) {
			return;
		}

		const auto& item = inventory.at(fullName);
		uint32_t offset = item.offset + sizeof(impl::DarkDBChunkHeader);
		uint32_t endOffset = item.offset + item.length;

		while ( offset < endOffset ) {
			PropertyEntry entry;
			if ( !impl::readStruct( buffer, offset, entry ) ) break;

			uint32_t next = offset + entry.size;

			if ( propName == "Position" && entry.size >= sizeof(PropertyPosition) && entry.objectId >= 0 ) {
				PropertyPosition position;
				if ( impl::readStruct( buffer, offset, position ) ) {
					ctx.properties.position[entry.objectId] = position;
				}
			} else if ( propName == "Light" && entry.size >= sizeof(PropertyLight) ) {
				PropertyLight light;
				if ( impl::readStruct( buffer, offset, light ) ) {
					ctx.properties.light[entry.objectId] = light;
				}
			} else if ( propName == "Ambient" && entry.size >= sizeof(PropertyAmbient) ) {
				PropertyAmbient sound;
				if ( impl::readStruct( buffer, offset, sound ) ) {
					ctx.properties.ambient[entry.objectId] = sound;
				}
			} else if ( propName == "Scripts" ) {
				const char* scriptCursor = (const char*)(buffer.data() + offset);
				size_t remainingBytes = entry.size;

				for ( auto s = 0; s < 4; ++s ) {
					if ( remainingBytes <= 0 || *scriptCursor == '\0' ) break;

					auto script = uf::stl::string( scriptCursor );
					if ( script.empty() ) break;

					ctx.properties.script[entry.objectId].emplace_back(script);

					size_t step = strnlen(scriptCursor, remainingBytes) + 1;
					if ( step > remainingBytes ) break;

					scriptCursor += step;
					remainingBytes -= step;
				}
			} else if ( propName == "SymName" ) {
				if ( entry.size > 4 ) {
					auto symName = impl::getStringFromOffset( buffer, offset + 4, entry.size - 4 );
					if ( !symName.empty() ) ctx.archetypes[entry.objectId] = symName;
				}
			} else if ( propName == "ModelName" ) {
				auto modelName = impl::getStringFromOffset(buffer, offset, entry.size);
				if ( !modelName.empty() ) ctx.modelNames[entry.objectId] = modelName;
			}

			offset = next;
		}
	}

	void parseObjVec( pod::Graph& graph, impl::DarkContext& ctx, const impl::DarkDBInvItem& item ) {
		auto& buffer = ctx.buffer;
		DarkDBObjVec_Header header;
		uint32_t offset = item.offset + sizeof(impl::DarkDBChunkHeader);
		if ( !impl::readStruct( buffer, offset, header ) ) return;

		if ( header.maxID <= 0 ) return;
		ctx.objects.assign( header.maxID, false );

		const uint8_t* bitmap = buffer.data() + offset;
		uint32_t bitArraySizeBytes = item.length - sizeof(impl::DarkDBChunkHeader) - 8;

		if ( offset + bitArraySizeBytes > buffer.size() ) return;

		for ( int32_t id = header.minID; id < header.maxID; ++id ) {
			if ( id < 0 ) continue;

			int32_t startByte = header.minID >> 3;
			int32_t myByte = id >> 3;
			int32_t byteIndex = myByte - startByte;
			int32_t bitOffset = id & 0x07;

			if ( byteIndex >= 0 && byteIndex < bitArraySizeBytes ) {
				ctx.objects[id] = (bitmap[byteIndex] & (1 << bitOffset)) != 0;
			}
		}
	}

	void parseTextures( pod::Graph& graph, impl::DarkContext& ctx, const impl::DarkDBInvItem& item ) {
		auto& buffer = ctx.buffer;
		uint32_t offset = item.offset + sizeof(impl::DarkDBChunkHeader);
		uint32_t size = item.length;
		DarkDBTXLIST_Header header;
		if ( !impl::readStruct( buffer, offset, header ) ) return;

		uf::stl::vector<DarkDBTXLIST_fam> fams;
		if ( impl::readArray( buffer, offset, header.fam_count, fams ) ) {
			ctx.families.resize( header.fam_count );
			for ( auto i = 0; i < header.fam_count; ++i ) {
				ctx.families[i] = uf::stl::string(fams[i].name, strnlen(fams[i].name, 16));
				std::transform(ctx.families[i].begin(), ctx.families[i].end(), ctx.families[i].begin(), ::tolower);
				std::replace(ctx.families[i].begin(), ctx.families[i].end(), '\\', '/');
			}
		}

		auto& storage = uf::graph::getStorage(graph);
		ctx.textureToMaterialId.resize( header.txt_count, -1 );

		uf::stl::vector<DarkDBTXLIST_texture> texs;
		if ( impl::readArray( buffer, offset, header.txt_count, texs ) ) {
			for (uint32_t i = 0; i < header.txt_count; ++i) {
				const auto& tex = texs[i];
				uf::stl::string texName(tex.name, strnlen(tex.name, 16));
				if (texName.empty() || texName == "null") continue;

				std::transform(texName.begin(), texName.end(), texName.begin(), ::tolower);
				std::replace(texName.begin(), texName.end(), '\\', '/');

				uf::stl::string matName = texName;
				if (tex.fam > 0 && tex.fam < ctx.families.size() && !ctx.families[tex.fam].empty()) {
					matName = ctx.families[tex.fam] + "/" + texName;
				}

				// could instead just check against storage.materials.map.count( matName )
				auto it = std::find( graph.materials.begin(), graph.materials.end(), matName );
				if ( it == graph.materials.end() ) {
					ctx.textureToMaterialId[i] = graph.materials.size();
					graph.materials.emplace_back(matName);
					storage.materials[matName].indexAlbedo = -1;
				} else {
					ctx.textureToMaterialId[i] = std::distance(graph.materials.begin(), it);
				}
			}
		}
	}

	void loadMaterials( pod::Graph& graph, impl::DarkContext& ctx ) {
		auto& storage = uf::graph::getStorage(graph);
		uf::stl::vector<uf::stl::string> extensions = { ".png", ".dds", ".tga", ".pcx", ".gif", ".bmp" };
		uf::stl::vector<uint8_t> missing_pixels = { 255, 0, 255, 255, 0, 0, 0, 255, 0, 0, 0, 255, 255, 0, 255, 255 };

		uf::stl::vector<uint8_t> buffer;
		for ( const auto& matName : graph.materials ) {
			auto& image = storage.images[matName].data;
			if ( !image.getPixels().empty() ) continue; // already loaded

			bool loaded = false;

			// sanitize name and family
			uf::stl::string family = "";
			uf::stl::string texName = matName;
			uf::stl::vector<uf::stl::string> searchDirs;
			size_t slashPos = matName.find('/');
			if ( slashPos != uf::stl::string::npos ) {
				family = matName.substr(0, slashPos);
				texName = matName.substr(slashPos + 1);
			}

			// family deduced, add it to search queue
			if ( !family.empty() ) {
				searchDirs.emplace_back("fam://" + family + "/");
				searchDirs.emplace_back("fam://" + family + "/anim/");
				for ( const auto& fam : ctx.families ) {
					if ( fam != family && !fam.empty() ) searchDirs.emplace_back("fam://" + fam + "/");
					if ( fam != family && !fam.empty() ) searchDirs.emplace_back("fam://" + fam + "/anim/");
				}
			// no family deduced, most likely an object
			} else {
				searchDirs.emplace_back("obj://txt16/");
				searchDirs.emplace_back("obj://txt/");
				searchDirs.emplace_back("obj://");
				for ( const auto& fam : ctx.families ) {
					if ( !fam.empty() ) {
						searchDirs.emplace_back("fam://" + fam + "/");
						searchDirs.emplace_back("fam://" + fam + "/anim/");
					}
				}
			}

			// search for target
			for ( const auto& dir : searchDirs ) {
				if ( loaded ) break;

				if ( dir.starts_with("fam://") ) {
					size_t fStart = 6;
					size_t fEnd = dir.find('/', fStart);
					if ( fEnd != uf::stl::string::npos ) family = dir.substr(fStart, fEnd - fStart);
				}

				for ( const auto& ext : extensions ) {
					uf::stl::string target = dir + texName + ext;
					// target doesn't exist
					if ( !uf::io::exists( target ) ) continue;
					// failed to read target
					if ( !uf::io::readAsBuffer( buffer, target ) ) continue;

					// is a PCX
					if ( ext == ".pcx" ) {
						if ( !family.empty() ) ext::ttlg::loadPalette( family, ctx.palettes[family] );
						const uint8_t* palette = (!family.empty() && !ctx.palettes[family].empty()) ? ctx.palettes[family].data() : nullptr;
						if ( !ext::ttlg::loadPcx( image, buffer, palette ) ) {
							UF_MSG_ERROR("Failed to load PCX: {}", target);
							continue;
						}
					// not a PCX, load it
					} else if ( !uf::image::open(image, buffer, target ) ) {
						continue;
					}

					// animated
					if ( dir.find("/anim/") != uf::stl::string::npos ) {
						UF_MSG_DEBUG("Animated texture found: {}", target);
					}

					loaded = true;
					break;
				}
			}

			// did not load, fallback to missing_texture
			if ( !loaded ) {
				UF_MSG_DEBUG("Could not load material: {}", texName);
				image.loadFromBuffer( missing_pixels, { 2, 2 }, 8, 4 );
			}

			size_t imageID = graph.images.size();
			size_t textureID = graph.textures.size();

			graph.images.emplace_back( matName );
			graph.textures.emplace_back( matName );
			storage.textures[matName].index = imageID;

			auto& material = storage.materials[matName];
			material.colorBase = { 1.0f, 1.0f, 1.0f, 1.0f };
			material.factorRoughness = 1.0f;
			material.factorMetallic = 0.0f;
			material.factorOcclusion = 1.0f;
			material.factorAlphaCutoff = 0.1f;
			material.indexAlbedo = textureID;
			material.indexNormal = -1;
			material.indexEmissive = -1;
			material.indexMetallicRoughness = -1;
			material.indexOcclusion = -1;
			material.indexCubemap = -1;

			// to-do: fill out
			if ( matName.find("glass") != uf::stl::string::npos ) {
				material.modeAlpha = pod::Material::AlphaMode::BLEND;
			}
		}
	}

	void parseWorld( pod::Graph& graph, impl::DarkContext& ctx, const impl::DarkDBInvItem& item ) {
		auto& buffer = ctx.buffer;
		uint32_t offset = item.offset + sizeof(impl::DarkDBChunkHeader);
		impl::WRHeader header;
		if ( !impl::readStruct( buffer, offset, header ) ) return;

		auto& storage = uf::graph::getStorage(graph);
		uf::stl::string meshName = "worldspawn";
		graph.meshes.emplace_back(meshName);
		graph.primitives.emplace_back(meshName);

		auto& mesh = storage.meshes[meshName];
		auto& primitives = storage.primitives[meshName];
		uf::stl::unordered_map<int32_t, impl::Meshlet> meshlets;

		// cell information
		struct ParsedCell {
			uint32_t cellIdx;
			impl::WRCellHeader header;
			uf::stl::vector<pod::Vector3f> vertices;
			uf::stl::vector<impl::WRPolygon> polys;
			uf::stl::vector<impl::WRPolygonTexturing> texInfo;
			uf::stl::vector<uf::stl::vector<uint8_t>> polyIndices;
			uf::stl::vector<impl::WRPlane> planes;
			uf::stl::vector<impl::WRLightInfo> lmInfos;
			uf::stl::vector<int16_t> animLightList;
			uf::stl::vector<uint16_t> lightIndices;
		};
		uf::stl::vector<ParsedCell> cells( header.numCells );

		// fill cell information
		for ( uint32_t c = 0; c < header.numCells; ++c ) {
			auto& cell = cells[c];
			cell.cellIdx = c;

			if ( !impl::readStruct( buffer, offset, cell.header ) ) break;

			// read cell information
			impl::readArray( buffer, offset, cell.header.numVertices, cell.vertices );
			impl::readArray( buffer, offset, cell.header.numPolygons, cell.polys );
			impl::readArray( buffer, offset, cell.header.numTextured, cell.texInfo );

			// read index count
			uint32_t numIndices;
			if ( !impl::readStruct( buffer, offset, numIndices ) ) break;
			if ( offset + numIndices > buffer.size() ) break;

			// fill indices
			cell.polyIndices.resize( cell.header.numPolygons );
			for ( uint8_t i = 0; i < cell.header.numPolygons; ++i ) {
				cell.polyIndices[i].resize( cell.polys[i].count );
				for ( uint8_t j = 0; j < cell.polys[i].count; ++j ) {
					cell.polyIndices[i][j] = buffer[offset++];
				}
			}

			// read planes
			impl::readArray( buffer, offset, cell.header.numPlanes, cell.planes );

			// read animated lights
			impl::readArray(buffer, offset, cell.header.numAnimLights, cell.animLightList);

			// read lightmaps information
			impl::readArray(buffer, offset, cell.header.numTextured, cell.lmInfos);

			// read lightmap
			uint32_t lightPixelSize = 2; // to-do: deduce?
			for ( uint8_t i = 0; i < cell.header.numTextured; ++i ) {
				int lmCount = 1;
				uint32_t flags = cell.lmInfos[i].animflags;
				while ( flags ) {
					if ( flags & 1 ) lmCount++;
					flags >>= 1;
				}

				uint32_t w = cell.lmInfos[i].lx;
				uint32_t h = cell.lmInfos[i].ly;
				uint32_t lmSizeBytes = w * h * lightPixelSize;

				if ( lmSizeBytes > 0 && ( offset + lmSizeBytes * lmCount ) <= buffer.size()) {
					uf::stl::vector<uint16_t> samples;
					impl::readArray( buffer, offset, w * h * lmCount, samples );

					for ( int layer = 0; layer < lmCount; ++layer ) {
						pod::Image image;
						image.size = { w, h };
						image.channels = 4;
						image.bpp = 32;
						image.pixels.resize( w * h * 4 );

						for ( auto y = 0; y < h; ++y ) {
							for ( auto x = 0; x < w; ++x ) {
								uint16_t sample = samples[(layer * w * h) + (y * w + x)];
								auto color = pod::Vector3f{
									(float)((sample >> 10) & 0x1F),
									(float)((sample >>  5) & 0x1F),
									(float)((sample	  ) & 0x1F),
								} / 31.0f;
								impl::encodeRGBE( color, &image.pixels[(y * w + x) * 4] );
							}
						}

						if ( layer == 0 ) {
							uf::atlas::add( ctx.lightmapAtlas, image, impl::darkFaceHash(c, i) );
						} else {
							int currentLayer = 1;
							int lightID = -1;
							for ( int bit = 0; bit < 32; ++bit ) {
								if ( cell.lmInfos[i].animflags & (1 << bit) ) {
									if ( currentLayer == layer ) {
										if ( bit < cell.animLightList.size() ) {
											lightID = cell.animLightList[bit];
										}
										break;
									}
									currentLayer++;
								}
							}

							pod::Atlas::hash_t animHash = ::fmt::format("c_{}_p_{}_anim_{}", c, i, lightID);
							uf::atlas::add( ctx.lightmapAtlas, image, animHash );
						}
					}
				} else {
					offset += ( lmSizeBytes * lmCount );
				}
			}

			uint32_t lightCount;
			if ( impl::readStruct( buffer, offset, lightCount ) ) {
				impl::readArray( buffer, offset, lightCount, cell.lightIndices );
			}
		}

		// combine lightmap into atlas
		auto atlasImageID = graph.images.size();
		auto atlasTextureID = graph.textures.size();

		if ( !ctx.lightmapAtlas.tiles.empty() ) {
			uf::atlas::generate(ctx.lightmapAtlas, 1);
			auto& imageKey = graph.images.emplace_back("lightmap_atlas");
			auto& textureKey = graph.textures.emplace_back("lightmap_atlas");

			storage.images[imageKey].data = uf::atlas::get( ctx.lightmapAtlas );
			storage.textures[textureKey].index = atlasImageID;
		}

		// build mesh from cells
		uf::stl::vector<uint32_t> indices;
		uf::stl::vector<pod::Vector2f> uvs;
		uf::stl::vector<pod::Vector2f> sts;
		for ( const auto& cell : cells ) {
			uint8_t solidPolys = cell.header.numPolygons - cell.header.numPortals;
			for ( uint8_t p = 0; p < solidPolys; ++p ) {
				const auto& poly = cell.polys[p];
				if ( p >= cell.header.numTextured || poly.plane >= cell.header.numPlanes) continue;

				const auto& lm = cell.lmInfos[p];
				const auto& texInfo = cell.texInfo[p];
				const auto& plane = cell.planes[poly.plane];

				// clear buffer
				indices.clear(); indices.resize(poly.count);
				uvs.clear(); uvs.resize(poly.count);
				sts.clear(); sts.resize(poly.count);

				// bind material information
				int32_t materialID = -1;
				uf::stl::string matName = "";
				if ( texInfo.txt < ctx.textureToMaterialId.size() ) {
					materialID = ctx.textureToMaterialId[texInfo.txt];
					if ( materialID >= 0 && materialID < graph.materials.size() ) {
						matName = graph.materials[materialID];
					}
				}

				// prepare meshlet
				auto& meshlet = meshlets[materialID];
				meshlet.primitive.instance.materialID = materialID;
				meshlet.primitive.instance.lightmapID = atlasTextureID;

				pod::Vector3f origin = cell.vertices[cell.polyIndices[p][texInfo.originVertex]];

				// project UV
				pod::Vector2f mag2 = { uf::vector::dot(texInfo.axisU, texInfo.axisU), uf::vector::dot(texInfo.axisV, texInfo.axisV) };
				float dotp = uf::vector::dot( texInfo.axisU, texInfo.axisV );

				pod::Vector2f sh = pod::Vector2f{ (float)texInfo.u, (float)texInfo.v } / 4096.0f;

				pod::Vector2f texSize = { 64.0f, 64.0f };
				if ( !matName.empty() && storage.images.map.count(matName) ) {
					const auto& img = storage.images[matName].data;
					if ( img.size.x > 0 && img.size.y > 0 ) {
						texSize = { (float)img.size.x, (float)img.size.y };
					}
				}

				pod::Vector2f rs = texSize / 64.0f;

				pod::Vector2f lmBase = { (float)lm.u, (float)lm.v };
				pod::Vector2f texBase = { (float)texInfo.u, (float)texInfo.v };
				pod::Vector2f lsh = (pod::Vector2f{0.5f, 0.5f} - lmBase) + (texBase / 1024.0f);

				float corr = 0.0f;
				float det = ( mag2.x * mag2.y - dotp * dotp );
				if ( std::abs(det) > EPS ) corr = 1.0f / det;

				pod::Vector2f c = { corr * mag2.y, corr * mag2.x };
				float cross = corr * dotp;

				pod::Vector2f minSt = { std::numeric_limits<float>::max(), std::numeric_limits<float>::max() };

				for ( uint8_t v = 0; v < poly.count; ++v ) {
					pod::Vector3f pos = cell.vertices[cell.polyIndices[p][v]];
					pod::Vector3f vrel = pos - origin;

					pod::Vector2f pr = { uf::vector::dot(texInfo.axisU, vrel), uf::vector::dot(texInfo.axisV, vrel) };
					pod::Vector2f proj;

					if ( dotp == 0.0f ) {
						proj = pr / mag2;
					} else {
						proj.x = pr.x * c.x - pr.y * cross;
						proj.y = pr.y * c.y - pr.x * cross;
					}

					uvs[v] = (proj + sh) / rs;
					sts[v] = (proj * 4.0f) + lsh;

					minSt = uf::vector::min(minSt, sts[v]);
				}

				pod::Vector2f lmSh = {
					impl::findWrap(minSt.x),
					impl::findWrap(minSt.y)
				};

				pod::Vector2f lmSize = {
					lm.lx > 0 ? (float)(lm.lx) : 0.5f,
					lm.ly > 0 ? (float)(lm.ly) : 0.5f
				};

				// write vertices
				for ( uint8_t v = 0; v < poly.count; ++v ) {
					indices[v] = meshlet.vertices.size();

					auto& vert = meshlet.vertices.emplace_back();
					vert.position = impl::convertPos_NewDark( cell.vertices[cell.polyIndices[p][v]] );
					vert.normal = impl::convertPos_NewDark( plane.normal, 1.0f );
					vert.color = { 255, 255, 255, 255 };
					vert.uv = uvs[v];
					vert.st = uf::atlas::mapUv(ctx.lightmapAtlas, (sts[v] + lmSh) / lmSize, impl::darkFaceHash(cell.cellIdx, p));
				}

				// triangle strip => triangle
				for ( uint8_t t = 1; t < poly.count - 1; ++t ) {
					meshlet.indices.emplace_back(indices[0]);
					meshlet.indices.emplace_back(indices[t]);
					meshlet.indices.emplace_back(indices[t + 1]);
				}
			}
		}

		if ( meshlets.empty()) return;

		size_t primitiveID = 0;
		for ( auto& [ matID, meshlet ] : meshlets ) {
			meshlet.primitive.drawCommand.indices = meshlet.indices.size();
			meshlet.primitive.drawCommand.vertices = meshlet.vertices.size();
			meshlet.primitive.instance.materialID = matID;
			meshlet.primitive.instance.primitiveID = primitiveID++;
			meshlet.primitive.instance.bounds = uf::mesh::bounds( meshlet.vertices );

			uf::mesh::tangents( meshlet.vertices, meshlet.indices );
		}

		if ( false ) {
			uf::meshgrid::Grid grid;
			grid.divisions = {8, 1, 8};
			auto mlets = uf::stl::values( meshlets );
			auto partitioned = uf::meshgrid::partition( grid, mlets, EPS, true, true );
			mesh.compile( partitioned, primitives );
		} else {
			mesh.compile( meshlets, primitives );
		}


		auto nodeID = graph.nodes.size();
		auto& node = graph.nodes.emplace_back();
		node.name = "worldspawn";
		node.mesh = (int32_t)(graph.meshes.size() - 1);
		graph.root.children.emplace_back(nodeID);
	}

	void loadObjects( pod::Graph& graph, const impl::DarkContext& ctx ) {
		for ( auto objectID = 0; objectID < ctx.objects.size(); ++objectID ) {
			if ( !ctx.objects[objectID] ) continue; // inactive object
			if ( ctx.properties.position.count(objectID) == 0 ) continue; // no position bound

			auto nodeID = graph.nodes.size();
			graph.root.children.emplace_back(nodeID);
			
			auto& node = graph.nodes.emplace_back();
			
			auto& metadata = node.metadata["dark"];

			// bind name
			if ( ctx.customNames.count(objectID) ) {
				node.name = uf::stl::string(ctx.customNames.at(objectID).c_str());
			} else {
				uf::stl::string symName;
				if ( ctx.findInheritedProperty(objectID, ctx.archetypes, symName) ) {
					node.name = uf::stl::string(symName.c_str());
				}
				if ( node.name.empty()) node.name = ::fmt::format("object #{}", objectID);
			}
			
			// bind position
			PropertyPosition position = ctx.properties.position.at(objectID); {
				// intentionally out of order so the struct can stay in the same order
				auto facing = pod::Vector3f{ position.heading, position.bank, position.pitch } * -M_PI / 32768.0f;

				auto qPitch   = uf::quaternion::axisAngle(pod::Vector3f{1.0f, 0.0f, 0.0f}, facing.x);
				auto qHeading = uf::quaternion::axisAngle(pod::Vector3f{0.0f, 1.0f, 0.0f}, facing.y);
				auto qBank	  = uf::quaternion::axisAngle(pod::Vector3f{0.0f, 0.0f, 1.0f}, facing.z);

				node.transform.position = impl::convertPos_NewDark( position.position );
				node.transform.orientation = uf::quaternion::multiply(qHeading, uf::quaternion::multiply(qBank, qPitch));
			
			}

			// bind model
			uf::stl::string modelName;
			if ( ctx.findInheritedProperty( objectID, ctx.modelNames, modelName ) && !modelName.empty() ) {
				uf::stl::string model = uf::stl::string(modelName.c_str());
				std::transform( model.begin(), model.end(), model.begin(), ::tolower );

				if ( !model.ends_with(".bin") ) model += ".bin";

				uf::stl::string path = "obj://" + model;
				auto it = std::find( graph.meshes.begin(), graph.meshes.end(), path );
				if ( it != graph.meshes.end() ) {
					node.mesh = (int32_t)std::distance(graph.meshes.begin(), it);
				} else if ( !impl::modelBlacklist.count( model ) ) {
					size_t meshID = graph.meshes.size();
					if ( ext::ttlg::loadBin( graph, path ) ) {
						node.mesh = (int32_t)(meshID);
					} else {
						node.mesh = -1;
					}
				}
			}

			// bind light
			PropertyLight light;
			if ( ctx.findInheritedProperty( objectID, ctx.properties.light, light ) ) {
				// create new node
				auto lightNodeID = graph.nodes.size();
				auto& lightNode = graph.nodes.emplace_back();
				lightNode.name = ::fmt::format("{}_light", node.name);
				graph.nodes[nodeID].children.emplace_back(lightNodeID);

				graph.lights[::fmt::format("{}_{}", lightNode.name, lightNodeID)] = {
					.range = light.radius,
					.color = impl::hsvToRgb(light.hue, light.saturation, 1.0f),
					.intensity = light.brightness / M_PI,
				};
			}

			// bind ambient sound
			PropertyAmbient ambient;
			if ( ctx.findInheritedProperty( objectID, ctx.properties.ambient, ambient ) ) {
				metadata["sound"]["schema"] = uf::stl::string(ambient.schemaName);
				metadata["sound"]["volume"] = ambient.volume;
				metadata["sound"]["radius"] = ambient.radius;
				metadata["sound"]["flags"]  = ambient.flags;
			}

			// bind script
			uf::stl::vector<uf::stl::string> scripts;
			if ( ctx.findInheritedProperty( objectID, ctx.properties.script, scripts ) ) {
				for ( const auto& s : scripts ) metadata["scripts"].emplace_back(s);
			}

			// fill out metadata
			{
				metadata["id"] = objectID;
				metadata["cell"] = position.cell;

				// to-do: deduce whether this should have physics
				if ( false ) {
					node.metadata["physics"]["type"] = "bounding box";
					node.metadata["physics"]["mass"] = 0;
				}
			}
		}
	}

	void processLinks( pod::Graph& graph, const impl::DarkContext& ctx ) {
		uf::stl::unordered_map<int32_t, size_t> objectToNode;
		for ( size_t i = 0; i < graph.nodes.size(); ++i ) {
			auto& node = graph.nodes[i];
			if ( node.metadata["dark"].isObject() && node.metadata["dark"]["id"].is<int>() ) {
				int32_t objId = node.metadata["dark"]["id"].as<int32_t>();
				objectToNode[objId] = i;
			}
		}

		for ( const auto& link : ctx.links ) {
			if ( !objectToNode.count(link.sourceId) || !objectToNode.count(link.destId) ) continue;
			size_t sourceIdx = objectToNode[link.sourceId];
			size_t destIdx  = objectToNode[link.destId];

			auto& srcNode = graph.nodes[sourceIdx];
			auto& connection = srcNode.metadata["dark"]["connections"].emplace_back();

			connection["target_node"] = graph.nodes[destIdx].name;
			connection["target_id"] = link.destId;
			connection["flavor"] = link.flavor;
		}
	}

	void readInventory( impl::DarkContext& ctx, const impl::DarkDBHeader& header ) {
		auto& buffer = ctx.buffer;
		auto& inventory = ctx.inventory;

		uint32_t chunkCount;
		uint32_t offset = header.inv_offset;
		if ( !impl::readStruct( buffer, offset, chunkCount ) ) return;
		for ( uint32_t i = 0; i < chunkCount; ++i ) {
			impl::DarkDBInvItem item;
			if ( !impl::readStruct( buffer, offset, item ) ) break;
			uf::stl::string name = item.name;
			inventory[name] = item;
		}

		// parse strings
		impl::parseProperty( ctx, "SymName" );
		impl::parseProperty( ctx, "ModelName" );
		// parse properties
		impl::parseProperty( ctx, "Position" );
		impl::parseProperty( ctx, "Light" );
		impl::parseProperty( ctx, "Ambient" );
		impl::parseProperty( ctx, "Scripts" );
		// parse links
		if ( inventory.count("Relations") > 0 ) {
			impl::parseRelations( ctx, inventory["Relations"] );
		}
		for ( const auto& [ name, item ] : inventory ) {
			if ( name.starts_with("L$") && !name.starts_with("LD$")) {
				impl::parsePartitionedLinks( ctx, item );
			}
		}

		// bind hierarchy
		{
			uf::stl::unordered_map<uint16_t, size_t> flavorCounts;
			int16_t bestFlavor = -1;
			size_t maxCount = 0;

			for ( const auto& link : ctx.links ) {
				if ( link.destId >= 0 ) continue;
				flavorCounts[link.flavor]++;
				if ( flavorCounts[link.flavor] <= maxCount ) continue;
				maxCount = flavorCounts[link.flavor];
				bestFlavor = link.flavor;
			}

			if ( bestFlavor != -1 ) {
				for ( const auto& link : ctx.links ) {
					if ( link.flavor != bestFlavor ) continue;
					if ( ctx.parentMap.find(link.sourceId) != ctx.parentMap.end() ) continue;
					ctx.parentMap[link.sourceId] = link.destId;
				}
			}
		}
	}

	void loadGam( impl::DarkContext& ctx, const uf::stl::string& filename ) {
		uf::stl::vector<uint8_t> buffer;
		if ( !uf::io::readAsBuffer( buffer, filename ) ) {
			UF_MSG_WARNING("Failed to read Gamesys data: {}. Archetypes will be missing!", filename);
			return;
		}

		uint32_t offset = 0;
		impl::DarkDBHeader header;
		if ( !impl::readStruct( buffer, offset, header ) ) return;

		if ( header.dead_beef != 0xEFBEADDE ) {
			UF_MSG_ERROR("Invalid DB in GAM: DEADBEEF not found.");
			return;
		}

		uf::stl::unordered_map<uf::stl::string, impl::DarkDBInvItem> inventory;
		std::swap( ctx.inventory, inventory );
		std::swap( ctx.buffer, buffer );
		impl::readInventory( ctx, header );
		std::swap( ctx.buffer, buffer );
		std::swap( ctx.inventory, inventory );
	}
}

void ext::ttlg::loadMis( pod::Graph& graph, const uf::stl::string& filename, const uf::Serializer& metadata ) {
	impl::DarkContext ctx;
	auto& buffer = ctx.buffer;
	if ( !uf::io::readAsBuffer( buffer, filename ) ) {
		UF_MSG_ERROR("Failed to read MIS data: {}", filename);
		return;
	}

	uint32_t offset = 0;
	impl::DarkDBHeader header;
	if ( !impl::readStruct( buffer, offset, header ) ) {
		UF_MSG_ERROR("Failed to read DB header: {}", filename);
		return;
	}
	if ( header.dead_beef != 0xEFBEADDE ) {
		UF_MSG_ERROR("Invalid DB: DEADBEEF not found: {}", filename);
		return;
	}
	if ( header.inv_offset >= buffer.size() ) {
		UF_MSG_ERROR("Inventory offset is past EOF: {}", filename);
		return;
	}

	uf::graph::preprocess( graph, metadata, filename );
	auto& storage = uf::graph::getStorage( graph );

	// mount files
	auto famMount = uf::vfs::mount(ext::zlib::createZipMount("fam://", "game://Data/res/FAM.CRF", 1000), true);
	auto mshMount = uf::vfs::mount(ext::zlib::createZipMount("obj://", "game://Data/res/MESH.CRF", 1000), true);
	auto bmpMount = uf::vfs::mount(ext::zlib::createZipMount("obj://", "game://Data/res/BITMAP.CRF", 1000), true);
	auto objMount = uf::vfs::mount(ext::zlib::createZipMount("obj://", "game://Data/res/OBJ.CRF", 1000), true);
	// load initial data
	impl::loadGam( ctx, /*metadata["game"].as<uf::stl::string>*/("game://Data/SHOCK2.GAM") ); // to-do: deduce path from metadata
	impl::readInventory( ctx, header );
	// mission parsing
	if ( ctx.inventory.count("TXLIST") > 0 ) {
		impl::parseTextures( graph, ctx, ctx.inventory["TXLIST"] );
		impl::loadMaterials( graph, ctx );
	}
	if ( ctx.inventory.count("WRRGB") > 0 ) {
		impl::parseWorld( graph, ctx, ctx.inventory["WRRGB"] );
	}
	if ( ctx.inventory.count("ObjVec") > 0 ) {
		impl::parseObjVec( graph, ctx, ctx.inventory["ObjVec"] );
		impl::loadObjects( graph, ctx );
		impl::loadMaterials( graph, ctx );
	}
	impl::processLinks( graph, ctx );
	// disable postprocessing flags
	if ( filename.starts_with("game://") ) graph.metadata["exporter"]["enabled"] = false; // disable exporting if loaded from a VPK
	graph.metadata["exporter"]["unwrap"] = false; // do not unwrap UVs for baking (we already have those)
	graph.metadata["baking"]["enabled"] = false; // disable lightmap baking (we already have those)

	uf::graph::postprocess(graph);
}