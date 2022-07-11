#include <uf/ext/lua/lua.h>
#if UF_USE_LUA
bool ext::lua::enabled = true;
sol::state ext::lua::state;
uf::stl::string ext::lua::main = uf::io::root + "/scripts/main.lua";
uf::stl::unordered_map<uf::stl::string, uf::stl::string> ext::lua::modules;

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
#include <uf/utils/io/inputs.h>

sol::table ext::lua::createTable() {
	return sol::table(ext::lua::state, sol::create);
}
uf::stl::string ext::lua::sanitize( const uf::stl::string& dirty, int index  ) {
	auto split = uf::string::split( dirty, "::" );
	if ( index < 0 ) index = split.size() + index;
	uf::stl::string part = split.at(index);
	part = uf::string::replace( part, "<>", "" );
	return part;
}
std::optional<uf::stl::string> ext::lua::encode( sol::table table ) {
	sol::protected_function fun = ext::lua::state["json"]["encode"];
	auto result = fun( table );
	if ( !result.valid() ) {
		sol::error err = result;
		UF_MSG_ERROR("{}", err.what())
		return "{}";
	}
	return result;
}
std::optional<sol::table> ext::lua::decode( const uf::stl::string& string ) {
	sol::protected_function fun = ext::lua::state["json"]["decode"];
	auto result = fun( string );
	if ( !result.valid() ) {
		sol::error err = result;
		UF_MSG_ERROR("{}", err.what())
		return createTable();
	}
	return result;
}

uf::stl::vector<std::function<void()>>* ext::lua::onInitializationFunctions = NULL;
void ext::lua::onInitialization( const std::function<void()>& function ) {
	if ( !ext::lua::onInitializationFunctions ) {
		ext::lua::onInitializationFunctions = new uf::stl::vector<std::function<void()>>;
	}
	auto& functions = *ext::lua::onInitializationFunctions;
	functions.emplace_back(function);
}


namespace binds {
	namespace hook {
		void add( const uf::stl::string& name, sol::function function ) {
			uf::hooks.addHook( name, [function](ext::json::Value& json){
				sol::table table = ext::lua::state["json"]["decode"]( json.dump() );
				auto result = function( table );
				if ( !result.valid() ) {
					sol::error err = result;
					UF_MSG_ERROR("{}", err.what())
				}
			});
		};
		void call( const uf::stl::string& name, sol::table table = ext::lua::createTable() ) {
			ext::json::Value payload = uf::Serializer(table);
			uf::hooks.call( name, payload );
			return;
		};
	}
	namespace entities {
		uf::Object& get(const uint& uid) {
			auto* p = uf::Entity::globalFindByUid(uid);
			if ( p ) return p->as<uf::Object>();
			static uf::Object null;
			return null;
		};
		uf::Object& currentScene() {
			return uf::scene::getCurrentScene().as<uf::Object>();
		};
		uf::Object& controller(){
			return uf::scene::getCurrentScene().getController().as<uf::Object>();
		};
		void destroy( uf::Object& object ) {
			object.queueDeletion();
		//	object.as<uf::Entity>().destroy();
		//	object.destroy();
		//	delete &object;
		};
	}
	namespace string {
		uf::stl::string extension( const uf::stl::string& filename ) {
			return uf::io::extension( filename );
		};
		uf::stl::string resolveURI( const uf::stl::string& filename, sol::variadic_args va ) {
			auto it = va.begin();
			uf::stl::string root = it != va.end() ? *(it++) : uf::stl::string("");
			return uf::io::resolveURI( filename, root );
		};
		uf::stl::string si( sol::variadic_args va ) {
			auto it = va.begin();
			double value = *(it++);
			uf::stl::string unit = *(it++);
			size_t precision = va.size() > 2 ? *(it++) : 3;
			return uf::string::si( value, unit, precision );
		};
	}
	namespace io {
		void print( sol::variadic_args va ) {
			size_t count = va.size();
			for ( auto value : va ) {
				uf::stl::string str = ext::lua::state["tostring"]( value );
				fmt::print("{}", str);
				if ( --count != 0 ) fmt::print("\t");
			}
			fmt::print("\n");
			std::cout.flush();
		};
	}
	namespace math {
		double clamp( double value, double min, double max ) {
			return std::clamp( value, min, max );
		};
	}
	namespace time {
		double current(){ return uf::physics::time::current; };
		double previous(){ return uf::physics::time::previous; };
		double delta(){ return uf::physics::time::delta; };
	}
	namespace json {
		uf::stl::string pretty( const uf::stl::string& json ){
			uf::Serializer serializer = json;
			return serializer.serialize();
		};
		sol::table readFromFile( const uf::stl::string& filename ){
			uf::Serializer serializer;
			serializer.readFromFile( filename );
			uf::stl::string string = serializer.serialize();
			auto decoded = ext::lua::decode( string );
			return decoded ? decoded.value() : ext::lua::createTable();
		};
		bool writeToFile( sol::table table, const uf::stl::string& path ) {
			if ( uf::io::extension(path) != "json" ) return false;
			auto encoded = ext::lua::encode( table );
			if ( encoded ) {
				uf::Serializer json = encoded.value();
				json.writeToFile( path );
				return true;
			}
			return false;
		};
	}
	namespace os {
		uf::stl::string arch() {
			return UF_ENV;
		}
	}
}

