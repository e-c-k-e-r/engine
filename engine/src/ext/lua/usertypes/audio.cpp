#include <uf/ext/lua/lua.h>

#include <uf/utils/audio/audio.h>

UF_LUA_REGISTER_USERTYPE(uf::Audio,
	sol::call_constructor, sol::initializers( []( uf::Audio& self ){},
	[]( uf::Audio& self, const std::string& filename = "", double volume = 1 ){
		if ( filename != "" ) self.load(filename);
		self.setVolume(volume);
	}),
	UF_LUA_REGISTER_USERTYPE_MEMBER( uf::Audio::initialized ),
	UF_LUA_REGISTER_USERTYPE_MEMBER( uf::Audio::playing ),
	UF_LUA_REGISTER_USERTYPE_MEMBER( uf::Audio::destroy ),
	UF_LUA_REGISTER_USERTYPE_MEMBER( uf::Audio::load ),
	UF_LUA_REGISTER_USERTYPE_MEMBER( uf::Audio::play ),
	UF_LUA_REGISTER_USERTYPE_MEMBER( uf::Audio::stop ),
	UF_LUA_REGISTER_USERTYPE_MEMBER( uf::Audio::getTime ),
	UF_LUA_REGISTER_USERTYPE_MEMBER( uf::Audio::setTime ),
	UF_LUA_REGISTER_USERTYPE_MEMBER( uf::Audio::getDuration ),
	UF_LUA_REGISTER_USERTYPE_MEMBER( uf::Audio::setPosition ),
	UF_LUA_REGISTER_USERTYPE_MEMBER( uf::Audio::setOrientation ),
	UF_LUA_REGISTER_USERTYPE_MEMBER( uf::Audio::setVolume ),
	UF_LUA_REGISTER_USERTYPE_MEMBER( uf::Audio::getPitch ),
	UF_LUA_REGISTER_USERTYPE_MEMBER( uf::Audio::setPitch ),
	UF_LUA_REGISTER_USERTYPE_MEMBER( uf::Audio::getGain ),
	UF_LUA_REGISTER_USERTYPE_MEMBER( uf::Audio::setGain ),
	UF_LUA_REGISTER_USERTYPE_MEMBER( uf::Audio::getRolloffFactor ),
	UF_LUA_REGISTER_USERTYPE_MEMBER( uf::Audio::setRolloffFactor ),
	UF_LUA_REGISTER_USERTYPE_MEMBER( uf::Audio::getMaxDistance ),
	UF_LUA_REGISTER_USERTYPE_MEMBER( uf::Audio::setMaxDistance ),
	UF_LUA_REGISTER_USERTYPE_MEMBER( uf::Audio::getVolume ),
	UF_LUA_REGISTER_USERTYPE_MEMBER( uf::Audio::getFilename )
)