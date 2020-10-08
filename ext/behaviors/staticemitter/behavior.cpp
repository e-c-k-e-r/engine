#include "behavior.h"

#include <uf/utils/serialize/serializer.h>

#include <uf/utils/audio/audio.h>
#include <uf/utils/math/transform.h>
#include <uf/engine/asset/asset.h>

#include <mutex>

EXT_BEHAVIOR_REGISTER_CPP(StaticEmitterBehavior)
#define this ((uf::Scene*) &self)

void ext::StaticEmitterBehavior::initialize( uf::Object& self ) {

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
		if ( !(mode & 0x2) ) {
			mode = mode | 0x2;
		}

		sMetadata["system"]["renderer"]["shader"]["mode"] = mode;
		sMetadata["system"]["renderer"]["shader"]["parameters"][0] = flicker;
		sMetadata["system"]["renderer"]["shader"]["parameters"][1] = pieces;
		sMetadata["system"]["renderer"]["shader"]["parameters"][2] = staticBlend;
		sMetadata["system"]["renderer"]["shader"]["parameters"][3] = "time";
	}
}

void ext::StaticEmitterBehavior::render( uf::Object& self ){}
void ext::StaticEmitterBehavior::destroy( uf::Object& self ){}
#undef this