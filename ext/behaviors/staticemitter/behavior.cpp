#include "behavior.h"

#include <uf/utils/serialize/serializer.h>

#include <uf/utils/audio/audio.h>
#include <uf/utils/math/transform.h>
#include <uf/utils/string/ext.h>
#include <uf/engine/asset/asset.h>

EXT_BEHAVIOR_REGISTER_CPP(StaticEmitterBehavior)
#define this ((uf::Object*) &self)
void ext::StaticEmitterBehavior::initialize( uf::Object& self ) {
	auto& metadata = this->getComponent<uf::Serializer>();
	auto* hud = this->loadChildPointer("/hud.json", true);
	metadata["hud"]["uid"] = hud->getUid();
}
void ext::StaticEmitterBehavior::tick( uf::Object& self ) {
	auto& scene = uf::scene::getCurrentScene();
	auto& controller = scene.getController();

	auto& cTransform = controller.getComponent<pod::Transform<>>();
	auto& transform = this->getComponent<pod::Transform<>>();

	float distanceSquared = uf::vector::distanceSquared( transform.position, cTransform.position );

	auto& metadata = this->getComponent<uf::Serializer>();
	auto& sMetadata = scene.getComponent<uf::Serializer>();
	if ( metadata["static"]["scale"].isNumeric() ) {
		distanceSquared *= metadata["static"]["scale"].asFloat();
	}
	{
		float lo = metadata["static"]["range"][0].isNumeric() ? metadata["static"]["range"][0].asFloat() : 0.1f;
		float hi = metadata["static"]["range"][1].isNumeric() ? metadata["static"]["range"][1].asFloat() : 0.5f;
		float staticBlend = std::clamp( distanceSquared > 1 ? 1.0f / distanceSquared : 1.0f, lo, hi );

		float flicker = metadata["static"]["flicker"].isNumeric() ? metadata["static"]["flicker"].asFloat() : 0.001;
		float pieces = metadata["static"]["pieces"].isNumeric() ? metadata["static"]["pieces"].asFloat() : 1000;

		uint32_t mode = sMetadata["system"]["renderer"]["shader"]["mode"].asUInt();
		mode |= (0x1 << 1);

		sMetadata["system"]["renderer"]["shader"]["mode"] = mode;
		sMetadata["system"]["renderer"]["shader"]["parameters"][0] = flicker;
		sMetadata["system"]["renderer"]["shader"]["parameters"][1] = pieces;
		sMetadata["system"]["renderer"]["shader"]["parameters"][2] = staticBlend;
		sMetadata["system"]["renderer"]["shader"]["parameters"][3] = "time";
	}

	static uf::Timer<long long> timer(false);
	if ( !timer.running() ) timer.start();
	if ( metadata["hud"]["uid"].isNumeric() && timer.elapsed().asDouble() > 0.125 ) {
		timer.reset();
		auto* hud = this->findByUid( metadata["hud"]["uid"].asUInt64() );
		auto& hMetadata = hud->getComponent<uf::Serializer>();
		double d = uf::vector::distance( cTransform.position, transform.position );
		std::string string = uf::string::si( d, "m" );
		hMetadata["text settings"]["alpha"] = 0.5f;
		if ( hMetadata["text settings"]["string"].asString() != string ) {
			uf::Serializer payload;
			payload["string"] = string;
			hud->as<uf::Object>().callHook("gui:UpdateString.%UID%", payload);
		}
	}
}

void ext::StaticEmitterBehavior::render( uf::Object& self ){}
void ext::StaticEmitterBehavior::destroy( uf::Object& self ){}
#undef this