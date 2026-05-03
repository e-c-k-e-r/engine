#pragma once

#define UF_ENTITY_METADATA_USE_JSON 0

#define UF_BEHAVIOR_DEFINE_TYPE() // static constexpr const char* type = __FILE__;
#define UF_BEHAVIOR_DEFINE_TRAITS() namespace Traits { extern UF_API bool ticks; extern UF_API bool renders; extern UF_API uf::stl::string thread; }
#define UF_BEHAVIOR_DEFINE_METADATA(...) struct Metadata : public pod::Behavior::Metadata {\
public:\
	/*virtual*/ void serialize( uf::Object&, uf::Serializer& );\
	/*virtual*/ void deserialize( uf::Object&, uf::Serializer& );\
	__VA_ARGS__\
}

#define UF_BEHAVIOR_DEFINE_FUNCTIONS()\
	void UF_API attach( uf::Entity& );\
	void UF_API initialize( uf::Object& );\
	void UF_API tick( uf::Object& );\
	void UF_API render( uf::Object& );\
	void UF_API destroy( uf::Object& );

#define EXT_BEHAVIOR_DEFINE_TRAITS() namespace Traits { extern bool ticks; extern bool renders; extern uf::stl::string thread; }
#define EXT_BEHAVIOR_DEFINE_FUNCTIONS()\
	void attach( uf::Entity& );\
	void initialize( uf::Object& );\
	void tick( uf::Object& );\
	void render( uf::Object& );\
	void destroy( uf::Object& );

#define UF_BEHAVIOR_TRAITS_CPP( OBJ, TICKS, RENDERS, THREAD)\
	bool OBJ::Traits::TICKS;\
	bool OBJ::Traits::RENDERS;\
	uf::stl::string OBJ::Traits::THREAD;

#define UF_BEHAVIOR_ENTITY_METADATA_H( OBJ, METADATA )\
	namespace OBJ ## Behavior {\
		UF_BEHAVIOR_DEFINE_TYPE;\
		UF_BEHAVIOR_DEFINE_TRAITS;\
		UF_BEHAVIOR_DEFINE_FUNCTIONS;\
		UF_BEHAVIOR_DEFINE_METADATA METADATA;\
	};

#define UF_BEHAVIOR_ENTITY_H( OBJ ) UF_BEHAVIOR_ENTITY_METADATA_H( OBJ, struct Metadata{}; )

#define UF_BEHAVIOR_ENTITY_CPP_BEGIN( OBJ )\
	void OBJ ## Behavior::attach( uf::Entity& self ) {\
		self.addBehavior(pod::Behavior{\
			.type = TYPE(OBJ ## Behavior::Metadata)/*OBJ ## Behavior::type*/,\
			.traits = {\
				.ticks = OBJ ## Behavior::Traits::ticks,\
				.renders = OBJ ## Behavior::Traits::renders,\
				.thread = OBJ ## Behavior::Traits::thread,\
			},\
			.initialize = OBJ ## Behavior::initialize,\
			.tick = OBJ ## Behavior::tick,\
			.render = OBJ ## Behavior::render,\
			.destroy = OBJ ## Behavior::destroy,\
		});\
	}

#define UF_BEHAVIOR_ENTITY_CPP_ATTACH( OBJ ) {\
	OBJ ## Behavior::attach( *this );\
}

#define UF_BEHAVIOR_ENTITY_CPP_END( OBJ )

#define UF_BEHAVIOR_METADATA_BIND_SERIALIZER_HOOKS( METADATA, JSON ) {\
	this->addHook( "object:Serialize.%UID%", [&]{ METADATA.serialize(self, JSON); });\
	this->addHook( "object:Serialize.%UID%", [&](ext::json::Value& json){ METADATA.serialize(self, (uf::Serializer&) json); });\
	this->addHook( "object:Deserialize.%UID%", [&]{	 METADATA.deserialize(self, JSON); });\
	this->addHook( "object:Deserialize.%UID%", [&](ext::json::Value& json){	 METADATA.deserialize(self, (uf::Serializer&) json); });\
	METADATA.deserialize(self, JSON);\
}
