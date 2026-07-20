#include <uf/ext/lgs/common.h>
#include <uf/ext/lgs/mis.h>
#include <uf/ext/lgs/bin.h>
#include <uf/ext/lgs/pcx.h>

#include <uf/ext/valve/common.h>
#include <uf/ext/zlib/zlib.h>
#include <uf/utils/mesh/grid.h>
#include <uf/utils/memory/unordered_set.h>
#include <uf/utils/memory/reader.h>


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

	struct DarkDBChunkHeader {
		char name[12];
		uint32_t version_high;
		uint32_t version_low;
		uint32_t zero;

	};

	struct DarkDBInvItem {
		char name[12];
		uint32_t offset;
		uint32_t length;
		
		inline uint32_t start() const { return offset + sizeof(impl::DarkDBChunkHeader); }
		inline uint32_t len() const { return length >= sizeof(impl::DarkDBChunkHeader) ? length - sizeof(impl::DarkDBChunkHeader) : 0; }
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

	struct PropertyFrobInfo {
		uint32_t world_action;
		uint32_t inv_action;
		uint32_t tool_action;
		uint32_t pad;
	};

	struct PropertyPhysType {
		enum int32_t {
			OBB	  		= 0,
			SPHERE   	= 1,
			SPHERE_HAT	= 2,
			NONE	 	= 3,
		};
		int32_t type;
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


	constexpr uint8_t SCHEMA_LOOP_POLY	= 0x01;
	constexpr uint8_t SCHEMA_LOOP_COUNT = 0x02;

	struct PropertySchPlayParams {
		enum uint32_t {
			SCH_PLAY_RETRIGGER 	= (1 << 0),
			SCH_PAN_POS 		= (1 << 1),
			SCH_PAN_RANGE	  	= (1 << 2),
			SCH_NO_REPEAT	  	= (1 << 3),
			SCH_NO_CACHE	   	= (1 << 4),
			SCH_STREAM		 	= (1 << 5),
			SCH_PLAY_ONCE	  	= (1 << 6),
			SCH_NO_COMBAT	  	= (1 << 7),
			SCH_NET_AMBIENT		= (1 << 8),
			SCH_LOC_SPATIAL		= (1 << 9),
		};
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

	struct DarkRoomPortal {
		int32_t portalId;
		int32_t index;
		impl::WRPlane portalPlane;
		uf::stl::vector<impl::WRPlane> edgePlanes;
		int32_t farRoomId;
		int32_t nearRoomId;
		pod::Vector3f center;
		int32_t farPortalId;
	};

	struct DarkRoom {
		int32_t objId;
		int16_t roomId;
		pod::Vector3f center;
		impl::WRPlane planes[6];

		uf::stl::vector<DarkRoomPortal> portals;
		uf::stl::vector<uf::stl::vector<float>> portalDists;
		uf::stl::vector<uf::stl::vector<int32_t>> watchList;

		pod::Vector3f getSize() const {
			pod::Vector3f size;
			size.x = -(uf::vector::dot(planes[0].normal, center) + planes[0].d);
			size.y = -(uf::vector::dot(planes[1].normal, center) + planes[1].d);
			size.z = -(uf::vector::dot(planes[2].normal, center) + planes[2].d);
			return size;
		}

		bool contains( const pod::Vector3f& pt ) const {
			for (int i = 0; i < 6; ++i) {
				if ( uf::vector::dot(planes[i].normal, pt) + planes[i].d > 0.001f ) {
					return false;
				}
			}
			return true;
		}
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
			uf::stl::unordered_map<int32_t, PropertyFrobInfo> frobInfo;
			uf::stl::unordered_map<int32_t, PropertyPhysType> physType;
			uf::stl::unordered_map<int32_t, PropertyPhysAttr> physAttr;
			uf::stl::unordered_map<int32_t, PropertyPhysDims> physDims;
			uf::stl::unordered_map<int32_t, PropertyPhysState> physState;
			uf::stl::unordered_map<int32_t, PropertyAmbient> ambient;
			uf::stl::unordered_map<int32_t, PropertySchPlayParams> schPlayParams;
			uf::stl::unordered_map<int32_t, PropertySchLoopParams> schLoopParams;
			uf::stl::unordered_map<int32_t, uf::stl::vector<PropertySchSamp>> schSamps;
			uf::stl::unordered_map<int32_t, uf::stl::string> objSoundName;
			uf::stl::unordered_map<int32_t, uf::stl::string> classTag;
			uf::stl::unordered_map<int32_t, uf::stl::string> schMsg;
			uf::stl::unordered_map<int32_t, uf::stl::string> schAction;
			
			uf::stl::unordered_map<int32_t, uf::stl::vector<uf::stl::string>> script;
		} properties;

		uf::stl::unordered_map<uf::stl::string, uf::stl::string> fileDatabase;

		uf::stl::unordered_map<int32_t, DarkSchema> schemas;
		uf::stl::vector<DarkLinkData> links;
		uf::stl::unordered_map<uint16_t, uf::stl::string> linkFlavorNames;

		uf::stl::unordered_map<int16_t, size_t> roomNodes;
		uf::stl::vector<int32_t> roomEAX;
		uf::stl::vector<DarkRoom> rooms;

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

	template<typename T>
	void extractProperty( impl::DarkContext& ctx, const uf::stl::string& name, uf::stl::unordered_map<int32_t, T>& map, size_t prefixSkip = 0 ) {
		auto invName = ::fmt::format("P${}", name).substr(0, 11);
		if ( ctx.inventory.count(invName) == 0 ) return;

		const auto& item = ctx.inventory.at( invName );
		uf::stl::reader reader( ctx.buffer, item.start(), item.len());

		while ( !reader.eof() ) {
			const auto* entry = reader.read<PropertyEntry>();
			if ( !entry ) break;

			if ( entry->size <= 0 ) {
				reader.skip( entry->size );
				continue;
			}

			T copy{};
			size_t len = std::min<size_t>(entry->size, sizeof(T));
			const void* data = reader.read< char >(len);
			if ( data ) {
				std::memcpy(&copy, data, len);
				map[entry->objectId] = copy;
			}

			if ( entry->size > len ) {
				reader.skip( entry->size - len );
			}
		}
	}

	template<>
	void extractProperty( impl::DarkContext& ctx, const uf::stl::string& propName, uf::stl::unordered_map<int32_t, uf::stl::string>& map, size_t prefixSkip ) {
		auto fullName = ::fmt::format("P${}", propName).substr(0, 11);
		if (ctx.inventory.count(fullName) == 0) return;

		const auto& item = ctx.inventory.at(fullName);

		uf::stl::reader reader( ctx.buffer, item.start(), item.len());

		while ( !reader.eof() ) {
			const auto* entry = reader.read<PropertyEntry>();
			if ( !entry ) break;

			if ( entry->objectId == 0 || entry->size <= prefixSkip ) {
				reader.skip(entry->size);
				continue;
			}
			reader.skip( prefixSkip );
			size_t len = entry->size - prefixSkip;
			const char* data = reader.read< char >(len);
			if ( data ) map[entry->objectId] = uf::stl::string(data, strnlen(data, len));
		}
	}

	template<>
	void extractProperty( impl::DarkContext& ctx, const uf::stl::string& propName, uf::stl::unordered_map<int32_t, uf::stl::vector<uf::stl::string>>& map, size_t prefixSkip ) {
		auto fullName = ::fmt::format("P${}", propName).substr(0, 11);
		if (ctx.inventory.count(fullName) == 0) return;

		const auto& item = ctx.inventory.at(fullName);
		uf::stl::reader reader( ctx.buffer, item.start(), item.len());

		while ( !reader.eof() ) {
			const auto* entry = reader.read<PropertyEntry>();
			if ( !entry ) break;

			if ( entry->objectId == 0 || entry->size <= prefixSkip ) {
				reader.skip(entry->size);
				continue;
			}
			reader.skip( prefixSkip );
			size_t len = entry->size - prefixSkip;
			const char* data = reader.read< char >(len);

			if ( data ) {
				const char* cursor = data;
				size_t remainingBytes = len;

				for ( auto s = 0; s < 4; ++s ) {
					if ( remainingBytes <= 0 || *cursor == '\0' ) break;

					auto script = uf::stl::string( cursor );
					if ( script.empty() ) break;

					map[entry->objectId].emplace_back(script);

					size_t step = strnlen(cursor, remainingBytes) + 1;
					if ( step > remainingBytes ) break;

					cursor += step;
					remainingBytes -= step;
				}
			}
		}
	}

	void parseRelations( impl::DarkContext& ctx, const impl::DarkDBInvItem& item ) {
		uf::stl::reader reader(ctx.buffer, item.start(), item.len());

		uint16_t flavorId = 1;
		while ( reader.remaining() >= 32 ) {
			const char* relNameData = reader.read< char >(32);
			if ( relNameData ) {
				uf::stl::string relName(relNameData, strnlen(relNameData, 32));
				if ( !relName.empty() ) ctx.linkFlavorNames[flavorId] = relName;
			}
			flavorId++;
		}
	}

	void parsePartitionedLinks( impl::DarkContext& ctx, const impl::DarkDBInvItem& item ) {
		uf::stl::reader reader(ctx.buffer, item.start(), item.len());

		size_t count = item.len() / sizeof(impl::DarkPartitionedLink);
		uf::stl::vector<impl::DarkPartitionedLink> chunkLinks;

		if ( reader.read(count, chunkLinks) ) {
			for ( const auto& l : chunkLinks ) {
				ctx.links.push_back({l.src, l.dest, l.flavor});
			}
		}
	}

	bool parseRoom(uf::stl::reader& reader, DarkRoom& outRoom) {
		const auto* pObjId = reader.read<int32_t>();
		const auto* pRoomId = reader.read<int16_t>();
		// reader.skip(2);
		const auto* pCenter = reader.read<pod::Vector3f>();
		if ( !pObjId || !pRoomId || !pCenter ) return false;

		outRoom.objId = *pObjId;
		outRoom.roomId = *pRoomId;
		outRoom.center = *pCenter;

		for ( int i = 0; i < 6; ++i ) {
			const auto* pPlane = reader.read<impl::WRPlane>();
			if (!pPlane) return false;
			outRoom.planes[i] = *pPlane;
		}

		const auto* numPortals = reader.read<int32_t>();
		if (!numPortals) return false;

		outRoom.portals.resize(*numPortals);
		for ( int i = 0; i < *numPortals; ++i ) {
			auto& portal = outRoom.portals[i];

			portal.portalId = *reader.read<int32_t>();
			portal.index = *reader.read<int32_t>();
			portal.portalPlane = *reader.read<impl::WRPlane>();

			int32_t numEdges = *reader.read<int32_t>();
			reader.read(numEdges, portal.edgePlanes);

			portal.farRoomId = *reader.read<int32_t>();
			portal.nearRoomId = *reader.read<int32_t>();
			portal.center = *reader.read<pod::Vector3f>();
			portal.farPortalId = *reader.read<int32_t>();
		}

		outRoom.portalDists.resize(*numPortals);
		for ( int i = 0; i < *numPortals; ++i ) {
			reader.read(*numPortals, outRoom.portalDists[i]);
		}

		const auto* numWatches = reader.read<int32_t>();
		if ( !numWatches ) return false;

		outRoom.watchList.resize(*numWatches);
		for ( int i = 0; i < *numWatches; ++i ) {
			int32_t watchSize = *reader.read<int32_t>();
			reader.read(watchSize, outRoom.watchList[i]);
		}

		return true;
	}

	void parseRoomEAX( impl::DarkContext& ctx, const impl::DarkDBInvItem& item ) {
		uf::stl::reader reader(ctx.buffer, item.start(), item.len());

		const auto* versionMajor = reader.read<int32_t>();
		const auto* versionMinor = reader.read<int32_t>();

		const auto* size = reader.read<int32_t>();
		if ( !size || *size <= 0 || *size > 10000 ) {
			UF_MSG_ERROR("Invalid EAX size");
			return;
		}

		reader.read( *size, ctx.roomEAX );
	}

	void parseRooms( impl::DarkContext& ctx, const impl::DarkDBInvItem& item ) {
		uf::stl::reader reader(ctx.buffer, item.start(), item.len());

		const auto* roomsOK = reader.read<int32_t>();
		if ( !roomsOK || *roomsOK == 0 ) return;

		const auto* numRooms = reader.read<int32_t>();
		if ( !numRooms ) return;

		for ( int32_t i = 0; i < *numRooms; ++i ) {
			impl::DarkRoom room;
			if ( impl::parseRoom( reader, room ) ) {
				ctx.rooms.push_back( room );
			} else {
				if ( i == *numRooms - 1 ) {
					//UF_MSG_DEBUG("Room {} truncated at EOF", i);
				} else {
					UF_MSG_WARNING("Failed to parse room {} of {}", i, *numRooms);
				}
				break;
			}
		}
	}

	void processRooms( pod::Graph& graph, impl::DarkContext& ctx ) {
		for ( const auto& room : ctx.rooms ) {
			auto nodeID = graph.nodes.size();
			graph.root.children.emplace_back(nodeID);
			auto& node = graph.nodes.emplace_back();
			ctx.roomNodes[room.roomId] = nodeID;

			node.name = ::fmt::format("room_{}", room.roomId);

			node.transform.position = impl::convertPos_NewDark( room.center );

			pod::Vector3f localExtents = room.getSize();

			auto& metadata = node.metadata["dark"];
			metadata["id"] = room.objId;
			metadata["room_id"] = room.roomId;
			metadata["is_room"] = true;

			auto& physMeta = node.metadata["physics"];
			physMeta["type"] = "obb";
			physMeta["category"] = "trigger";
			physMeta["inertia"] = false;
			physMeta["mass"] = 0.0f;

			// extents seem wrong......
			pod::Vector3f worldExtents = impl::convertPos_NewDark(localExtents);
			physMeta["extent"] = uf::vector::encode( uf::vector::abs(worldExtents) );

			if ( room.roomId >= 0 && room.roomId < ctx.roomEAX.size() ) {
				int32_t eaxType = ctx.roomEAX[room.roomId];
				metadata["sound"]["eax_type"] = eaxType;
			}
		}
	}

	void parseEnvSoundTree(
		uf::stl::reader& reader,
		const uf::stl::unordered_map<int32_t, uf::stl::string>& tagMap,
		const uf::stl::unordered_map<int32_t, uf::stl::string>& valueMap,
		uf::stl::string currentTags,
		impl::DarkContext& ctx
	) {
		const auto* dataSize = reader.read<int32_t>();
		if (!dataSize) return;

		for (int i = 0; i < *dataSize; ++i) {
			const auto* data = reader.read<sTagDBData>();
			if (!data) return;
			ctx.properties.classTag[data->objId] = currentTags;
		}

		const auto* branchSize = reader.read<int32_t>();
		if (!branchSize) return;

		for (int i = 0; i < *branchSize; ++i) {
			const auto* key = reader.read<cTagDBKey>();
			if (!key) return;

			uf::stl::string newTags = currentTags;
			if (!newTags.empty()) newTags += ", ";

			uf::stl::string tagName = tagMap.count(key->type) ? tagMap.at(key->type) : ::fmt::format("UnknownTag_{}", key->type);
			uf::stl::string valueNames = "";

			bool isEnum = false;
			for (int v = 0; v < 8; ++v) {
				if (key->aEnum[v] == 255) isEnum = true;
			}

			if (isEnum || valueMap.count(key->aEnum[0])) {
				for (int v = 0; v < 8; ++v) {
					if (key->aEnum[v] != 255 && key->aEnum[v] != 0) {
						if (!valueNames.empty()) valueNames += "|";
						valueNames += valueMap.count(key->aEnum[v]) ? valueMap.at(key->aEnum[v]) : std::to_string(key->aEnum[v]);
					}
				}
				newTags += tagName + " " + valueNames;
			} else {
				newTags += tagName + " [" + std::to_string(key->minVal) + "-" + std::to_string(key->maxVal) + "]";
			}

			parseEnvSoundTree(reader, tagMap, valueMap, newTags, ctx);
		}
	}

	void parseSchSamp( impl::DarkContext& ctx, const impl::DarkDBInvItem& item ) {
		uf::stl::reader reader(ctx.buffer, item.start(), item.len());

		while ( !reader.eof() ) {
			const auto* pObjId = reader.read<int32_t>();
			if ( !pObjId ) break;
			int32_t objId = *pObjId;

			const auto* pNumSamples = reader.read<int32_t>();
			if ( !pNumSamples ) break;
			int32_t numSamples = *pNumSamples;

			for ( int32_t i = 0; i < numSamples; ++i ) {
				const auto* pStrLen = reader.read<int32_t>();
				if ( !pStrLen ) break;
				int32_t strLen = *pStrLen;

				const char* strData = reader.read< char >(strLen);
				if ( !strData ) break;

				uf::stl::string sampleName(strData, strnlen(strData, strLen));

				const auto* pFreq = reader.read<uint8_t>();
				if ( !pFreq ) break;
				uint8_t freq = *pFreq;

				ctx.properties.schSamps[objId].emplace_back(PropertySchSamp{
					.name = sampleName,
					.weight = freq,
				});
			}
		}
	}

	void parseObjVec( pod::Graph& graph, impl::DarkContext& ctx, const impl::DarkDBInvItem& item ) {
		uf::stl::reader reader(ctx.buffer, item.start(), item.len());

		const auto* header = reader.read<DarkDBObjVec_Header>();
		if ( !header || header->maxID <= 0 ) return;

		ctx.objects.assign( header->maxID, false );

		uint32_t bitArraySizeBytes = item.len() - sizeof(DarkDBObjVec_Header);
		const uint8_t* bitmap = reader.read<uint8_t>(bitArraySizeBytes);
		if ( !bitmap ) return;

		for ( int32_t id = header->minID; id < header->maxID; ++id ) {
			if ( id < 0 ) continue;

			int32_t startByte = header->minID >> 3;
			int32_t myByte = id >> 3;
			int32_t byteIndex = myByte - startByte;
			int32_t bitOffset = id & 0x07;

			if ( byteIndex >= 0 && byteIndex < bitArraySizeBytes ) {
				ctx.objects[id] = (bitmap[byteIndex] & (1 << bitOffset)) != 0;
			}
		}
	}

	void parseTextures( pod::Graph& graph, impl::DarkContext& ctx, const impl::DarkDBInvItem& item ) {
	//	uf::stl::reader reader(ctx.buffer, item.start(), item.len()); // should be item.length?
		uf::stl::reader reader(ctx.buffer, item.start(), item.length); // should be item.length?

		const auto* header = reader.read<DarkDBTXLIST_Header>();
		if ( !header ) return;

		uf::stl::vector<DarkDBTXLIST_fam> fams;
		if ( reader.read( header->fam_count, fams ) ) {
			ctx.families.resize( header->fam_count );
			for ( auto i = 0; i < header->fam_count; ++i ) {
				ctx.families[i] = uf::stl::string(fams[i].name, strnlen(fams[i].name, 16));
				std::transform(ctx.families[i].begin(), ctx.families[i].end(), ctx.families[i].begin(), ::tolower);
				std::replace(ctx.families[i].begin(), ctx.families[i].end(), '\\', '/');
			}
		}

		auto& storage = uf::graph::getStorage(graph);
		ctx.textureToMaterialId.resize( header->txt_count, -1 );

		uf::stl::vector<DarkDBTXLIST_texture> texs;
		if ( reader.read( header->txt_count, texs ) ) {
			for (uint32_t i = 0; i < texs.size() /*header->txt_count*/; ++i) {
				const auto& tex = texs[i];
				uf::stl::string texName(tex.name, strnlen(tex.name, 16));
				if (texName.empty() || texName == "null") continue;

				std::transform(texName.begin(), texName.end(), texName.begin(), ::tolower);
				std::replace(texName.begin(), texName.end(), '\\', '/');

				uf::stl::string matName = texName;
				if (tex.fam > 0 && tex.fam < ctx.families.size() && !ctx.families[tex.fam].empty()) {
					matName = ctx.families[tex.fam] + "/" + texName;
				}

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
		uf::stl::reader reader(buffer, 0, buffer.size());

		const auto* fileVersion = reader.read<uint32_t>();
		if ( !fileVersion ) return false;
		if ( *fileVersion != 1) {
			UF_MSG_WARNING("Song file version mismatch (expected 1, got {})", *fileVersion);
		}

		const auto* songInfo = reader.read<impl::sSongInfo>();
		if ( !songInfo ) return false;
		outSong.id = uf::stl::string(songInfo->id, strnlen(songInfo->id, kSONG_MaxStringLen));

		const auto* numGlobalEvents = reader.read<uint32_t>();
		if ( !numGlobalEvents ) return false;

		for ( uint32_t i = 0; i < *numGlobalEvents; ++i ) {
			const auto* evtInfo = reader.read<impl::sSongEventInfo>();
			if ( !evtInfo ) break;

			UF_MSG_DEBUG("Song Global Event: {}, flags: {}", evtInfo->eventString, evtInfo->flags);

			auto& evt = outSong.globalEvents.emplace_back();
			evt.eventString = uf::stl::string(evtInfo->eventString, strnlen(evtInfo->eventString, kSONG_MaxStringLen));
			evt.flags = evtInfo->flags;

			const auto* numGotos = reader.read<uint32_t>();
			if ( !numGotos ) break;

			for ( uint32_t j = 0; j < *numGotos; ++j ) {
				const auto* gotoInfo = reader.read<impl::sSongGotoInfo>();
				if ( !gotoInfo ) break;
				UF_MSG_DEBUG("  -> Goto section: {}, prob: {}", gotoInfo->sectionIndex, gotoInfo->probability);
				evt.gotos.push_back({ gotoInfo->sectionIndex, gotoInfo->probability });
			}
		}

		const auto* numSections = reader.read<uint32_t>();
		if ( !numSections ) return false;

		for ( uint32_t i = 0; i < *numSections; ++i ) {
			const auto* secInfo = reader.read<impl::sSongSectionInfo>();
			if ( !secInfo ) break;

			auto& sec = outSong.sections.emplace_back();
			sec.id = uf::stl::string(secInfo->id, strnlen(secInfo->id, kSONG_MaxStringLen));
			sec.volume = secInfo->volume;
			sec.loopCount = secInfo->loopCount;

			const auto* numSamples = reader.read<uint32_t>();
			if ( !numSamples ) break;

			for ( uint32_t j = 0; j < *numSamples; ++j ) {
				const auto* sampInfo = reader.read<impl::sSongSampleInfo>();
				if ( !sampInfo ) break;

				uf::stl::string sampleName = uf::stl::string(sampInfo->name, strnlen(sampInfo->name, kSONG_MaxStringLen));
				sec.samples.push_back({ sampleName });
			}

			const auto* numSecEvents = reader.read<uint32_t>();
			if ( !numSecEvents ) break;

			for ( uint32_t j = 0; j < *numSecEvents; ++j ) {
				const auto* evtInfo = reader.read<impl::sSongEventInfo>();
				if ( !evtInfo ) break;

				auto& evt = sec.events.emplace_back();
				evt.eventString = uf::stl::string(evtInfo->eventString, strnlen(evtInfo->eventString, kSONG_MaxStringLen));
				evt.flags = evtInfo->flags;

				const auto* numGotos = reader.read<uint32_t>();
				if ( !numGotos ) break;

				for ( uint32_t k = 0; k < *numGotos; ++k ) {
					const auto* gotoInfo = reader.read<impl::sSongGotoInfo>();
					if ( !gotoInfo ) break;
					evt.gotos.push_back({ gotoInfo->sectionIndex, gotoInfo->probability });
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
					if ( !family.empty() ) ext::lgs::loadPalette( family, ctx.palettes[family] );
					const uint8_t* palette = (!family.empty() && !ctx.palettes[family].empty()) ? ctx.palettes[family].data() : nullptr;
					if ( !ext::lgs::loadPcx( image, buffer, palette ) ) {
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
		uf::stl::reader reader(ctx.buffer, item.start(), item.len());

		const auto* header = reader.read<impl::WRHeader>();
		if ( !header ) return;

		auto& storage = uf::graph::getStorage(graph);
		uf::stl::string meshName = "worldspawn";
		graph.meshes.emplace_back(meshName);
		graph.primitives.emplace_back(meshName);

		auto& mesh = storage.meshes[meshName];
		auto& primitives = storage.primitives[meshName];
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
		uf::stl::vector<ParsedCell> cells( header->numCells );

		// fill cell information
		for ( uint32_t c = 0; c < cells.size(); ++c ) {
			auto& cell = cells[c];
			cell.cellIdx = c;

			const auto* cellHeader = reader.read<impl::WRCellHeader>();
			if ( !cellHeader ) break;
			cell.header = *cellHeader;

			// read cell information
			reader.read( cell.header.numVertices, cell.vertices );
			reader.read( cell.header.numPolygons, cell.polys );
			reader.read( cell.header.numTextured, cell.texInfo );

			// read index count
			const auto* numIndices = reader.read<uint32_t>();
			if ( !numIndices ) break;
			if ( reader.remaining() < *numIndices ) break;

			// fill indices
			cell.polyIndices.resize( cell.header.numPolygons );
			for ( uint8_t i = 0; i < cell.header.numPolygons; ++i ) {
				reader.read( cell.polys[i].count, cell.polyIndices[i] );
			}

			// read planes
			reader.read( cell.header.numPlanes, cell.planes );

			// read animated lights
			reader.read( cell.header.numAnimLights, cell.animLightList );

			// read lightmaps information
			reader.read( cell.header.numTextured, cell.lmInfos );

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
				uint32_t totalLmBytes = lmSizeBytes * lmCount;

				if ( lmSizeBytes > 0 && reader.remaining() >= totalLmBytes ) {
					uf::stl::vector<uint16_t> samples;
					reader.read( w * h * lmCount, samples );

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
									(float)((sample   ) & 0x1F),
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
					reader.skip( totalLmBytes );
				}
			}

			const auto* lightCount = reader.read<uint32_t>();
			if ( lightCount ) {
				reader.read( *lightCount, cell.lightIndices );
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
				auto& meshlet = meshlets[((uint64_t)(cell.cellIdx) << 32) | (uint32_t)(materialID)];
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
			uint32_t auxID = (uint32_t)(meshletKey >> 32);
			uint32_t matID = (uint32_t)(meshletKey & 0xFFFFFFFF);

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
			auto& node = graph.nodes.emplace_back();
			auto& metadata = node.metadata["dark"];

			// bind name
			uf::stl::string archetype = "Unknown";
			ctx.findInheritedProperty(objectID, ctx.archetypes, archetype);

			if ( ctx.customNames.count(objectID) ) {
				node.name = uf::stl::string(ctx.customNames.at(objectID).c_str());
			} else {
				if ( archetype != "Unknown" ) node.name = uf::stl::string(archetype.c_str());
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

			// bind parent
			size_t parentNodeID = -1;
			for ( const auto& room : ctx.rooms ) {
				if ( room.contains( position.position ) ) {
					if ( ctx.roomNodes.count(room.roomId)) {
						parentNodeID = ctx.roomNodes.at(room.roomId);
						break;
					}
				}
			}

			if ( parentNodeID != -1 ) {
				node.metadata["debug"]["parent node transforms"] = false;

				graph.nodes[parentNodeID].children.emplace_back(nodeID);
			} else {
				graph.root.children.emplace_back(nodeID);
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
					if ( ext::lgs::loadBin( graph, path ) ) {
						node.mesh = (int32_t)(meshID);
					} else {
						node.mesh = -1;
					}
				}
			}

			// bind door
			PropertyDoor doorProp;
			bool isDoor = false;
			if ( ( isDoor = ctx.findInheritedProperty( objectID, ctx.properties.rotDoor, doorProp ) ) ) {
				metadata["door"]["is_rotating"] = true;
			} else if ( ( isDoor = ctx.findInheritedProperty( objectID, ctx.properties.transDoor, doorProp ) ) ) {
				metadata["door"]["is_rotating"] = false;
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
				metadata["class_tags"] = classTags;
			}

			// bind object sound name
			uf::stl::string objSound;
			if ( ctx.findInheritedProperty( objectID, ctx.properties.objSoundName, objSound ) ) {
				metadata["sound"]["schema"] = objSound;
			}

			// bind ambient sound
			PropertyAmbient ambient;
			if ( ctx.findInheritedProperty( objectID, ctx.properties.ambient, ambient ) ) {
				uf::stl::string schemaName(ambient.schemaName, strnlen(ambient.schemaName, 16));

				metadata["sound"]["schema"] = schemaName;
				metadata["sound"]["volume"] = ambient.override_volume;
				metadata["sound"]["radius"] = ambient.radius * impl::darkToMeters;
				metadata["sound"]["flags"]  = ambient.flags;
			}

			// bind script
			uf::stl::vector<uf::stl::string> scripts;
			if ( ctx.findInheritedProperty( objectID, ctx.properties.script, scripts ) ) {
				for ( const auto& s : scripts ) {
					metadata["scripts"].emplace_back(s);
				}
			}

			// bind frobbage
			PropertyFrobInfo frob;
			bool isFrobbable = false;
			if ( ctx.findInheritedProperty(objectID, ctx.properties.frobInfo, frob) ) {
				isFrobbable = frob.world_action != 0;
				
				metadata["frob"]["world"] = frob.world_action;
				metadata["frob"]["inv"]   = frob.inv_action;
				metadata["frob"]["tool"]  = frob.tool_action;
			}

			// bind physics
			PropertyPhysType physType;
			// to-do: optimize this as it cuts my FPS by ~30% with lots of small physics objects
			if ( ctx.findInheritedProperty( objectID, ctx.properties.physType, physType ) && physType.type != impl::PropertyPhysType::NONE ) {
				auto& physMeta = node.metadata["physics"];

				if ( physType.type == impl::PropertyPhysType::OBB ) {
					physMeta["type"] = "obb";
				} else if ( physType.type == impl::PropertyPhysType::SPHERE || physType.type == impl::PropertyPhysType::SPHERE_HAT ) {
					physMeta["type"] = "sphere";
					physMeta["radius"] = 0.5f;
				} else {
					physMeta["type"] = "mesh";
				}

				PropertyPhysAttr physAttr;
				if ( ctx.findInheritedProperty( objectID, ctx.properties.physAttr, physAttr ) ) {
					physMeta["mass"] = 0.0f; // (physType.type == impl::PropertyPhysType::OBB || physAttr.edge_trigger != 0) ? 0.0f : physAttr.mass;
					physMeta["friction"] = physAttr.base_friction;
					physMeta["restitution"] = physAttr.elasticity;
					physMeta["gravity"] = uf::vector::encode( pod::Vector3f{0.0f, -0.0981f * physAttr.gravity, 0.0f} );

					if ( physAttr.edge_trigger != 0 ) {
						physMeta["category"] = "trigger";
						physMeta["inertia"] = false;
					}
				}

				PropertyPhysDims physDims;
				if ( ctx.findInheritedProperty( objectID, ctx.properties.physDims, physDims ) ) {
					if ( physType.type == impl::PropertyPhysType::OBB ) {
						pod::Vector3f center = impl::convertPos_NewDark( physDims.offset[0] );
						pod::Vector3f extent = impl::convertPos_NewDark( uf::vector::abs( physDims.size ) ) * 0.5f;

						if ( physAttr.edge_trigger != 0 ) extent += 1.0f;
						if ( extent > 0.f ) physMeta["extent"] = uf::vector::encode(extent);
						if ( !(center == 0.f) ) physMeta["center"] = uf::vector::encode(center);
					} else if ( physType.type == impl::PropertyPhysType::SPHERE || physType.type == impl::PropertyPhysType::SPHERE_HAT ) {
						pod::Vector3f offset = impl::convertPos_NewDark(physDims.offset[0]);
						if ( physDims.radius[0] > 0.f ) physMeta["radius"] = physDims.radius[0] * impl::darkToMeters;
						if ( !(offset == 0.f) ) physMeta["offset"] = uf::vector::encode(offset);
					}
				}

				PropertyPhysState physState;
				if ( ctx.findInheritedProperty( objectID, ctx.properties.physState, physState ) ) {
					pod::Vector3f vel = impl::convertPos_NewDark(physState.velocity, 1.0f);
					pod::Vector3f angVel = { physState.rot_velocity.x, physState.rot_velocity.z, physState.rot_velocity.y };

					if ( !(vel == 0.f) ) physMeta["velocity"] = uf::vector::encode(vel);
					if ( !(angVel == 0.f) ) physMeta["angularVelocity"] = uf::vector::encode(angVel);
				}
			} else if ( node.mesh != -1 && (isDoor || isFrobbable) ) {
				auto& physMeta = node.metadata["physics"];
				physMeta["type"] = "obb";
				physMeta["mass"] = 0.0f;
			}

			// fill out metadata
			{
				metadata["id"] = objectID;
				metadata["cell"] = position.cell;	
				metadata["archetype"] = archetype;
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
			if ( !objectToNode.count(link.sourceId) ) continue;

			uf::stl::string flavor = "";
			if ( ctx.linkFlavorNames.count(link.flavor) ) {
				flavor = ctx.linkFlavorNames.at(link.flavor);
			} else {
				flavor = ::fmt::format("Unknown_{}", link.flavor);
			}

			size_t sourceIdx = objectToNode[link.sourceId];
			auto& srcNode = graph.nodes[sourceIdx];

			if ( !objectToNode.count(link.destId) ) continue;

			size_t destIdx  = objectToNode[link.destId];
			auto& connection = srcNode.metadata["dark"]["connections"].emplace_back();

			connection["flavor"] = flavor;
			connection["target_node"] = graph.nodes[destIdx].name;
			connection["target_id"] = link.destId;

			auto& inConnection = graph.nodes[destIdx].metadata["dark"]["incoming_connections"].emplace_back();
			inConnection["flavor"] = flavor;
			inConnection["source_id"] = link.sourceId;
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
			uf::stl::reader reader(ctx.buffer, item.start(), item.len());

			size_t readLen = std::min<size_t>(item.len(), 32);
			const char* songData = reader.read< char >(readLen);

			if ( songData ) {
				missionSong = uf::stl::string(songData, strnlen(songData, readLen));
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
	void processSchema( pod::Graph& graph, impl::DarkContext& ctx ) {
		auto nodeID = graph.nodes.size();
		auto& node = graph.nodes.emplace_back();
		
		graph.root.children.emplace_back( nodeID );
		node.name = "Schema DB";

		auto& schemaDbMeta = node.metadata["dark"]["schema_db"];
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

			if ( schema.hasPlayParams ) {
				entry["schema_volume"] = schema.playParams.volume;
				entry["play_once"] = (schema.playParams.flags & impl::PropertySchPlayParams::SCH_PLAY_ONCE) != 0;
				entry["stream"] = (schema.playParams.flags & impl::PropertySchPlayParams::SCH_STREAM) != 0;
			}

			if ( schema.hasLoopParams ) {
				entry["interval_min"] = schema.loopParams.intervalMin;
				entry["interval_max"] = schema.loopParams.intervalMax;
			}
		}

		UF_MSG_DEBUG("Processed schema: {}", schemaDbMeta.size());
	}

	uf::stl::unordered_map<int32_t, uf::stl::string> parseNameMap( uf::stl::reader& reader ) {
		uf::stl::unordered_map<int32_t, uf::stl::string> map;

		const auto* upperBound = reader.read<int32_t>();
		const auto* lowerBound = reader.read<int32_t>();
		const auto* size = reader.read<int32_t>();

		if (!upperBound || !lowerBound || !size) return map;

		for (int i = 0; i < *size; ++i) {
			const auto* flag = reader.read< char >();
			if (!flag) break;

			if (*flag == '+') {
				uf::stl::vector< char > text;
				if (!reader.read(16, text)) break;
				map[i + *lowerBound] = uf::stl::string(text.data(), strnlen(text.data(), 16));
			}
		}
		return map;
	}

	void readInventory( impl::DarkContext& ctx, const impl::DarkDBHeader& header ) {
		auto& buffer = ctx.buffer;
		auto& inventory = ctx.inventory;

		uf::stl::reader reader(buffer, header.inv_offset, buffer.size() - header.inv_offset);

		const auto* chunkCount = reader.read<uint32_t>();
		if ( chunkCount ) {
			for ( uint32_t i = 0; i < *chunkCount; ++i ) {
				const auto* item = reader.read<impl::DarkDBInvItem>();
				if ( !item ) break;
				uf::stl::string name = item->name;
				inventory[name] = *item;
			}
		}

		impl::extractProperty( ctx, "SymName", 			ctx.archetypes, 4 );
		impl::extractProperty( ctx, "ModelName", 		ctx.modelNames );
		impl::extractProperty( ctx, "Class Tag", 		ctx.properties.classTag, 4 );
		impl::extractProperty( ctx, "SchMsg", 			ctx.properties.schMsg, 4 );
		impl::extractProperty( ctx, "SchAction", 		ctx.properties.schAction, 4 );
		impl::extractProperty( ctx, "Scripts",			ctx.properties.script );

		impl::extractProperty( ctx, "Position", 		ctx.properties.position );
		impl::extractProperty( ctx, "Light", 			ctx.properties.light );
		impl::extractProperty( ctx, "TransDoor", 		ctx.properties.transDoor );
		impl::extractProperty( ctx, "RotDoor", 			ctx.properties.rotDoor );
		impl::extractProperty( ctx, "FrobInfo",			ctx.properties.frobInfo );
		impl::extractProperty( ctx, "Ambient", 			ctx.properties.ambient );
		impl::extractProperty( ctx, "AmbientHacked", 	ctx.properties.ambient );
		impl::extractProperty( ctx, "ObjSoundN",		ctx.properties.objSoundName, 0 );
		impl::extractProperty( ctx, "PhysType", 		ctx.properties.physType );
		impl::extractProperty( ctx, "PhysAttr", 		ctx.properties.physAttr );
		impl::extractProperty( ctx, "PhysDims", 		ctx.properties.physDims );
		impl::extractProperty( ctx, "PhysState", 		ctx.properties.physState );
		impl::extractProperty( ctx, "SchPlayParams", 	ctx.properties.schPlayParams );
		impl::extractProperty( ctx, "SchLoopParams", 	ctx.properties.schLoopParams );

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
			uf::stl::reader speechReader(ctx.buffer, item.start(), item.len());

			auto concepts = impl::parseNameMap(speechReader);
			auto tags = impl::parseNameMap(speechReader);
			auto values = impl::parseNameMap(speechReader);

			if ( inventory.count("ENV_SOUND") > 0 ) {
				const auto& envItem = inventory["ENV_SOUND"];
				uf::stl::reader envReader(ctx.buffer, envItem.start(), envItem.len());

				const auto* numRequiredTags = envReader.read<int32_t>();
				if ( numRequiredTags ) {
					envReader.skip(*numRequiredTags);
					impl::parseEnvSoundTree(envReader, tags, values, "", ctx);
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

		uf::stl::reader reader(buffer, 0, buffer.size());
		const auto* header = reader.read<impl::DarkDBHeader>();
		if ( !header ) return;
		if ( header->dead_beef != 0xEFBEADDE ) {
			UF_MSG_ERROR("Invalid DB in GAM: DEADBEEF not found.");
			return;
		}

		uf::stl::unordered_map<uf::stl::string, impl::DarkDBInvItem> inventory;
		std::swap( ctx.inventory, inventory );
		std::swap( ctx.buffer, buffer );
		impl::readInventory( ctx, *header );
		std::swap( ctx.buffer, buffer );
		std::swap( ctx.inventory, inventory );
	}
}

void ext::lgs::loadMis( pod::Graph& graph, const uf::stl::string& filename, const uf::Serializer& metadata ) {
	impl::DarkContext ctx;
	auto& buffer = ctx.buffer;
	if ( !uf::io::readAsBuffer( buffer, filename ) ) {
		UF_MSG_ERROR("Failed to read MIS data: {}", filename);
		return;
	}

	uf::stl::reader reader(buffer, 0, buffer.size());
	const auto* header = reader.read<impl::DarkDBHeader>();
	if ( !header ) {
		UF_MSG_ERROR("Failed to read DB header: {}", filename);
		return;
	}
	if ( header->dead_beef != 0xEFBEADDE ) {
		UF_MSG_ERROR("Invalid DB: DEADBEEF not found: {}", filename);
		return;
	}
	if ( header->inv_offset >= buffer.size() ) {
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
	impl::readInventory( ctx, *header );
	// mission parsing
	if ( ctx.inventory.count("TXLIST") > 0 ) {
		impl::parseTextures( graph, ctx, ctx.inventory["TXLIST"] );
		impl::loadMaterials( graph, ctx );
	}
	if ( ctx.inventory.count("WRRGB") > 0 ) {
		impl::parseWorld( graph, ctx, ctx.inventory["WRRGB"] );
	}
	if ( ctx.inventory.count("ROOM_DB") > 0 ) {
		impl::parseRooms( ctx, ctx.inventory["ROOM_DB"] );
	}
	if ( ctx.inventory.count("ROOM_EAX") > 0 ) {
		impl::parseRoomEAX( ctx, ctx.inventory["ROOM_EAX"] );
	}
	if ( ctx.inventory.count("ObjVec") > 0 ) {
		impl::parseObjVec( graph, ctx, ctx.inventory["ObjVec"] );
		impl::processRooms( graph, ctx );
		impl::loadObjects( graph, ctx );
		impl::loadMaterials( graph, ctx );
	}
	impl::processLinks( graph, ctx );
	impl::processSongs( graph, ctx );
	impl::processSchema( graph, ctx );

	// disable postprocessing flags
	if ( filename.starts_with("game://") ) graph.metadata["exporter"]["enabled"] = false; // disable exporting if loaded from a VPK
	graph.metadata["exporter"]["unwrap"] = false; // do not unwrap UVs for baking (we already have those)
	graph.metadata["baking"]["enabled"] = false; // disable lightmap baking (we already have those)
	graph.metadata["debug"]["parent node transforms"] = false; // disable parenting node transforms

	uf::graph::postprocess(graph);
}