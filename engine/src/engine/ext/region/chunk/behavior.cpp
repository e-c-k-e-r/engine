#include "behavior.h"
#include "../behavior.h"

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


UF_BEHAVIOR_REGISTER_CPP(ext::RegionChunkBehavior)
UF_BEHAVIOR_TRAITS_CPP(ext::RegionChunkBehavior, ticks = true, renders = false, multithread = false)
#define this (&self)
namespace {
	void load( uf::Object& self, const uf::Image& image ) {
		auto& metadata = this->getComponent<ext::RegionChunkBehavior::Metadata>();
		auto& metadataJson = this->getComponent<uf::Serializer>();

		auto& mesh = this->getComponent<uf::Mesh>();
		auto& collider = this->getComponent<pod::PhysicsState>();
		
		auto& graphic = this->getComponent<uf::Graphic>();
		auto& texture = graphic.material.textures.emplace_back();

		auto& region = this->getParent();
		auto& regionMetadata = region.getComponent<ext::RegionBehavior::Metadata>();
		
		// logic to load from disk here // logic to generate terrain here

		{
			texture.loadFromImage( image );
		}
		{
			mesh.bindIndirect<pod::DrawCommand>();
			mesh.bind<uf::graph::mesh::Base, uint16_t>();
			
			pod::Vector3f scale = {
				regionMetadata.settings.chunkSize.x / 2,
				regionMetadata.settings.chunkSize.y / 2,
				regionMetadata.settings.chunkSize.z / 2,
			};

			mesh.insertVertices<uf::graph::mesh::Base>({
				{ scale * pod::Vector3f{ -1.0f, 0.0f, -1.0f}, {0.0f, 0.0f}, { 255, 255, 255, 255 }, {0.0f, 0.0f}, {0.0f, 1.0f, 0.0f} },
				{ scale * pod::Vector3f{ -1.0f, 0.0f, 1.0f}, {0.0f, 1.0f}, { 255, 255, 255, 255 }, {0.0f, 1.0f}, {0.0f, 1.0f, 0.0f} },
				{ scale * pod::Vector3f{ 1.0f, 0.0f, 1.0f}, {1.0f, 1.0f}, { 255, 255, 255, 255 }, {1.0f, 1.0f}, {0.0f, 1.0f, 0.0f} },
				{ scale * pod::Vector3f{ 1.0f, 0.0f, -1.0f}, {1.0f, 0.0f}, { 255, 255, 255, 255 }, {0.0f, 1.0f}, {0.0f, 1.0f, 0.0f} },
			});
			mesh.insertIndices<uint16_t>({
				0, 1, 2, 2, 3, 0
			});
		}
		
		{
			uf::graph::convert( self );
		}

		{
			auto& phyziks = metadataJson["physics"];

			collider.stats.mass = phyziks["mass"].as(collider.stats.mass);
			collider.stats.friction = phyziks["friction"].as(collider.stats.friction);
			collider.stats.restitution = phyziks["restitution"].as(collider.stats.restitution);
			collider.stats.inertia = uf::vector::decode( phyziks["inertia"], collider.stats.inertia );
			collider.stats.gravity = uf::vector::decode( phyziks["gravity"], collider.stats.gravity );

			uf::physics::impl::create( self, mesh, !phyziks["static"].as<bool>(true) );

		}
	}
}
void ext::RegionChunkBehavior::initialize( uf::Object& self ) {
	auto& metadata = this->getComponent<ext::RegionChunkBehavior::Metadata>();
	auto& metadataJson = this->getComponent<uf::Serializer>();
	auto& transform = this->getComponent<pod::Transform<>>();
	
	this->addHook( "asset:Load.%UID%", [&](pod::payloads::assetLoad& payload){
		if ( !uf::asset::isExpected( payload, uf::asset::Type::IMAGE ) ) return;
		if ( !uf::asset::has( payload ) ) uf::asset::load( payload );
		const auto& image = uf::asset::get<uf::Image>( payload );

		UF_MSG_DEBUG("LOADING: {} // {}", payload.filename, uf::string::toString(transform.position));

		::load( self, image );
	});

	// probably add logic to ensure the position is properly set here

	UF_BEHAVIOR_METADATA_BIND_SERIALIZER_HOOKS(metadata, metadataJson);
}
void ext::RegionChunkBehavior::tick( uf::Object& self ) {}
void ext::RegionChunkBehavior::render( uf::Object& self ){}
void ext::RegionChunkBehavior::destroy( uf::Object& self ){}
void ext::RegionChunkBehavior::Metadata::serialize( uf::Object& self, uf::Serializer& serializer ){
	serializer["region"]["index"] = uf::vector::encode( /*this->*/index );
}
void ext::RegionChunkBehavior::Metadata::deserialize( uf::Object& self, uf::Serializer& serializer ){
	/*this->*/index = uf::vector::decode( serializer["region"]["index"], /*this->*/index );
}
#undef this