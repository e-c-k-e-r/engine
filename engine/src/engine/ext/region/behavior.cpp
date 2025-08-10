#include "behavior.h"
#include "./chunk/behavior.h"

#include <uf/utils/hook/hook.h>
#include <uf/utils/time/time.h>
#include <uf/utils/serialize/serializer.h>
#include <uf/utils/userdata/userdata.h>
#include <uf/utils/window/window.h>
#include <uf/utils/window/payloads.h>
#include <uf/utils/camera/camera.h>
#include <uf/utils/audio/audio.h>
#include <uf/ext/openvr/openvr.h>
#include <uf/engine/graph/graph.h>
#include <uf/engine/asset/asset.h>
#include <uf/utils/math/physics.h>
#include <uf/spec/controller/controller.h>
#include <uf/utils/graphic/graphic.h>
#include <uf/utils/io/inputs.h>

#include <sstream>


UF_BEHAVIOR_REGISTER_CPP(ext::RegionBehavior)
UF_BEHAVIOR_TRAITS_CPP(ext::RegionBehavior, ticks = true, renders = false, multithread = false)
#define this (&self)

namespace {
	pod::Vector3i getIndex( const pod::Vector3f& position, const pod::Vector3ui& size ) {
		pod::Vector3f index = { position.x / size.x, position.y / size.y, position.z / size.z };
	//	index.x += index.x > 0 ? 0.5f : -0.5f;
	//	index.y += index.y > 0 ? 0.5f : -0.5f;
	//	index.z += index.z > 0 ? 0.5f : -0.5f;
		return { index.x, index.y, index.z };
	}
	bool outOfRange( const pod::Vector3f& a, const pod::Vector3f& b, const pod::Vector3f& range ) {
		auto distance = uf::vector::distance( a, b );
		auto length = uf::vector::norm( range );

		UF_MSG_DEBUG("{}\t\t||{} - {}|| > ||{}|| // ({} : {})",
			distance > length,
			uf::vector::toString(a),
			uf::vector::toString(b),
			uf::vector::toString(range),
			distance,
			length
		);

		return distance > length;
	/*
		float dot = uf::vector::dot( a, b );
		float mag = uf::vector::magnitude( range );
		return dot > mag;
	*/
	}


	void generateChunk( uf::Object& self, pod::Vector3i index ) {
		auto& metadata = this->getComponent<ext::RegionBehavior::Metadata>();
		if ( metadata.chunks.count( index ) > 0 ) return;

		auto& child = this->loadChild("/region-chunk.json", false);
		auto& serializer = child.getComponent<uf::Serializer>();
		auto& chunkMetadata = child.getComponent<ext::RegionChunkBehavior::Metadata>();
		auto& chunkTransform = child.getComponent<pod::Transform<>>();

		metadata.chunks[index] = &child;
		serializer["region"]["index"] = uf::vector::encode( index );
		chunkTransform.position = index * metadata.settings.chunkSize;
		child.initialize();

		UF_MSG_DEBUG("Generated chunk: {} ({})", uf::vector::toString( index ), child.getUid());
	}
	void generateChunks( uf::Object& self ) {
		auto& metadata = this->getComponent<ext::RegionBehavior::Metadata>();

		if ( metadata.last.checkTime >= 0 && metadata.last.checkTime >= uf::physics::time::current ) return;

		auto& scene = uf::scene::getCurrentScene();
		auto& controller = scene.getController();
		auto& controllerTransform = controller.getComponent<pod::Transform<>>();

		pod::Vector3i controllerIndex = getIndex( controllerTransform.position, metadata.settings.chunkSize );
		pod::Vector3i halfRange = {
			metadata.settings.chunkRange.x / 2,
			metadata.settings.chunkRange.y / 2,
			metadata.settings.chunkRange.z / 2,
		};


		for ( int y = 0; y < metadata.settings.chunkRange.y * 2; ++y ) {
		for ( int z = 0; z < metadata.settings.chunkRange.z * 2; ++z ) {
		for ( int x = 0; x < metadata.settings.chunkRange.x * 2; ++x ) {
		//	UF_MSG_DEBUG("{}, {}, {}", x, y, z);
			// skip loading if it would just degenerate
			pod::Vector3i index = pod::Vector3i{ x, y, z } + controllerIndex - halfRange;
			if ( outOfRange( controllerTransform.position, index * metadata.settings.chunkSize, metadata.settings.chunkSize * metadata.settings.chunkRange ) ) continue;
			generateChunk( self, index );
		}
		}
		}

		metadata.last.checkTime = uf::physics::time::current + metadata.settings.cooldown;
		metadata.last.controllerIndex = controllerIndex;
	}
	
