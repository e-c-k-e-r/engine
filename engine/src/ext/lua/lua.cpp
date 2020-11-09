#include <uf/ext/lua/lua.h>

sol::state ext::lua::state;
std::string ext::lua::main = "./data/scripts/main.lua";
std::unordered_map<std::string, std::string> ext::lua::modules;

#include <uf/utils/math/transform.h>
#include <uf/utils/math/physics.h>
#include <uf/utils/serialize/serializer.h>
#include <uf/engine/asset/asset.h>
#include <uf/engine/object/object.h>
#include <uf/engine/object/behaviors/lua.h>
#include <uf/utils/string/io.h>
#include <uf/utils/string/ext.h>
#include <uf/utils/window/window.h>
#include <uf/utils/audio/audio.h>

sol::table ext::lua::createTable() {
	return sol::table(ext::lua::state, sol::create);
}
std::string ext::lua::sanitize( const std::string& dirty, int index  ) {
	auto split = uf::string::split( dirty, "::" );
	if ( index < 0 ) index = split.size() + index;
	std::string part = split.at(index);
	part = uf::string::replace( part, "<>", "" );
	return part;
}
std::optional<std::string> ext::lua::encode( sol::table table ) {
	sol::protected_function fun = ext::lua::state["json"]["encode"];
	auto result = fun( table );
	if ( !result.valid() ) {
		sol::error err = result;
		uf::iostream << err.what() << "\n";
		return "{}";
	}
	return result;
}
std::optional<sol::table> ext::lua::decode( const std::string& string ) {
	sol::protected_function fun = ext::lua::state["json"]["decode"];
	auto result = fun( string );
	if ( !result.valid() ) {
		sol::error err = result;
		uf::iostream << err.what() << "\n";
		return createTable();
	}
	return result;
}

std::vector<std::function<void()>>* ext::lua::onInitializationFunctions = NULL;
void ext::lua::onInitialization( const std::function<void()>& function ) {
	if ( !ext::lua::onInitializationFunctions ) {
		ext::lua::onInitializationFunctions = new std::vector<std::function<void()>>;
	}
	auto& functions = *ext::lua::onInitializationFunctions;
	functions.emplace_back(function);
}

