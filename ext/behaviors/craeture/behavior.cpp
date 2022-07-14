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
UF_BEHAVIOR_TRAITS_CPP(ext::CraetureBehavior, ticks = false, renders = false, multithread = false)
#define this (&self)
namespace {
	void load( uf::Object& self, uf::Image& image ) {
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
}
void ext::CraetureBehavior::initialize( uf::Object& self ) {
	this->addHook( "asset:Load.%UID%", [&](pod::payloads::assetLoad& payload){
		if ( !uf::Asset::isExpected( payload, uf::Asset::Type::IMAGE ) ) return;

		uf::Scene& scene = uf::scene::getCurrentScene();
		uf::Asset& assetLoader = scene.getComponent<uf::Asset>();
		if ( !assetLoader.has<uf::Image>(payload.filename) ) return;
		auto& image = assetLoader.get<uf::Image>(payload.filename);
		::load( self, image );
	});

	auto& metadata = this->getComponent<ext::CraetureBehavior::Metadata>();
	auto& metadataJson = this->getComponent<uf::Serializer>();

	UF_BEHAVIOR_METADATA_BIND_SERIALIZER_HOOKS(metadata, metadataJson);
}
void ext::CraetureBehavior::tick( uf::Object& self ) {}
void ext::CraetureBehavior::render( uf::Object& self ){}
void ext::CraetureBehavior::destroy( uf::Object& self ){}
void ext::CraetureBehavior::Metadata::serialize( uf::Object& self, uf::Serializer& serializer ){}
void ext::CraetureBehavior::Metadata::deserialize( uf::Object& self, uf::Serializer& serializer ){}
#undef this