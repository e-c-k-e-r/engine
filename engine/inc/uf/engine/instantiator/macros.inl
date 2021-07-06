#pragma once

#define UF_OBJECT_REGISTER_CPP( OBJ ) \
namespace {\
	static uf::StaticInitialization TOKEN_PASTE(STATIC_INITIALIZATION_, __LINE__)( []{\
		uf::instantiator::registerObject<OBJ>( UF_NS_GET_LAST(OBJ) );\
	});\
}

#define UF_BEHAVIOR_REGISTER_CPP( BEHAVIOR ) \
namespace {\
	static uf::StaticInitialization TOKEN_PASTE(STATIC_INITIALIZATION_, __LINE__)( []{\
		uf::instantiator::registerBehavior(UF_NS_GET_LAST(BEHAVIOR), pod::Behavior{\
			.type = TYPE(BEHAVIOR::Metadata),\
			.traits = {\
				.ticks = BEHAVIOR::Traits::ticks,\
				.renders = BEHAVIOR::Traits::renders,\
				.multithread = BEHAVIOR::Traits::multithread,\
			},\
			.initialize = BEHAVIOR::initialize,\
			.tick = BEHAVIOR::tick,\
			.render = BEHAVIOR::render,\
			.destroy = BEHAVIOR::destroy,\
		});\
	});\
}

#define UF_BEHAVIOR_REGISTER_AS_OBJECT( BEHAVIOR, OBJ )\
namespace {\
	static uf::StaticInitialization TOKEN_PASTE(STATIC_INITIALIZATION_, __LINE__)( []{\
		uf::instantiator::registerBinding( UF_NS_GET_LAST(OBJ), UF_NS_GET_LAST(BEHAVIOR) );\
	});\
}

#define UF_OBJECT_REGISTER_BEGIN( OBJ )\
namespace {\
	static uf::StaticInitialization TOKEN_PASTE(STATIC_INITIALIZATION_, __LINE__)( []{\
		uf::stl::string name = UF_NS_GET_LAST(OBJ);\
		uf::instantiator::registerObject<OBJ>( name );

#define UF_OBJECT_BIND_BEHAVIOR( BEHAVIOR )\
		uf::instantiator::registerBinding( name, UF_NS_GET_LAST(BEHAVIOR) );

#define UF_OBJECT_REGISTER_BEHAVIOR( BEHAVIOR )\
		if ( !uf::instantiator::behaviors ) uf::instantiator::behaviors = new uf::stl::unordered_map<uf::stl::string, pod::Behavior>;\
		uf::instantiator::registerBehavior(UF_NS_GET_LAST(BEHAVIOR), pod::Behavior{\
			.type = TYPE(BEHAVIOR::Metadata),\
			.traits = {\
				.ticks = BEHAVIOR::Traits::ticks,\
				.renders = BEHAVIOR::Traits::renders,\
				.multithread = BEHAVIOR::Traits::multithread,\
			},\
			.initialize = BEHAVIOR::initialize,\
			.tick = BEHAVIOR::tick,\
			.render = BEHAVIOR::render,\
			.destroy = BEHAVIOR::destroy,\
		});\
		UF_OBJECT_BIND_BEHAVIOR( BEHAVIOR )

#define UF_OBJECT_REGISTER_END()\
	});\
}
