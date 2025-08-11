#include "behavior.h"

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


UF_BEHAVIOR_REGISTER_CPP(ext::CraetureBehavior)
UF_BEHAVIOR_TRAITS_CPP(ext::CraetureBehavior, ticks = true, renders = false, multithread = false)
#define this (&self)
namespace {
	void load( uf::Object& self, const uf::Image& image ) {
		auto& graphic = self.getComponent<uf::Graphic>();
		auto& texture = graphic.material.textures.emplace_back();
		texture.loadFromImage( image );

		auto& mesh = self.getComponent<uf::Mesh>();
		mesh.bindIndirect<pod::DrawCommand>();
		mesh.bind<uf::graph::mesh::Base, uint16_t>();

		float scaleX = 1;
		float scaleY = 1;

		mesh.insertVertices<uf::graph::mesh::Base>({
			{ {-1.0f * scaleX, 1.0f * scaleY, 0.0f}, {0.0f, 1.0f}, {}, {0.0f, 1.0f}, {0.0f, 0.0f, -1.0f} },
			{ {-1.0f * scaleX, -1.0f * scaleY, 0.0f}, {0.0f, 0.0f}, {}, {0.0f, 0.0f}, {0.0f, 0.0f, -1.0f} },
			{ {1.0f * scaleX, -1.0f * scaleY, 0.0f}, {1.0f, 0.0f}, {}, {1.0f, 0.0f}, {0.0f, 0.0f, -1.0f} },
			{ {1.0f * scaleX, 1.0f * scaleY, 0.0f}, {1.0f, 1.0f}, {}, {1.0f, 1.0f}, {0.0f, 0.0f, -1.0f}, }
		});
		mesh.insertIndices<uint16_t>({
			0, 1, 2, 2, 3, 0
		});

		uf::graph::convert( self );
	}

	void load_migoto( uf::Object& self, const uf::Serializer& json ) {
	#if !UF_ENV_DREAMCAST && UF_USE_FLOAT16
		struct Position {
			pod::Vector3f position;
			pod::Vector3f normal;
			pod::Vector4f tangent;
		};
		struct TexCoord {
			pod::ColorRgba color;
			pod::Vector2f16 uv;
			pod::Vector2f st;
			pod::Vector2f16 wx;
		};
		struct Weight {
			pod::Vector4f alpha;
			pod::Vector4f indices;
		};

		auto& metadataJson = this->getComponent<uf::Serializer>();
		//auto root = metadataJson["system"]["root"].as<uf::stl::string>();
		auto root = json["root"].as(metadataJson["system"]["root"].as<uf::stl::string>());

		auto ib_buffer = uf::io::readAsBuffer( root + "/" + json["ib_buffer"].as<uf::stl::string>() );
		auto ib_buffer_start = (uint32_t*) ib_buffer.data();

		auto pos_buffer = uf::io::readAsBuffer( root + "/" + json["pos_buffer"].as<uf::stl::string>() );
		auto uv_buffer = uf::io::readAsBuffer( root + "/" + json["texcoord_buffer"].as<uf::stl::string>() );
		auto weight_buffer = uf::io::readAsBuffer( root + "/" + json["weight_buffer"].as<uf::stl::string>() );

		auto pos_buffer_start = (Position*) pos_buffer.data();
		auto uv_buffer_start = (TexCoord*) uv_buffer.data();
		auto weight_buffer_start = (Weight*) weight_buffer.data();

		auto pos_stride = sizeof(float) * 10;
		auto uv_stride = sizeof(uint8_t) * 4 + sizeof(float) * 4;
		auto weight_stride = sizeof(float) * 8;

		auto vb_stride =  pos_stride + uv_stride + weight_stride;
		auto ib_stride = sizeof(uint32_t);

		auto ib_count = ib_buffer.size() / ib_stride;
		auto vb_count = pos_buffer.size() / pos_stride;

		uf::stl::vector<uint32_t> indices( ib_buffer_start, ib_buffer_start + ib_count );
		uf::stl::vector<uf::graph::mesh::Skinned> vertices( vb_count );

		for ( auto i = 0; i < vb_count; ++i ) {
			auto& vertex = vertices[i];

			auto& pos = pos_buffer_start[i];
			auto& uv = uv_buffer_start[i];
			auto& weight = weight_buffer_start[i];

			vertex.position = pos.position;
			vertex.normal = pos.normal;
			vertex.tangent = pos.tangent;
			
			vertex.color = uv.color;
			vertex.uv[0] = uv.uv[0];
			vertex.uv[1] = uv.uv[1];
			//vertex.st = uv.st;
			
			vertex.joints = weight.indices;
			vertex.weights = weight.alpha;
		}

		uf::Image albedoImage;
		albedoImage.open( root + "/" + json["albedo_path"].as<uf::stl::string>() );

		auto& graphic = self.getComponent<uf::Graphic>();
		auto& albedoTexture = graphic.material.textures.emplace_back();
		auto& mesh = self.getComponent<uf::Mesh>();


		albedoTexture.loadFromImage( albedoImage );

		mesh.bindIndirect<pod::DrawCommand>();
		mesh.bind<uf::graph::mesh::Skinned, uint32_t>();

		mesh.insertVertices<uf::graph::mesh::Skinned>( vertices );
		mesh.insertIndices<uint32_t>( indices );

		uf::graph::convert( self );
	#endif
	}
}
void ext::CraetureBehavior::initialize( uf::Object& self ) {
/*
	this->addHook( "asset:Load.%UID%", [&](pod::payloads::assetLoad& payload){
		if ( !uf::asset::isExpected( payload, uf::asset::Type::IMAGE ) ) return;
		if ( !uf::asset::has( payload ) ) uf::asset::load( payload );
		const auto& image = uf::asset::get<uf::Image>( payload );

		::load( self, image );
	});
*/

	auto& metadata = this->getComponent<ext::CraetureBehavior::Metadata>();
	auto& metadataJson = this->getComponent<uf::Serializer>();

	UF_BEHAVIOR_METADATA_BIND_SERIALIZER_HOOKS(metadata, metadataJson);

	if ( !ext::json::isNull( metadataJson["migoto"] ) ) {
		::load_migoto( self, metadataJson["migoto"] );
	}
}
void ext::CraetureBehavior::tick( uf::Object& self ) {
	auto& metadata = this->getComponent<ext::CraetureBehavior::Metadata>();

	if ( this->hasComponent<pod::Graph>() ) {
		auto& graph = this->getComponent<pod::Graph>();
		pod::payloads::QueueAnimationPayload payload;
		payload.name = metadata.animation;
		this->queueHook("animation:Set.%UID%", payload);
		metadata.animation = "";
	}
}
void ext::CraetureBehavior::render( uf::Object& self ){}
void ext::CraetureBehavior::destroy( uf::Object& self ){}
void ext::CraetureBehavior::Metadata::serialize( uf::Object& self, uf::Serializer& serializer ){}
void ext::CraetureBehavior::Metadata::deserialize( uf::Object& self, uf::Serializer& serializer ){}
#undef this