	void degenerateChunk( uf::Object& self, const pod::Vector3i& index ) {
		auto& metadata = this->getComponent<ext::RegionBehavior::Metadata>();
		UF_MSG_DEBUG("Degenerating: {}", uf::vector::toString( index ));
		if ( metadata.chunks.count( index ) <= 0 ) return;

		auto& child = *metadata.chunks[index];
		child.queueDeletion();
		metadata.chunks.erase( index );
		
		UF_MSG_DEBUG("Degenerated chunk: {} ({})", uf::vector::toString( index ), child.getUid());
	}
	void degenerateChunks( uf::Object& self ) {
		auto& metadata = this->getComponent<ext::RegionBehavior::Metadata>();
		auto& scene = uf::scene::getCurrentScene();
		auto& controller = scene.getController();
		auto& controllerTransform = controller.getComponent<pod::Transform<>>();

		uf::stl::vector<pod::Vector3i> degenerates;

		for ( auto& pair : metadata.chunks ) {
			auto& index = pair.first;
			if ( !outOfRange( controllerTransform.position, index * metadata.settings.chunkSize, metadata.settings.chunkSize * metadata.settings.chunkRange ) ) continue;
			degenerates.emplace_back( index );
		}

		for ( auto& index : degenerates ) {
			degenerateChunk( self, index );
		}
	}
}

void ext::RegionBehavior::initialize( uf::Object& self ) {
	auto& metadata = this->getComponent<ext::RegionBehavior::Metadata>();
	auto& metadataJson = this->getComponent<uf::Serializer>();

	UF_BEHAVIOR_METADATA_BIND_SERIALIZER_HOOKS(metadata, metadataJson);

	auto& transform = this->getComponent<pod::Transform<>>();

	// generate chunks
	this->addHook( "region:GenerateChunks.%UID%", [&](){
		UF_MSG_DEBUG("GENERATE");
		generateChunks( self );
	});
	this->addHook( "region:DegenerateChunks.%UID%", [&](){
		UF_MSG_DEBUG("DEGENERATE");
		degenerateChunks( self );
	});
	this->addHook( "region:Update.%UID%", [&](){
		generateChunks( self );
		degenerateChunks( self );
	});

	this->addHook( "controller:Ready", [&](ext::json::Value& payload){
		metadata.ready = true;
		UF_MSG_DEBUG("READY");
		this->queueHook( "region:GenerateChunks.%UID%" );
	});
}
void ext::RegionBehavior::tick( uf::Object& self ) {
	auto& metadata = this->getComponent<ext::RegionBehavior::Metadata>();
	auto& metadataJson = this->getComponent<uf::Serializer>();
	
	if ( !metadata.ready ) return;

	auto& scene = uf::scene::getCurrentScene();
	auto& controller = scene.getController();
	auto& controllerTransform = controller.getComponent<pod::Transform<>>();

	pod::Vector3i controllerIndex = getIndex( controllerTransform.position, metadata.settings.chunkSize );

	// controller moved indices, regenerate
	if ( controllerIndex != metadata.last.controllerIndex || metadata.chunks.size() == 0 ) {
		if ( metadata.last.checkTime >= 0 && metadata.last.checkTime >= uf::physics::time::current ) return;
		degenerateChunks( self );
		generateChunks( self );
		metadata.last.checkTime = uf::physics::time::current + metadata.settings.cooldown;
	}
}
void ext::RegionBehavior::render( uf::Object& self ){}
void ext::RegionBehavior::destroy( uf::Object& self ){
	auto& metadata = this->getComponent<ext::RegionBehavior::Metadata>();
	for ( auto& pair : metadata.chunks ) {
		degenerateChunk( self, pair.first );
	}
	metadata.chunks.clear();
}
void ext::RegionBehavior::Metadata::serialize( uf::Object& self, uf::Serializer& serializer ){
	serializer["region"]["range"] = uf::vector::encode( /*this->*/settings.chunkRange );
	serializer["region"]["size"] = uf::vector::encode( /*this->*/settings.chunkSize );
	serializer["region"]["cooldown"] = /*this->*/settings.cooldown;

}
void ext::RegionBehavior::Metadata::deserialize( uf::Object& self, uf::Serializer& serializer ){
	/*this->*/settings.chunkRange = uf::vector::decode( serializer["region"]["range"], /*this->*/settings.chunkRange );
	/*this->*/settings.chunkSize = uf::vector::decode( serializer["region"]["size"], /*this->*/settings.chunkSize );
	/*this->*/settings.cooldown = serializer["region"]["cooldown"].as( /*this->*/settings.cooldown );
}
#undef this