void ext::lua::initialize() {
	state.open_libraries(sol::lib::base, sol::lib::package, sol::lib::table, sol::lib::math, sol::lib::string, sol::lib::ffi, sol::lib::jit);

	// load modules
	for ( auto pair : modules ) {
		const std::string& name = pair.first;
		const std::string& script = pair.second;
		if ( uf::io::extension(script) == "lua" ) {
			state.require_file(name, uf::io::resolveURI( script ), true);
		} else {
			state.require_script(name, script, true);
		}
	}

	// load on-initialization defines
	if ( ext::lua::onInitializationFunctions ) {
		auto& functions = *ext::lua::onInitializationFunctions;
		for ( auto& function : functions ) function();
	}
	
	// `hooks` table
	{

		auto hooks = state["hooks"].get_or_create<sol::table>();
		hooks["add"] = []( const std::string& name, const sol::function& function ) {
			uf::hooks.addHook( name, [function]( const std::string& payload )->std::string{
				sol::table table = ext::lua::state["json"]["decode"]( payload );
				auto result = function( table );
				if ( !result.valid() ) {
					sol::error err = result;
					uf::iostream << err.what() << "\n";
					return "false";
				}
				return "true";
			});
		};
		hooks["call"] = []( const std::string& name, sol::table table = createTable() ) {
			uf::Serializer payload = table;
			return uf::hooks.call( name, payload );
		};
	}
	// `entities` table
	{
		auto entities = state["entities"].get_or_create<sol::table>();
		entities["get"] = [](const uint& uid)->uf::Object&{
			auto* p = uf::Entity::globalFindByUid(uid);
			if ( p ) return p->as<uf::Object>();
			static uf::Object null;
			return null;
		};
		entities["currentScene"] = []()->uf::Object&{
			return uf::scene::getCurrentScene().as<uf::Object>();
		};
		entities["controller"] = []()->uf::Object&{
			return uf::scene::getCurrentScene().getController().as<uf::Object>();
		};
		entities["destroy"] = []( uf::Object& object ){
			object.getParent().removeChild(object);
			object.destroy();
		//	delete &object;
		};
	}
	// `string` table
	{
		auto string = state["string"].get_or_create<sol::table>();
		string["extension"] = []( const std::string& filename ) {
			return uf::io::extension( filename );
		};
		string["resolveURI"] = []( const std::string& filename ) {
			return uf::io::resolveURI( filename );
		};
		string["si"] = []( sol::variadic_args va ) {
			auto it = va.begin();
			double value = *(it++);
			std::string unit = *(it++);
			size_t precision = va.size() > 2 ? *(it++) : 3;
			return uf::string::si( value, unit, precision );
		};
	}
	// `io` table
	{
		auto io = state["io"].get_or_create<sol::table>();
		io["print"] = []( sol::variadic_args va ) {
			size_t count = va.size();
			for ( auto value : va ) {
				std::string str = ext::lua::state["tostring"]( value );
				uf::iostream << str;
				if ( --count != 0 ) uf::iostream << "\t";
			}
			uf::iostream << "\n";
		};
	}
	// `math` table
	{
		auto math = state["math"].get_or_create<sol::table>();
		math["clamp"] = []( double value, double min, double max ) {
			return std::clamp( value, min, max );
		};
	}
	// `time` table
	{
		auto time = state["time"].get_or_create<sol::table>();
		time["current"] = []()->double{ return uf::physics::time::current; };
		time["previous"] = []()->double{ return uf::physics::time::previous; };
		time["delta"] = []()->double{ return uf::physics::time::delta; };
	}
	// `json` table
	{
		state["json"]["pretty"] = []( const std::string& json )->std::string{
			uf::Serializer serializer = json;
			return serializer.serialize();
		};
		state["json"]["readFromFile"] = []( const std::string& filename ){
			uf::Serializer serializer;
			serializer.readFromFile( filename );
			std::string string = serializer.serialize(false);
			auto decoded = decode( string );
			return decoded ? decoded.value() : ext::lua::createTable();
		};
		state["json"]["writeToFile"] = []( sol::table table, const std::string& path ) {
			if ( uf::io::extension(path) != "json" ) return false;
			auto encoded = encode( table );
			if ( encoded ) {
				uf::Serializer json = encoded.value();
				json.writeToFile( path );
				return true;
			}
			return false;
		};
	}
	// `window` table
	{
		auto window = state["window"].get_or_create<sol::table>();
		window["keyPressed"] = [](const std::string& name)->bool{
			return uf::Window::isKeyPressed(name);
		};
	}
	run(main);
}
bool ext::lua::run( const std::string& s, bool safe ) {
	// is file
	if ( uf::io::extension(s) == "lua" ) {
		if ( safe ) {
			auto result = state.safe_script_file( uf::io::resolveURI( s ), sol::script_pass_on_error );
			if ( !result.valid() ) {
				sol::error err = result;
				uf::iostream << err.what() << "\n";
				return false;
			}
		} else {
			state.script_file( uf::io::resolveURI( s ) );
		}
	// is string with lua
	} else {
		if ( safe ) {
			auto result = state.safe_script( s, sol::script_pass_on_error );
			if ( !result.valid() ) {
				sol::error err = result;
				uf::iostream << err.what() << "\n";
				return false;
			}
		} else {
			state.script( s );
		}
	}
	return true;
}

pod::LuaScript ext::lua::script( const std::string& filename ) {
	pod::LuaScript script;
	script.file = filename;
	script.env = sol::environment( ext::lua::state, sol::create, ext::lua::state.globals() );
	return script;
}
bool ext::lua::run( const pod::LuaScript& s, bool safe ) {
	// is file
	if ( uf::io::extension(s.file) == "lua" ) {
		if ( safe ) {
			auto result = state.safe_script_file( s.file, s.env, sol::script_pass_on_error );
			if ( !result.valid() ) {
				sol::error err = result;
				uf::iostream << err.what() << "\n";
				return false;
			}
		} else {
			state.script_file( s.file );
		}
	// is string with lua
	} else {
		if ( safe ) {
			auto result = state.safe_script( s.file, s.env, sol::script_pass_on_error );
			if ( !result.valid() ) {
				sol::error err = result;
				uf::iostream << err.what() << "\n";
				return false;
			}
		} else {
			state.script( s.file );
		}
	}
	return true;
}
void ext::lua::terminate() {
	if ( ext::lua::onInitializationFunctions ) {
		delete ext::lua::onInitializationFunctions;
		ext::lua::onInitializationFunctions = NULL;
	}
}