void ext::lua::initialize() {
	if ( !ext::lua::enabled ) return;

	state.open_libraries(sol::lib::base, sol::lib::package, sol::lib::table, sol::lib::math, sol::lib::string, sol::lib::ffi, sol::lib::jit);

	// load modules
	for ( auto pair : modules ) {
		const uf::stl::string& name = pair.first;
		const uf::stl::string& script = pair.second;
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
		hooks["add"] = UF_LUA_C_FUN(::binds::hook::add);
		hooks["call"] = UF_LUA_C_FUN(::binds::hook::call);
	}
	// `entities` table
	{
		auto entities = state["entities"].get_or_create<sol::table>();
		entities["get"] = UF_LUA_C_FUN(::binds::entities::get);
		entities["currentScene"] = UF_LUA_C_FUN(::binds::entities::currentScene);
		entities["controller"] = UF_LUA_C_FUN(::binds::entities::controller);
		entities["destroy"] = UF_LUA_C_FUN(::binds::entities::destroy);
	}
	// `string` table
	{
		auto string = state["string"].get_or_create<sol::table>();
		string["extension"] = UF_LUA_C_FUN(::binds::string::extension);
		string["resolveURI"] = UF_LUA_C_FUN(::binds::string::resolveURI);
		string["si"] = UF_LUA_C_FUN(::binds::string::si);
		
		string["match"] = UF_LUA_C_FUN(uf::string::match);
		string["matched"] = UF_LUA_C_FUN(uf::string::matched);
	}
	// `io` table
	{
		auto io = state["io"].get_or_create<sol::table>();
		io["print"] = UF_LUA_C_FUN(::binds::io::print);
	}
	// `math` table
	{
		auto math = state["math"].get_or_create<sol::table>();
		math["clamp"] = UF_LUA_C_FUN(::binds::math::clamp);
	}
	// `time` table
	{
		auto time = state["time"].get_or_create<sol::table>();
		time.set("current", UF_LUA_C_FUN(::binds::time::current));
		time.set("previous", UF_LUA_C_FUN(::binds::time::previous));
		time.set("delta", UF_LUA_C_FUN(::binds::time::delta));
	}
	// `json` table
	{
		state["json"]["pretty"] = UF_LUA_C_FUN(::binds::json::pretty);
		state["json"]["readFromFile"] = UF_LUA_C_FUN(::binds::json::readFromFile);
		state["json"]["writeToFile"] = UF_LUA_C_FUN(::binds::json::writeToFile);
	}
	// `window` table
	{
		auto window = state["window"].get_or_create<sol::table>();
		window["keyPressed"] = UF_LUA_C_FUN(uf::Window::isKeyPressed);
	}
	// `os` table
	{
		auto os = state["os"].get_or_create<sol::table>();
		os["arch"] = UF_LUA_C_FUN(::binds::os::arch);
	}
	// `inputs` table
	{
		auto inputs = state["inputs"].get_or_create<sol::table>();
		inputs["key"] = UF_LUA_C_FUN(uf::inputs::key);
		inputs["analog"] = UF_LUA_C_FUN(uf::inputs::analog);
		inputs["analog2"] = UF_LUA_C_FUN(uf::inputs::analog2);
	}
	run(main);
}
bool ext::lua::run( const uf::stl::string& s, bool safe ) {
	// is file
	if ( uf::io::extension(s) == "lua" ) {
		if ( safe ) {
			auto result = state.safe_script_file( uf::io::resolveURI( s ), sol::script_pass_on_error );
			if ( !result.valid() ) {
				sol::error err = result;
				UF_MSG_ERROR("{}", err.what());
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
				UF_MSG_ERROR("{}", err.what());
				return false;
			}
		} else {
			state.script( s );
		}
	}
	return true;
}

pod::LuaScript ext::lua::script( const uf::stl::string& filename ) {
	pod::LuaScript script;
	if ( !ext::lua::enabled ) return script;
	
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
				UF_MSG_ERROR("{}", err.what());
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
				UF_MSG_ERROR("{}", err.what());
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
#endif