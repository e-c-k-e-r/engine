#include <uf/ext/lua/lua.h>
#if 0
#if UF_USE_LUA
#include <uf/utils/audio/audio.h>
UF_LUA_REGISTER_USERTYPE(uf::Audio,
	sol::call_constructor, sol::initializers( []( uf::Audio& self ){},
	[]( uf::Audio& self, const uf::stl::string& filename = "", double volume = 1 ){
		if ( filename != "" ) self.load(filename);
		self.setVolume(volume);
	}),
	UF_LUA_REGISTER_USERTYPE_MEMBER_FUN( uf::Audio::initialized ),
	UF_LUA_REGISTER_USERTYPE_MEMBER_FUN( uf::Audio::playing ),
	UF_LUA_REGISTER_USERTYPE_MEMBER_FUN( uf::Audio::destroy ),
	UF_LUA_REGISTER_USERTYPE_MEMBER_FUN( uf::Audio::load ),
	UF_LUA_REGISTER_USERTYPE_MEMBER_FUN( uf::Audio::play ),
	UF_LUA_REGISTER_USERTYPE_MEMBER_FUN( uf::Audio::stop ),
	UF_LUA_REGISTER_USERTYPE_MEMBER_FUN( uf::Audio::getTime ),
	UF_LUA_REGISTER_USERTYPE_MEMBER_FUN( uf::Audio::setTime ),
	UF_LUA_REGISTER_USERTYPE_MEMBER_FUN( uf::Audio::getDuration ),
	UF_LUA_REGISTER_USERTYPE_MEMBER_FUN( uf::Audio::setPosition ),
	UF_LUA_REGISTER_USERTYPE_MEMBER_FUN( uf::Audio::setOrientation ),
	UF_LUA_REGISTER_USERTYPE_MEMBER_FUN( uf::Audio::setVolume ),
	UF_LUA_REGISTER_USERTYPE_MEMBER_FUN( uf::Audio::getPitch ),
	UF_LUA_REGISTER_USERTYPE_MEMBER_FUN( uf::Audio::setPitch ),
	UF_LUA_REGISTER_USERTYPE_MEMBER_FUN( uf::Audio::getGain ),
	UF_LUA_REGISTER_USERTYPE_MEMBER_FUN( uf::Audio::setGain ),
	UF_LUA_REGISTER_USERTYPE_MEMBER_FUN( uf::Audio::getRolloffFactor ),
	UF_LUA_REGISTER_USERTYPE_MEMBER_FUN( uf::Audio::setRolloffFactor ),
	UF_LUA_REGISTER_USERTYPE_MEMBER_FUN( uf::Audio::getMaxDistance ),
	UF_LUA_REGISTER_USERTYPE_MEMBER_FUN( uf::Audio::setMaxDistance ),
	UF_LUA_REGISTER_USERTYPE_MEMBER_FUN( uf::Audio::getVolume ),
	UF_LUA_REGISTER_USERTYPE_MEMBER_FUN( uf::Audio::getFilename )
)
#endif
#endif