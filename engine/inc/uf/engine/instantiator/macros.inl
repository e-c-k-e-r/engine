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
		uf::instantiator::registerBehavior<BEHAVIOR>( UF_NS_GET_LAST(BEHAVIOR) );\
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
		std::string name = UF_NS_GET_LAST(OBJ);\
		uf::instantiator::registerObject<OBJ>( name );

#define UF_OBJECT_BIND_BEHAVIOR( BEHAVIOR )\
		uf::instantiator::registerBinding( name, UF_NS_GET_LAST(BEHAVIOR) );

#define UF_OBJECT_REGISTER_BEHAVIOR( BEHAVIOR )\
		uf::instantiator::registerBehavior<BEHAVIOR>( UF_NS_GET_LAST(BEHAVIOR) );\
		UF_OBJECT_BIND_BEHAVIOR( BEHAVIOR )

#define UF_OBJECT_REGISTER_END()\
	});\
}

/*
#define UF_OBJECT_REGISTER_CPP( OBJ ) \
namespace {\
	static uf::StaticInitialization REGISTER_UF_ ## OBJ( []{\
		uf::instantiator::registerObject<uf::OBJ>( #OBJ );\
	});\
}
#define EXT_OBJECT_REGISTER_CPP( OBJ ) \
namespace {\
	static uf::StaticInitialization REGISTER_EXT_ ## OBJ( []{\
		uf::instantiator::registerObject<ext::OBJ>( #OBJ );\
	});\
}
#define UF_BEHAVIOR_REGISTER_CPP( BEHAVIOR ) \
namespace {\
	static uf::StaticInitialization REGISTER_UF_ ## BEHAVIOR( []{\
		uf::instantiator::registerBehavior<uf::BEHAVIOR>( #BEHAVIOR );\
	});\
}
#define EXT_BEHAVIOR_REGISTER_CPP( BEHAVIOR ) \
namespace {\
	static uf::StaticInitialization REGISTER_EXT_ ## BEHAVIOR( []{\
		uf::instantiator::registerBehavior<ext::BEHAVIOR>( #BEHAVIOR );\
	});\
}
#define EXT_BEHAVIOR_REGISTER_AS_OBJECT( BEHAVIOR, OBJ )\
namespace {\
	static uf::StaticInitialization REGISTER_AS_OBJECT_EXT_ ## BEHAVIOR( []{\
		uf::instantiator::registerBinding( #OBJ, #BEHAVIOR );\
	});\
}

#define UF_OBJECT_REGISTER_BEGIN( OBJ )\
namespace {\
	static uf::StaticInitialization REGISTER_EXT_ ## OBJ( []{\
		std::string name = #OBJ;\
		uf::instantiator::registerObject<uf::OBJ>( name );

#define UF_OBJECT_BIND_BEHAVIOR( BEHAVIOR )\
		uf::instantiator::registerBinding( name, #BEHAVIOR );

#define UF_OBJECT_REGISTER_BEHAVIOR( BEHAVIOR )\
		uf::instantiator::registerBehavior<uf::BEHAVIOR>( #BEHAVIOR );\
		UF_OBJECT_BIND_BEHAVIOR( BEHAVIOR )

#define UF_OBJECT_REGISTER_END()\
	});\
}

#define EXT_OBJECT_REGISTER_BEGIN( OBJ )\
namespace {\
	static uf::StaticInitialization REGISTER_EXT_ ## OBJ( []{\
		std::string name = #OBJ;\
		uf::instantiator::registerObject<ext::OBJ>( name );

#define EXT_OBJECT_BIND_BEHAVIOR( BEHAVIOR )\
		uf::instantiator::registerBinding( name, #BEHAVIOR );

#define EXT_OBJECT_REGISTER_BEHAVIOR( BEHAVIOR )\
		uf::instantiator::registerBehavior<ext::BEHAVIOR>( #BEHAVIOR );\
		EXT_OBJECT_BIND_BEHAVIOR( BEHAVIOR )

#define EXT_OBJECT_REGISTER_END()\
	});\
}
*/