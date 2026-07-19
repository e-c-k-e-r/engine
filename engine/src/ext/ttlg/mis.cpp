#include <uf/ext/ttlg/common.h>
#include <uf/ext/ttlg/mis.h>
#include <uf/ext/ttlg/bin.h>
#include <uf/ext/ttlg/pcx.h>

#include <uf/ext/valve/common.h>
#include <uf/ext/zlib/zlib.h>
#include <uf/utils/mesh/grid.h>
#include <uf/utils/memory/unordered_set.h>


// to-do: split this into subcomponents

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

	struct DarkLinkData {
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

	struct PropertyPosition {
		pod::Vector3f position;
		int32_t cell;
		int16_t facing[3]; // heading, pitch, bank
	};

	struct PropertyLight {
		float brightness;
		float hue;
		float saturation;
		float z_offset;
		float radius;
	};

	struct PropertyDoor {
		int32_t type;
		float closed;
		float open;
		float base_speed;
		int32_t axis;
		int32_t status;
		int32_t hard_limits;
		float sound_blocking;
		int32_t vision_blocking;
		float push_mass;
		// ...
	};

	struct PropertyAmbient {
		int32_t radius;
		int32_t override_volume;
		uint16_t flags;
		uint16_t pad;
		char schemaName[16];
		char auxSchema1[16];
		char auxSchema2[16];
	};

	// ?
	struct PropertyAnimLight {
		uint32_t mode;
		float millis;
		pod::Vector3f color;
		float brightness;
		float radius;
	};

	struct PropertyPhysType {
		int32_t type; // 0 = OBB, 1 = Sphere, 2 = SphereHat, 3 = None
		int32_t num_submodels;
		int32_t remove_on_sleep;
		int32_t special;
	};

	struct PropertyPhysAttr {
		float gravity;
		float mass;
		float density;
		float elasticity;
		float base_friction;
		pod::Vector3f cog_offset;
		int32_t rot_axes;
		int32_t rest_axes;
		int32_t climbable_sides;
		int32_t edge_trigger;
		float pore_size;
	};

	struct PropertyPhysDims {
		float radius[2];
		pod::Vector3f offset[2];
		pod::Vector3f size;
		int32_t pt_vs_terrain;
		int32_t pt_vs_not_special;
	};

	struct PropertyPhysState {
		pod::Vector3f location;
		pod::Vector3f facing;
		pod::Vector3f velocity;
		pod::Vector3f rot_velocity;
	};

	constexpr uint32_t SCH_PLAY_RETRIGGER = (1 << 0);
	constexpr uint32_t SCH_PAN_POS		= (1 << 1);
	constexpr uint32_t SCH_PAN_RANGE	  = (1 << 2);
	constexpr uint32_t SCH_NO_REPEAT	  = (1 << 3);
	constexpr uint32_t SCH_NO_CACHE	   = (1 << 4);
	constexpr uint32_t SCH_STREAM		 = (1 << 5);
	constexpr uint32_t SCH_PLAY_ONCE	  = (1 << 6);
	constexpr uint32_t SCH_NO_COMBAT	  = (1 << 7);
	constexpr uint32_t SCH_NET_AMBIENT	= (1 << 8);
	constexpr uint32_t SCH_LOC_SPATIAL	= (1 << 9);

	constexpr uint8_t SCHEMA_LOOP_POLY	= 0x01;
	constexpr uint8_t SCHEMA_LOOP_COUNT   = 0x02;

	struct PropertySchPlayParams {
		uint32_t flags;
		int32_t volume;
		int32_t pan;
		int32_t initialDelay;
		int32_t fade;
	};

	struct PropertySchLoopParams {
		uint8_t flags;
		uint8_t maxSamples;
		uint16_t count;
		uint16_t intervalMin;
		uint16_t intervalMax;
	};

	struct PropertySchSamp {
		uf::stl::string name;
		uint8_t weight;
	};

	struct PropertySongParams {
		char songName[32];
	};

	constexpr size_t kSONG_MaxStringLen = 32;

	struct sSongInfo {
		char id[kSONG_MaxStringLen];
	};

	struct sSongSectionInfo {
		char id[kSONG_MaxStringLen];
		int32_t volume;
		int32_t loopCount;
	};

	struct sSongSampleInfo {
		char name[kSONG_MaxStringLen];
	};

	struct sSongEventInfo {
		char eventString[kSONG_MaxStringLen];
		uint32_t flags;
	};

	struct sSongGotoInfo {
		int32_t sectionIndex;
		int32_t probability;
	};

	struct sTagDBData {
		int32_t objId;
		float weight;
	};

	struct cTagDBKey {
		uint32_t type;
		union {
			struct {
				int32_t minVal;
				int32_t maxVal;
			};
			uint8_t aEnum[8];
		};
	};
#pragma pack(pop)

	// pseudo-structs
	struct DarkSchema {
		uf::stl::string name;
		uf::stl::vector<PropertySchSamp> wavs;
		PropertySchPlayParams playParams{};
		PropertySchLoopParams loopParams{};

		uf::stl::string classTag;
		uf::stl::string msg;
		uf::stl::string action;

		// can probably deduce without these
		bool hasPlayParams = false;
		bool hasLoopParams = false;
	};

	struct SongGoto {
		int32_t sectionIndex;
		int32_t probability;
	};

	struct SongEvent {
		uf::stl::string eventString;
		uint32_t flags;
		uf::stl::vector<SongGoto> gotos;
	};

	struct SongSample {
		uf::stl::string name;
	};

	struct SongSection {
		uf::stl::string id;
		int32_t volume;
		int32_t loopCount;
		uf::stl::vector<SongSample> samples;
		uf::stl::vector<SongEvent> events;
	};

	struct Song {
		uf::stl::string id;
		uf::stl::vector<SongEvent> globalEvents;
		uf::stl::vector<SongSection> sections;
	};


	// do not load these
	uf::stl::unordered_set<uf::stl::string> modelBlacklist = {
		"playbox.bin",
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
			uf::stl::unordered_map<int32_t, PropertyDoor> rotDoor;
			uf::stl::unordered_map<int32_t, PropertyDoor> transDoor;

			uf::stl::unordered_map<int32_t, PropertyPhysType> physType;
			uf::stl::unordered_map<int32_t, PropertyPhysAttr> physAttr;
			uf::stl::unordered_map<int32_t, PropertyPhysDims> physDims;
			uf::stl::unordered_map<int32_t, PropertyPhysState> physState;

			uf::stl::unordered_map<int32_t, PropertyAmbient> ambient;

			uf::stl::unordered_map<int32_t, PropertySchPlayParams> schPlayParams;
			uf::stl::unordered_map<int32_t, PropertySchLoopParams> schLoopParams;
			uf::stl::unordered_map<int32_t, uf::stl::vector<PropertySchSamp>> schSamps;
			uf::stl::unordered_map<int32_t, uf::stl::string> classTag;
			uf::stl::unordered_map<int32_t, uf::stl::string> schMsg;
			uf::stl::unordered_map<int32_t, uf::stl::string> schAction;
			
			uf::stl::unordered_map<int32_t, uf::stl::vector<uf::stl::string>> script;
		} properties;

		uf::stl::unordered_map<uf::stl::string, uf::stl::string> fileDatabase;

		uf::stl::unordered_map<int32_t, DarkSchema> schemas;
		uf::stl::vector<DarkLinkData> links;
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

		auto fullName = ::fmt::format( "P${}", propName ).substr(0, 11);
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

			// to-do: clean up most of this redundancy
			if ( propName == "Position" && entry.objectId >= 0 ) {
				PropertyPosition position;
				if ( impl::readStruct( buffer, offset, position, entry.size ) ) {
					ctx.properties.position[entry.objectId] = position;
				}
			} else if ( propName == "Light" ) {
				PropertyLight light;
				if ( impl::readStruct( buffer, offset, light, entry.size ) ) {
					ctx.properties.light[entry.objectId] = light;
				}
			} else if ( propName == "RotDoor" ) {
				PropertyDoor p;
				if ( impl::readStruct( buffer, offset, p, entry.size ) ) {
					ctx.properties.rotDoor[entry.objectId] = p;
				}
			} else if ( propName == "TransDoor" ) {
				PropertyDoor p;
				if ( impl::readStruct( buffer, offset, p, entry.size ) ) {
					ctx.properties.transDoor[entry.objectId] = p;
				}
			} else if ( propName == "Ambient" || propName == "AmbientHacked" ) {
				PropertyAmbient sound;
				if ( impl::readStruct( buffer, offset, sound, entry.size ) ) {
					ctx.properties.ambient[entry.objectId] = sound;
				}
			} else if ( propName == "PhysType" ) {
				PropertyPhysType p;
				if ( impl::readStruct( buffer, offset, p, entry.size ) ) {
					ctx.properties.physType[entry.objectId] = p;
				}
			} else if ( propName == "PhysType" && entry.size >= 8 ) {
				PropertyPhysType p;
				if ( impl::readStruct( buffer, offset, p, entry.size ) ) {
					ctx.properties.physType[entry.objectId] = p;
				}
			} else if ( propName == "PhysAttr" && entry.size >= 48 ) {
				PropertyPhysAttr p;
				if ( impl::readStruct( buffer, offset, p, entry.size ) ) {
					ctx.properties.physAttr[entry.objectId] = p;
				}
			} else if ( propName == "PhysDims" && entry.size >= 48 ) {
				PropertyPhysDims p;
				if ( impl::readStruct( buffer, offset, p, entry.size ) ) {
					ctx.properties.physDims[entry.objectId] = p;
				}
			} else if ( propName == "PhysState" && entry.size >= 48 ) {
				PropertyPhysState p;
				if ( impl::readStruct( buffer, offset, p, entry.size ) ) {
					ctx.properties.physState[entry.objectId] = p;
				}
			} else if ( propName == "SchPlayParams" && entry.size >= sizeof(PropertySchPlayParams) ) {
				PropertySchPlayParams p;
				if ( impl::readStruct( buffer, offset, p, entry.size ) ) {
					ctx.properties.schPlayParams[entry.objectId] = p;
				}
			} else if ( propName == "SchLoopParams" && entry.size >= sizeof(PropertySchLoopParams) ) {
				PropertySchLoopParams p;
				if ( impl::readStruct( buffer, offset, p, entry.size ) ) {
					ctx.properties.schLoopParams[entry.objectId] = p;
				}
			} else if ( propName == "Scripts" ) {
				const char* cursor = (const char*)(buffer.data() + offset);
				size_t remainingBytes = entry.size;

				for ( auto s = 0; s < 4; ++s ) {
					if ( remainingBytes <= 0 || *cursor == '\0' ) break;

					auto script = uf::stl::string( cursor );
					if ( script.empty() ) break;

					ctx.properties.script[entry.objectId].emplace_back(script);

					size_t step = strnlen(cursor, remainingBytes) + 1;
					if ( step > remainingBytes ) break;

					cursor += step;
					remainingBytes -= step;
				}
			} else if ( propName == "Class Tag" || propName == "SchMsg" || propName == "SchAction" ) {
				if ( entry.size > 4 && entry.objectId != 0 ) {
					auto s = impl::getStringFromOffset( buffer, offset + 4, entry.size - 4 );
					if ( propName == "Class Tag" ) ctx.properties.classTag[entry.objectId] = s;
					if ( propName == "SchMsg" ) ctx.properties.schMsg[entry.objectId] = s;
					if ( propName == "SchAction" ) ctx.properties.schAction[entry.objectId] = s;
				}
			} /*else if ( propName == "Class Tag" ) {
				auto s = impl::getStringFromOffset( buffer, offset, entry.size );
				if ( !s.empty() ) ctx.properties.classTag[entry.objectId] = s;
			} else if ( propName == "SchMsg" ) {
				auto s = impl::getStringFromOffset( buffer, offset, entry.size );
				if ( !s.empty() ) ctx.properties.schMsg[entry.objectId] = s;
			} else if ( propName == "SchAction" ) {
				auto s = impl::getStringFromOffset( buffer, offset, entry.size );
				if ( !s.empty() ) ctx.properties.schAction[entry.objectId] = s;
			} */ else if ( propName == "ModelName" ) {
				auto modelName = impl::getStringFromOffset( buffer, offset, entry.size );
				if ( !modelName.empty() ) ctx.modelNames[entry.objectId] = modelName;
			} else if ( propName == "SymName" ) {
				if ( entry.size > 4 ) {
					auto symName = impl::getStringFromOffset( buffer, offset + 4, entry.size - 4 );
					if ( !symName.empty() ) ctx.archetypes[entry.objectId] = symName;
				}
			}

			offset = next;
		}
	}

	void parseEnvSoundTree(
		const uf::stl::vector<uint8_t>& buffer, uint32_t& offset,
		const uf::stl::unordered_map<int32_t, uf::stl::string>& tagMap,
		const uf::stl::unordered_map<int32_t, uf::stl::string>& valueMap,
		uf::stl::string currentTags,
		impl::DarkContext& ctx
	) {
		int32_t dataSize;
		if (!impl::readStruct(buffer, offset, dataSize)) return;

		for (int i = 0; i < dataSize; ++i) {
			sTagDBData data;
			if (!impl::readStruct(buffer, offset, data)) return;
			ctx.properties.classTag[data.objId] = currentTags;
		}

		int32_t branchSize;
		if (!impl::readStruct(buffer, offset, branchSize)) return;

		for (int i = 0; i < branchSize; ++i) {
			cTagDBKey key;
			if (!impl::readStruct(buffer, offset, key)) return;

			uf::stl::string newTags = currentTags;
			if (!newTags.empty()) newTags += ", ";

			uf::stl::string tagName = tagMap.count(key.type) ? tagMap.at(key.type) : ::fmt::format("UnknownTag_{}", key.type);
			uf::stl::string valueNames = "";

			bool isEnum = false;
			for (int v = 0; v < 8; ++v) {
				if (key.aEnum[v] == 255) isEnum = true;
			}

			if (isEnum || valueMap.count(key.aEnum[0])) {
				for (int v = 0; v < 8; ++v) {
					if (key.aEnum[v] != 255 && key.aEnum[v] != 0) {
						if (!valueNames.empty()) valueNames += "|";
						valueNames += valueMap.count(key.aEnum[v]) ? valueMap.at(key.aEnum[v]) : std::to_string(key.aEnum[v]);
					}
				}
				newTags += tagName + " " + valueNames;
			} else {
				newTags += tagName + " [" + std::to_string(key.minVal) + "-" + std::to_string(key.maxVal) + "]";
			}

			parseEnvSoundTree(buffer, offset, tagMap, valueMap, newTags, ctx);
		}
	}

	void parseSchSamp( impl::DarkContext& ctx, const impl::DarkDBInvItem& item ) {
		auto& buffer = ctx.buffer;
		uint32_t offset = item.offset + sizeof(impl::DarkDBChunkHeader);
		uint32_t endOffset = item.offset + item.length;

		while ( offset < endOffset ) {
			int32_t objId;
			if ( !impl::readStruct( buffer, offset, objId ) ) break;

			int32_t numSamples;
			if ( !impl::readStruct( buffer, offset, numSamples ) ) break;

			for ( int32_t i = 0; i < numSamples; ++i ) {
				int32_t strLen;
				if ( !impl::readStruct( buffer, offset, strLen ) ) break;

				uf::stl::string sampleName = impl::getStringFromOffset( buffer, offset, strLen );
				offset += strLen;

				uint8_t freq;
				if ( !impl::readStruct( buffer, offset, freq ) ) break;

				ctx.properties.schSamps[objId].emplace_back(PropertySchSamp{
					.name = sampleName,
					.weight = freq,
				});
			}
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

	void parseSchemas( impl::DarkContext& ctx ) {
		for ( const auto& [objId, wavs] : ctx.properties.schSamps ) {
			uf::stl::string schemaName;

			if ( ctx.customNames.count(objId) ) {
				schemaName = ctx.customNames.at(objId);
			} else if ( ctx.archetypes.count(objId) ) {
				schemaName = ctx.archetypes.at(objId);
			} else {
				continue;
			}

			std::transform(schemaName.begin(), schemaName.end(), schemaName.begin(), ::tolower);

			auto& schema = ctx.schemas[objId];
			schema.name = schemaName;

			ctx.findInheritedProperty(objId, ctx.properties.classTag, schema.classTag);
			ctx.findInheritedProperty(objId, ctx.properties.schMsg, schema.msg);
			ctx.findInheritedProperty(objId, ctx.properties.schAction, schema.action);

			for ( const auto& sample : wavs ) {
				uf::stl::string wavLower = sample.name;
				std::transform(wavLower.begin(), wavLower.end(), wavLower.begin(), ::tolower);
				if (!wavLower.ends_with(".wav")) wavLower += ".wav";

				uf::stl::string baseName = wavLower;
				size_t slashPos = baseName.find_last_of('/');
				if (slashPos != uf::stl::string::npos) {
					baseName = baseName.substr(slashPos + 1);
				}

				uf::stl::string resolvedPath = "SND://" + baseName;
				if ( ctx.fileDatabase.count(baseName) ) {
					resolvedPath = ctx.fileDatabase[baseName];
				}

				schema.wavs.emplace_back(PropertySchSamp{
					.name = resolvedPath,
					.weight = sample.weight,
				});
			}

			schema.hasPlayParams = ctx.findInheritedProperty(objId, ctx.properties.schPlayParams, schema.playParams);
			schema.hasLoopParams = ctx.findInheritedProperty(objId, ctx.properties.schLoopParams, schema.loopParams);
		}
	}

	bool parseSong( const uf::stl::vector<uint8_t>& buffer, impl::Song& outSong ) {
		uint32_t offset = 0;

		uint32_t fileVersion;
		if ( !impl::readStruct( buffer, offset, fileVersion ) ) return false;
		if ( fileVersion != 1) {
			UF_MSG_WARNING("Song file version mismatch (expected 1, got {})", fileVersion);
		}

		impl::sSongInfo songInfo;
		if ( !impl::readStruct( buffer, offset, songInfo ) ) return false;
		outSong.id = uf::stl::string(songInfo.id, strnlen(songInfo.id, kSONG_MaxStringLen));

		uint32_t numGlobalEvents;
		if ( !impl::readStruct( buffer, offset, numGlobalEvents ) ) return false;

		for ( uint32_t i = 0; i < numGlobalEvents; ++i ) {
			impl::sSongEventInfo evtInfo;
			if ( !impl::readStruct( buffer, offset, evtInfo ) ) break;

			auto& evt = outSong.globalEvents.emplace_back();
			evt.eventString = uf::stl::string(evtInfo.eventString, strnlen(evtInfo.eventString, kSONG_MaxStringLen));
			evt.flags = evtInfo.flags;

			uint32_t numGotos;
			if ( !impl::readStruct( buffer, offset, numGotos ) ) break;

			for ( uint32_t j = 0; j < numGotos; ++j ) {
				impl::sSongGotoInfo gotoInfo;
				if ( !impl::readStruct( buffer, offset, gotoInfo ) ) break;
				evt.gotos.push_back({ gotoInfo.sectionIndex, gotoInfo.probability });
			}
		}

		uint32_t numSections;
		if ( !impl::readStruct( buffer, offset, numSections ) ) return false;

		for ( uint32_t i = 0; i < numSections; ++i ) {
			impl::sSongSectionInfo secInfo;
			if ( !impl::readStruct( buffer, offset, secInfo ) ) break;

			auto& sec = outSong.sections.emplace_back();
			sec.id = uf::stl::string(secInfo.id, strnlen(secInfo.id, kSONG_MaxStringLen));
			sec.volume = secInfo.volume;
			sec.loopCount = secInfo.loopCount;

			uint32_t numSamples;
			if ( !impl::readStruct( buffer, offset, numSamples ) ) break;

			for ( uint32_t j = 0; j < numSamples; ++j ) {
				impl::sSongSampleInfo sampInfo;
				if ( !impl::readStruct( buffer, offset, sampInfo ) ) break;

				uf::stl::string sampleName = uf::stl::string(sampInfo.name, strnlen(sampInfo.name, kSONG_MaxStringLen));
				sec.samples.push_back({ sampleName });
			}

			uint32_t numSecEvents;
			if ( !impl::readStruct( buffer, offset, numSecEvents ) ) break;

			for ( uint32_t j = 0; j < numSecEvents; ++j ) {
				impl::sSongEventInfo evtInfo;
				if ( !impl::readStruct( buffer, offset, evtInfo ) ) break;

				auto& evt = sec.events.emplace_back();
				evt.eventString = uf::stl::string(evtInfo.eventString, strnlen(evtInfo.eventString, kSONG_MaxStringLen));
				evt.flags = evtInfo.flags;

				uint32_t numGotos;
				if ( !impl::readStruct( buffer, offset, numGotos ) ) break;

				for ( uint32_t k = 0; k < numGotos; ++k ) {
					impl::sSongGotoInfo gotoInfo;
					if ( !impl::readStruct( buffer, offset, gotoInfo ) ) break;
					evt.gotos.push_back({ gotoInfo.sectionIndex, gotoInfo.probability });
				}
			}
		}

		return true;
	}

	void loadMaterials( pod::Graph& graph, impl::DarkContext& ctx ) {
		auto& storage = uf::graph::getStorage(graph);
		uf::stl::vector<uint8_t> missing_pixels = { 255, 0, 255, 255, 0, 0, 0, 255, 0, 0, 0, 255, 255, 0, 255, 255 };
		uf::stl::vector<uint8_t> buffer;

		for ( const auto& matName : graph.materials ) {
			auto& image = storage.images[matName].data;
			if ( !image.getPixels().empty() ) continue; // already loaded

			bool loaded = false;
			uf::stl::string family = "";
			uf::stl::string texName = matName;

			size_t slashPos = matName.find('/');
			if ( slashPos != uf::stl::string::npos ) {
				family = matName.substr(0, slashPos);
				texName = matName.substr(slashPos + 1);
			}

			uf::stl::string targetPath = "";
			if ( ctx.fileDatabase.count(matName) ) {
				targetPath = ctx.fileDatabase[matName];
			} else {
				for ( const auto& fam : ctx.families ) {
					if ( ctx.fileDatabase.count(fam + "/" + texName) ) {
						targetPath = ctx.fileDatabase[fam + "/" + texName];
						family = fam;
						break;
					}
				}
			}
			if ( targetPath.empty() && ctx.fileDatabase.count(texName) ) {
				targetPath = ctx.fileDatabase[texName];
			}

			if ( !targetPath.empty() && uf::io::readAsBuffer( buffer, targetPath ) ) {
				uf::stl::string lowerTarget = targetPath;
				std::transform(lowerTarget.begin(), lowerTarget.end(), lowerTarget.begin(), ::tolower);

				if ( lowerTarget.ends_with(".pcx") ) {
					if ( !family.empty() ) ext::ttlg::loadPalette( family, ctx.palettes[family] );
					const uint8_t* palette = (!family.empty() && !ctx.palettes[family].empty()) ? ctx.palettes[family].data() : nullptr;
					if ( !ext::ttlg::loadPcx( image, buffer, palette ) ) {
						UF_MSG_ERROR("Failed to load PCX: {}", targetPath);
					} else {
						loaded = true;
					}
				} else if ( uf::image::open(image, buffer, targetPath ) ) {
					loaded = true;
				}

				if ( loaded && lowerTarget.find("/anim/") != uf::stl::string::npos ) {
					UF_MSG_DEBUG("Animated texture found: {}", targetPath);
				}
			}

			if ( !loaded ) {
				UF_MSG_DEBUG("Could not load material: {}", matName);
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
		//uf::stl::unordered_map<std::pair<size_t, size_t>, impl::Meshlet> meshlets;
		uf::stl::unordered_map<uint64_t, impl::Meshlet> meshlets;

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

		auto atlasImageID = -1;
		auto atlasTextureID = -1;

		// combine lightmap into atlas
		if ( !ctx.lightmapAtlas.tiles.empty() ) {
			atlasImageID = graph.images.size();
			atlasTextureID = graph.textures.size();

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
				if ( materialID == -1 ) {
					materialID = 0;
				}

				// prepare meshlet
				//auto& meshlet = meshlets[std::make_pair(cell.cellIdx, materialID)];
				auto& meshlet = meshlets[(static_cast<uint64_t>(cell.cellIdx) << 32) | static_cast<uint32_t>(materialID)];
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
		for ( auto& [ meshletKey, meshlet ] : meshlets ) {
		//	auto& [ auxID, matID ] = meshletKey;
			uint32_t auxID = static_cast<uint32_t>(meshletKey >> 32);
			uint32_t matID = static_cast<uint32_t>(meshletKey & 0xFFFFFFFF);

			meshlet.primitive.drawCommand.indices = meshlet.indices.size();
			meshlet.primitive.drawCommand.vertices = meshlet.vertices.size();
			meshlet.primitive.instance.materialID = matID;
			meshlet.primitive.instance.auxID = auxID;
			meshlet.primitive.instance.primitiveID = primitiveID++;
			meshlet.primitive.instance.bounds = uf::mesh::bounds( meshlet.vertices );

			uf::mesh::tangents( meshlet.vertices, meshlet.indices );
		}

		mesh.compile( meshlets, primitives );


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
				// auto facing = pod::Vector3f{ position.heading, position.bank, position.pitch } * -M_PI / 32768.0f;
				auto facing = pod::Vector3f{ position.facing[0], position.facing[2], position.facing[1] } * -M_PI / 32768.0f;

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

				uf::stl::string path = "OBJ://" + model;
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

			// bind door
			PropertyDoor doorProp;
			bool isDoor = false;

			if ( ctx.findInheritedProperty( objectID, ctx.properties.rotDoor, doorProp ) ) {
				metadata["door"]["is_rotating"] = true;
				isDoor = true;
			} else if ( ctx.findInheritedProperty( objectID, ctx.properties.transDoor, doorProp ) ) {
				metadata["door"]["is_rotating"] = false;
				isDoor = true;
			}

			if ( isDoor ) {
				metadata["door"]["closed"] = doorProp.closed;
				metadata["door"]["open"] = doorProp.open;
				metadata["door"]["speed"] = doorProp.base_speed * impl::darkToMeters;
				metadata["door"]["axis"] = doorProp.axis;
				metadata["door"]["status"] = doorProp.status;
			}

			// bind class tags
			uf::stl::string classTags;
			if ( ctx.findInheritedProperty( objectID, ctx.properties.classTag, classTags ) ) {
				UF_MSG_DEBUG("objectID={}, name={}, classTags={}", objectID, node.name, classTags);
				metadata["class_tags"] = classTags;
			}

			// bind ambient sound
			PropertyAmbient ambient;
			if ( ctx.findInheritedProperty( objectID, ctx.properties.ambient, ambient ) ) {
				uf::stl::string schemaName(ambient.schemaName, strnlen(ambient.schemaName, 16));

				metadata["sound"]["schema"] = schemaName;
				metadata["sound"]["volume"] = ambient.override_volume;
				metadata["sound"]["radius"] = ambient.radius * impl::darkToMeters;
				metadata["sound"]["flags"]  = ambient.flags;

				uf::stl::string targetSchema = schemaName;
				std::transform(targetSchema.begin(), targetSchema.end(), targetSchema.begin(), ::tolower);

				int32_t schemaObjId = -1;
				for ( const auto& [id, resolved] : ctx.schemas ) {
					if ( resolved.name == targetSchema ) {
						schemaObjId = id;
						break;
					}
				}

				if ( schemaObjId != -1 ) {
					const auto& resolved = ctx.schemas.at(schemaObjId);

					for ( const auto& wav : resolved.wavs ) {
						auto& metadataWav = metadata["sound"]["wavs"].emplace_back();
						metadataWav["uri"] = wav.name;
						metadataWav["weight"] = wav.weight;
					}

					if ( resolved.hasPlayParams ) {
						metadata["sound"]["schema_volume"] = resolved.playParams.volume;
						metadata["sound"]["play_once"] = (resolved.playParams.flags & impl::SCH_PLAY_ONCE) != 0;
						metadata["sound"]["stream"] = (resolved.playParams.flags & impl::SCH_STREAM) != 0;
					}

					if ( resolved.hasLoopParams ) {
						metadata["sound"]["interval_min"] = resolved.loopParams.intervalMin;
						metadata["sound"]["interval_max"] = resolved.loopParams.intervalMax;
					}
				}
			}

			// bind script
			uf::stl::vector<uf::stl::string> scripts;
			if ( ctx.findInheritedProperty( objectID, ctx.properties.script, scripts ) ) {
				for ( const auto& s : scripts ) metadata["scripts"].emplace_back(s);
			}

			// bind physics
			PropertyPhysType physType;
			// to-do: optimize this as it cuts my FPS by ~30% with lots of small physics objects
			if ( ctx.findInheritedProperty( objectID, ctx.properties.physType, physType ) && physType.type != 3 ) {
				auto& physMeta = node.metadata["physics"];

				if ( physType.type == 0 ) {
					physMeta["type"] = "obb";
				} else if ( physType.type == 1 || physType.type == 2 ) {
					physMeta["type"] = "sphere";
					physMeta["radius"] = 0.5f;
				} else {
					physMeta["type"] = "mesh";
				}

				PropertyPhysAttr physAttr;
				if ( ctx.findInheritedProperty( objectID, ctx.properties.physAttr, physAttr ) ) {
					physMeta["mass"] = physAttr.mass;
					physMeta["friction"] = physAttr.base_friction;
					physMeta["restitution"] = physAttr.elasticity;

					if ( physAttr.gravity == 0.0f ) {
						physMeta["gravity"] = uf::vector::encode(pod::Vector3f{0.0f, 0.0f, 0.0f});
					} else {
						physMeta["gravity"] = uf::vector::encode(pod::Vector3f{0.0f, -0.0981f * physAttr.gravity, 0.0f});
					}

					if ( physAttr.edge_trigger != 0 ) {
						physMeta["category"] = "trigger";
						physMeta["inertia"] = false;
						physMeta["mass"] = 0.0f;
					}
					if ( physType.type == 0 ) {
						physMeta["mass"] = 0.0f;
					}
				}

				PropertyPhysDims physDims;
				if ( ctx.findInheritedProperty( objectID, ctx.properties.physDims, physDims ) ) {
					if ( physType.type == 0 ) {
						pod::Vector3f extent = impl::convertPos_NewDark(physDims.size) * 0.5f;
						if ( extent.x > 0 && extent.y > 0 && extent.z > 0 ) {
							physMeta["extent"] = uf::vector::encode(extent);
						}

						pod::Vector3f center = impl::convertPos_NewDark(physDims.offset[0]);
						if ( center.x != 0.f || center.y != 0.f || center.z != 0.f ) {
							physMeta["center"] = uf::vector::encode(center);
						}
					} else if ( physType.type == 1 || physType.type == 2 ) {
						if ( physDims.radius[0] > 0 ) {
							physMeta["radius"] = physDims.radius[0] * impl::darkToMeters;
						}

						pod::Vector3f offset = impl::convertPos_NewDark(physDims.offset[0]);
						if ( offset.x != 0.f || offset.y != 0.f || offset.z != 0.f ) {
							physMeta["offset"] = uf::vector::encode(offset);
						}
					}
				}

				PropertyPhysState physState;
				if ( ctx.findInheritedProperty( objectID, ctx.properties.physState, physState ) ) {
					pod::Vector3f vel = impl::convertPos_NewDark(physState.velocity, 1.0f);
					if ( vel.x != 0.f || vel.y != 0.f || vel.z != 0.f ) physMeta["velocity"] = uf::vector::encode(vel);

					pod::Vector3f angVel = { physState.rot_velocity.x, physState.rot_velocity.z, physState.rot_velocity.y };
					if ( angVel.x != 0.f || angVel.y != 0.f || angVel.z != 0.f ) physMeta["angularVelocity"] = uf::vector::encode(angVel);
				}
			}

			// fill out metadata
			{
				metadata["id"] = objectID;
				metadata["cell"] = position.cell;
			}

			// bind light (at the end because we insert a new node)
			PropertyLight light;
			if ( ctx.findInheritedProperty( objectID, ctx.properties.light, light ) ) {
				// create new node
				auto lightNodeName = ::fmt::format("{}_light", node.name);
				auto lightNodeID = graph.nodes.size();
				auto& lightNode = graph.nodes.emplace_back();
				lightNode.name = lightNodeName;
				graph.nodes[nodeID].children.emplace_back(lightNodeID);

				graph.lights[::fmt::format("{}_{}", lightNode.name, lightNodeID)] = {
					.range = light.radius,
					.color = impl::hsvToRgb(light.hue, light.saturation, 1.0f),
					.intensity = light.brightness / M_PI,
				};
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

			uf::stl::string flavor = "";
			if ( ctx.linkFlavorNames.count(link.flavor) ) {
				flavor = ctx.linkFlavorNames.at(link.flavor);
			} else {
				flavor = ::fmt::format("Unknown_{}", link.flavor);
			}

			connection["flavor"] = flavor;
			connection["target_node"] = graph.nodes[destIdx].name;
			connection["target_id"] = link.destId;

			// bind sound descriptions
			if ( flavor == "SoundDescription" ) {
				if ( ctx.schemas.count(link.destId) ) {
					const auto& schema = ctx.schemas.at(link.destId);
					for (const auto& wav : schema.wavs) {
						connection["wavs"].emplace_back(wav.name);
					}
				}
			}
		}
	}

	void processSongs( pod::Graph& graph, impl::DarkContext& ctx ) {
		uf::stl::vector<uf::stl::string> songFiles = uf::vfs::list("SND://song/", ".snc");
		if ( songFiles.empty() ) songFiles = uf::vfs::list("SND://", ".snc");

		auto nodeID = graph.nodes.size();
		auto& node = graph.nodes.emplace_back();
		
		graph.root.children.emplace_back( nodeID );
		node.name = "Song";

		 uf::stl::string missionSong = "";
		if ( ctx.inventory.count("SONGPARAMS") > 0 ) {
			const auto& item = ctx.inventory.at("SONGPARAMS");
			uint32_t offset = item.offset + sizeof(impl::DarkDBChunkHeader);
			impl::PropertySongParams params;
			if ( impl::readStruct( ctx.buffer, offset, params ) ) {
				missionSong = uf::stl::string(params.songName, strnlen(params.songName, 32));
			}
		}
		node.metadata["dark"]["mission song"] = missionSong;

		auto& musicMarkersMeta = node.metadata["dark"]["music markers"];
		for ( auto& node : graph.nodes) {
			auto& soundMeta = node.metadata["dark"]["sound"];
			if ( !soundMeta.isObject() ) continue;
			int flags = soundMeta["flags"].as<int>(0);
			if ((flags & 16) != 0) {
				auto& marker = musicMarkersMeta.emplace_back();

				marker["position"] = uf::vector::encode(node.transform.position);
				marker["radius"] = soundMeta["radius"].as<float>();
				marker["theme"] = soundMeta["schema"].as<uf::stl::string>();
			}
		}

		auto& songMeta = node.metadata["dark"]["songs"];
		for ( const auto& sncPath : songFiles ) {
			uf::stl::vector<uint8_t> sncBuf;
			impl::Song song;

			if ( !uf::io::readAsBuffer( sncBuf, sncPath ) || !impl::parseSong( sncBuf, song ) ) continue;

			uf::stl::string songId = sncPath;

			size_t slashPos = songId.find_last_of('/');
			if ( slashPos != uf::stl::string::npos ) songId = songId.substr(slashPos + 1);

			size_t dotPos = songId.find_last_of('.');
			if ( dotPos != uf::stl::string::npos ) songId = songId.substr(0, dotPos);

			std::transform(songId.begin(), songId.end(), songId.begin(), ::tolower);

			auto& sm = songMeta[songId];

			for ( const auto& evt : song.globalEvents ) {
				auto& em = sm["events"].emplace_back();
				em["event"] = evt.eventString;
				em["flags"] = evt.flags;
				for ( const auto& gt : evt.gotos ) {
					auto& gm = em["gotos"].emplace_back();
					gm["section"] = gt.sectionIndex;
					gm["probability"] = gt.probability;
				}
			}

			for ( const auto& sec : song.sections ) {
				auto& secm = sm["sections"].emplace_back();
				secm["id"] = sec.id;
				secm["volume"] = sec.volume;
				secm["loop_count"] = sec.loopCount;

				for ( const auto& samp : sec.samples ) {
					secm["samples"].emplace_back(samp.name);
				}

				for ( const auto& evt : sec.events ) {
					auto& em = secm["events"].emplace_back();
					em["event"] = evt.eventString;
					em["flags"] = evt.flags;
					for ( const auto& gt : evt.gotos ) {
						auto& gm = em["gotos"].emplace_back();
						gm["section"] = gt.sectionIndex;
						gm["probability"] = gt.probability;
					}
				}
			}
		}
	}

	uf::stl::unordered_map<int32_t, uf::stl::string> parseNameMap(const uf::stl::vector<uint8_t>& buffer, uint32_t& offset) {
		uf::stl::unordered_map<int32_t, uf::stl::string> map;
		int32_t upperBound, lowerBound, size;

		if (!impl::readStruct(buffer, offset, upperBound)) return map;
		if (!impl::readStruct(buffer, offset, lowerBound)) return map;
		if (!impl::readStruct(buffer, offset, size)) return map;

		for (int i = 0; i < size; ++i) {
			char flag;
			if (!impl::readStruct(buffer, offset, flag)) break;

			if (flag == '+') {
				uf::stl::vector<char> text;
				if (!impl::readArray(buffer, offset, 16, text)) break;
				map[i + lowerBound] = uf::stl::string(text.data(), strnlen(text.data(), 16));
			}
		}
		return map;
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
		impl::parseProperty( ctx, "TransDoor" );
		impl::parseProperty( ctx, "RotDoor" );
		impl::parseProperty( ctx, "Ambient" );
		impl::parseProperty( ctx, "AmbientHacked" );
		impl::parseProperty( ctx, "Scripts" );
		//	 physics
		impl::parseProperty( ctx, "PhysType" );
		impl::parseProperty( ctx, "PhysAttr" );
		impl::parseProperty( ctx, "PhysDims" );
		impl::parseProperty( ctx, "PhysState" );
		//	schemas
		impl::parseProperty( ctx, "SchPlayParams" );
		impl::parseProperty( ctx, "SchLoopParams" );

		impl::parseProperty( ctx, "Class Tag" );
		impl::parseProperty( ctx, "SchMsg" );
		impl::parseProperty( ctx, "SchAction" );

		// parse schsamp
		if ( inventory.count("SchSamp") > 0 ) {
			impl::parseSchSamp( ctx, inventory["SchSamp"] );
		}
		impl::parseSchemas( ctx );

		// parse links
		if ( inventory.count("Relations") > 0 ) {
			impl::parseRelations( ctx, inventory["Relations"] );
		}
		for ( const auto& [ name, item ] : inventory ) {
			if ( name.starts_with("L$") && !name.starts_with("LD$")) {
				impl::parsePartitionedLinks( ctx, item );
			}
		}

		// parse tags
		if ( inventory.count("Speech_DB") > 0 ) {
			const auto& item = inventory["Speech_DB"];
			uint32_t offset = item.offset + sizeof(impl::DarkDBChunkHeader);
			auto& buffer = ctx.buffer;

			auto concepts = parseNameMap(buffer, offset);
			auto tags = parseNameMap(buffer, offset);
			auto values = parseNameMap(buffer, offset);

			if ( inventory.count("ENV_SOUND") > 0 ) {
				const auto& item = inventory["ENV_SOUND"];
				uint32_t offset = item.offset + sizeof(impl::DarkDBChunkHeader);

				int32_t numRequiredTags;
				if ( impl::readStruct(buffer, offset, numRequiredTags) ) {
					offset += numRequiredTags;

					parseEnvSoundTree(buffer, offset, tags, values, "", ctx);
				}
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
	auto famMount = uf::vfs::mount(ext::zlib::createZipMount("FAM://", "game://Data/res/FAM.CRF", 1000), true);
	auto mshMount = uf::vfs::mount(ext::zlib::createZipMount("OBJ://", "game://Data/res/MESH.CRF", 1000), true);
	auto bmpMount = uf::vfs::mount(ext::zlib::createZipMount("OBJ://", "game://Data/res/BITMAP.CRF", 1000), true);
	auto objMount = uf::vfs::mount(ext::zlib::createZipMount("OBJ://", "game://Data/res/OBJ.CRF", 1000), true);

	auto sndMount = uf::vfs::mount(ext::zlib::createZipMount("SND://", "game://Data/res/SND.CRF", 1000)); // not temp
	auto snd2Mount = uf::vfs::mount(ext::zlib::createZipMount("SND://", "game://Data/res/SND2.CRF", 1000)); // not temp
	auto songMount = uf::vfs::mount(ext::zlib::createZipMount("SND://", "game://Data/res/SONG.CRF", 1000)); // not temp

	//
	{
		auto wavs = uf::vfs::list( "SND://", ".wav", true );
		uf::stl::vector<uf::stl::string> imgExts = { ".png", ".dds", ".tga", ".pcx", ".gif", ".bmp" };
		uf::stl::vector<uf::stl::string> texMounts = { "FAM://", "OBJ://" };

		for ( const auto& path : wavs ) {
			uf::stl::string filename = path;

			size_t slashPos = filename.find_last_of('/');
			if (slashPos != uf::stl::string::npos) filename = filename.substr(slashPos + 1);

			std::transform(filename.begin(), filename.end(), filename.begin(), ::tolower);

			uf::stl::string absolutePath = path;
			if ( !absolutePath.starts_with("SND://") && !absolutePath.starts_with("snd://") ) {
				if ( absolutePath.starts_with("/") ) absolutePath = "SND:/" + absolutePath;
				else absolutePath = "SND://" + absolutePath;
			}

			ctx.fileDatabase[filename] = absolutePath;
		}


		for ( const auto& mount : texMounts ) {
			for ( const auto& ext : imgExts ) {
				auto files = uf::vfs::list(mount, ext, true);
				for ( const auto& path : files ) {
					uf::stl::string lowerPath = path;
					std::transform(lowerPath.begin(), lowerPath.end(), lowerPath.begin(), ::tolower);

					uf::stl::string filename = lowerPath;
					size_t slashPos = filename.find_last_of('/');
					if (slashPos != uf::stl::string::npos) filename = filename.substr(slashPos + 1);
					uf::stl::string baseName = filename.substr(0, filename.find_last_of('.'));

					if (ctx.fileDatabase.count(baseName) == 0) {
						ctx.fileDatabase[baseName] = path;
					}

					if (lowerPath.starts_with("fam://")) {
						size_t famEnd = lowerPath.find('/', 6);
						if (famEnd != uf::stl::string::npos) {
							uf::stl::string family = lowerPath.substr(6, famEnd - 6);
							ctx.fileDatabase[family + "/" + baseName] = path;
						}
					}
				}
			}
		}
	}

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
	impl::processSongs( graph, ctx );

	// needs a home
	auto& schemaDbMeta = graph.metadata["dark"]["schema_db"];
	for ( const auto& [id, schema] : ctx.schemas ) {
		if ( schema.wavs.empty() ) continue;

		auto& entry = schemaDbMeta.emplace_back();
		entry["name"] = schema.name;
		entry["tags"] = schema.classTag;
		entry["msg"]  = schema.msg;
		entry["action"] = schema.action;

		for (const auto& wav : schema.wavs) {
			entry["wavs"].emplace_back(wav.name);
		}
	}

	// disable postprocessing flags
	if ( filename.starts_with("game://") ) graph.metadata["exporter"]["enabled"] = false; // disable exporting if loaded from a VPK
	graph.metadata["exporter"]["unwrap"] = false; // do not unwrap UVs for baking (we already have those)
	graph.metadata["baking"]["enabled"] = false; // disable lightmap baking (we already have those)

	uf::graph::postprocess(graph);
}