#pragma once

#define UF_ENTITY_METADATA_USE_JSON 0
#define UF_BEHAVIOR_DEFINE_TYPE static constexpr const char* type = __FILE__;

#define UF_BEHAVIOR_ENTITY_METADATA_H( OBJ, METADATA )\
	namespace OBJ ## Behavior {\
		UF_BEHAVIOR_DEFINE_TYPE;\
		void attach( uf::Entity& );\
		void initialize( uf::Object& );\
		void tick( uf::Object& );\
		void render( uf::Object& );\
		void destroy( uf::Object& );\
		METADATA;\
	};

#define UF_BEHAVIOR_ENTITY_H( OBJ ) UF_BEHAVIOR_ENTITY_METADATA_H( OBJ, struct Metadata{}; )

#define UF_BEHAVIOR_ENTITY_CPP_BEGIN( OBJ )\
	void OBJ ## Behavior::attach( uf::Entity& self ) {\
		self.addBehavior(pod::Behavior{\
			.type = OBJ ## Behavior::type,\
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