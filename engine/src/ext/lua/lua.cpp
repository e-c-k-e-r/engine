#include <uf/ext/lua/lua.h>

#if UF_USE_LUA

bool ext::lua::enabled = true;
sol::state ext::lua::state;
uf::stl::string ext::lua::main;
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
#include <uf/utils/io/fmt.h>

namespace {
	struct vfs_reader {
		uf::stl::vector<uint8_t> buffer;
		bool read_once;

		vfs_reader(const uf::stl::string& filename) : read_once(false) {
			uf::io::readAsBuffer(buffer, filename);
		}

		static const char* read(lua_State*, void* data, size_t* size) {
			vfs_reader* reader = static_cast<vfs_reader*>(data);

			if (!reader->read_once && !reader->buffer.empty()) {
				*size = reader->buffer.size();
				reader->read_once = true;
				return reinterpret_cast<const char*>(reader->buffer.data());
			}

			*size = 0;
			return nullptr;
		}
	};

	ext::json::Value encodeNode( sol::object obj ) {
		if ( obj.get_type() == sol::type::boolean ) return ext::json::Value(obj.as<bool>());
		if ( obj.get_type() == sol::type::number ) return ext::json::Value(obj.as<double>());
		if ( obj.get_type() == sol::type::string ) return ext::json::Value(obj.as<uf::stl::string>());

		if ( obj.get_type() == sol::type::table ) {
			sol::table t = obj.as<sol::table>();
			ext::json::Value json;

			bool isArray = true;
			size_t expectedIndex = 1;
			for (auto& kv : t) {
				if (kv.first.get_type() != sol::type::number || kv.first.as<size_t>() != expectedIndex++) {
					isArray = false;
					break;
				}
			}

			if ( isArray ) {
				for ( auto& kv : t ) json.emplace_back( encodeNode(kv.second) );
			} else {
				for ( auto& kv : t ) {
					if ( kv.first.get_type() == sol::type::string ) {
						json[kv.first.as<uf::stl::string>()] = encodeNode( kv.second );
					}
				}
			}
			return json;
		}
		return ext::json::Value();
	}

	sol::object decodeNode(const ext::json::Value& json) {
		if ( json.is_null() ) return sol::lua_nil;
		if ( json.is_boolean() ) return sol::make_object(ext::lua::state, json.as<bool>());
		if ( json.is_number() ) return sol::make_object(ext::lua::state, json.as<double>());
		if ( json.is_string() ) return sol::make_object(ext::lua::state, json.as<uf::stl::string>());

		if ( json.is_array() ) {
			sol::table t = ext::lua::state.create_table();
			ext::json::forEach(json, [&](size_t i, const ext::json::Value& val) {
				t[i + 1] = decodeNode(val);
			});
			return t;
		}

		if ( json.is_object() ) {
			sol::table t = ext::lua::state.create_table();
			ext::json::forEach(json, [&](const uf::stl::string& key, const ext::json::Value& val) {
				t[key] = decodeNode(val);
			});
			return t;
		}
		return sol::lua_nil;
	}
}

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

/*
std::optional<uf::stl::string> ext::lua::encode( sol::table table ) {
	LUA_FUN fun = ext::lua::state["json"]["encode"];
	auto result = fun( table );
#if UF_LUA_PCALLS
	if ( !result.valid() ) {
		sol::error err = result;
		UF_MSG_ERROR("{}", err.what())
		return "{}";
	}
#endif
	return result;
}
std::optional<sol::table> ext::lua::decode( const uf::stl::string& string ) {
	LUA_FUN fun = ext::lua::state["json"]["decode"];
	auto result = fun( string );
#if UF_LUA_PCALLS
	if ( !result.valid() ) {
		sol::error err = result;
		UF_MSG_ERROR("{}", err.what())
		return createTable();
	}
#endif
	return result;
}
*/

std::optional<ext::json::Value> ext::lua::encode(sol::table table) {
	return ::encodeNode(table);
}
std::optional<sol::table> ext::lua::decode(const ext::json::Value& json) {
	sol::object obj = ::decodeNode(json);
	if ( obj.is<sol::table>() ) return obj.as<sol::table>();
	return ext::lua::state.create_table();
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
		void add( const uf::stl::string& name, sol::protected_function function ) {
		uf::hooks.addHook( name, [function]( const pod::Hook::userdata_t& payload ) -> pod::Hook::userdata_t {
			ext::json::Value jsonPayload;
			if ( payload.is<ext::json::Value>() ) {
				jsonPayload = payload.get<ext::json::Value>();
			}
			sol::table table = ext::lua::decode(jsonPayload).value_or(ext::lua::state.create_table());

			auto result = function( table );

		#if UF_LUA_PCALLS
			if ( !result.valid() ) {
				sol::error err = result;
				UF_MSG_ERROR("{}", err.what());
				return pod::Hook::userdata_t();
			}
		#endif

			sol::object retObj = result;
			if ( retObj.get_type() != sol::type::lua_nil && retObj.is<sol::table>() ) {
				auto encodedOpt = ext::lua::encode(retObj.as<sol::table>());
				if (encodedOpt.has_value()) {
					pod::Hook::userdata_t ret;
					ret.create<ext::json::Value>(encodedOpt.value());
					return ret;
				}
			}

			return pod::Hook::userdata_t();
		}, pod::Hook::Type{UF_USERDATA_CTTI(ext::json::Value), sizeof(ext::json::Value)});
	};

	sol::object call( const uf::stl::string& name, sol::table table = ext::lua::createTable() ) {
		auto encodedOpt = ext::lua::encode(table);
		ext::json::Value payload = encodedOpt.has_value() ? encodedOpt.value() : ext::json::Value();

		pod::Hook::userdata_t ud;
		ud.create<ext::json::Value>(payload);

		auto results = uf::hooks.call( name, ud );

		if ( results.empty() ) return sol::lua_nil;

		for ( auto& res : results ) {
			if ( !res.is<ext::json::Value>() ) continue;
			auto decodedOpt = ext::lua::decode( res.get<ext::json::Value>() );
			if ( decodedOpt.has_value() ) {
				return decodedOpt.value();
			}
		}
		return sol::lua_nil;
	};
	}
	namespace entities {
		uf::Object& get(const uint& uid) {
			auto* p = uf::Entity::globalFindByUid(uid);
			if ( p ) return p->as<uf::Object>();
			static uf::Object null;
			return null;
		};
		uf::Object& create( sol::optional<bool> init ) {
			auto& entity = uf::instantiator::instantiate("Object");
			if ( init.value_or(true) ) entity.initialize();
			return entity;
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
		uf::stl::vector<uf::Entity*> all() {
			return uf::scene::getCurrentScene().getGraph();
		}
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
				::fmt::print("{}", str);
				if ( --count != 0 ) ::fmt::print("\t");
			}
			::fmt::print("\n");
			fflush(stdout);
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
	namespace physics {
		pod::PhysicsBody& create( uf::Object& object, sol::optional<float> mass, sol::optional<pod::Vector3f> center ) {
			return uf::physics::create( object, mass.value_or(0.0f), center.value_or(pod::Vector3f{}) );
		}
	}
	namespace json {
		uf::stl::string pretty( const uf::stl::string& json ){
			uf::Serializer serializer = json;
			return serializer.serialize();
		};
		sol::table readFromFile( const uf::stl::string& filename ){
			uf::Serializer serializer;
			serializer.readFromFile( filename );
			auto decoded = ext::lua::decode( serializer );
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

	state.open_libraries(
		 sol::lib::base
		,sol::lib::package
		,sol::lib::table
		,sol::lib::math
		,sol::lib::string
		,sol::lib::bit32
#if UF_USE_LUAJIT
		,sol::lib::ffi
		,sol::lib::jit
#endif
	);

	// load modules
	for ( auto pair : modules ) {
		const uf::stl::string& name = pair.first;
		const uf::stl::string& script = pair.second;
		if ( uf::io::extension(script) == "lua" ) {
			uf::stl::string code;
			if ( uf::io::readAsString(code, script) ) {
				// Pass 'script' as the chunk name so your modules also have proper error tracing!
				state.require_script(name, code, true, script);
			} else {
				UF_MSG_ERROR("Lua: failed to load module via VFS: {}", script);
			}
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
		hooks.set("add", UF_LUA_C_FUN(::binds::hook::add));
		hooks.set("call", UF_LUA_C_FUN(::binds::hook::call));
	}
	// `entities` table
	{
		auto entities = state["entities"].get_or_create<sol::table>();
		entities.set("get", UF_LUA_C_FUN(::binds::entities::get));
		entities.set("create", UF_LUA_C_FUN(::binds::entities::create));
		entities.set("currentScene", UF_LUA_C_FUN(::binds::entities::currentScene));
		entities.set("controller", UF_LUA_C_FUN(::binds::entities::controller));
		entities.set("destroy", UF_LUA_C_FUN(::binds::entities::destroy));
		entities.set("all", UF_LUA_C_FUN(::binds::entities::all));
	}
	// `string` table
	{
		auto string = state["string"].get_or_create<sol::table>();
		string.set("extension", UF_LUA_C_FUN(::binds::string::extension));
		string.set("resolveURI", UF_LUA_C_FUN(::binds::string::resolveURI));
		string.set("si", UF_LUA_C_FUN(::binds::string::si));
		
	//	string.set("match", UF_LUA_C_FUN(uf::string::match));
		string.set("matched", UF_LUA_C_FUN(uf::string::matched));
	}
	// `io` table
	{
		auto io = state["io"].get_or_create<sol::table>();
		io.set("print", UF_LUA_C_FUN(::binds::io::print));
	}
	// `math` table
	{
		auto math = state["math"].get_or_create<sol::table>();
		math.set("clamp", UF_LUA_C_FUN(::binds::math::clamp));
	}
	// `time` table
	{
		auto time = state["time"].get_or_create<sol::table>();
		time.set("current", UF_LUA_C_FUN(::binds::time::current));
		time.set("previous", UF_LUA_C_FUN(::binds::time::previous));
		time.set("delta", UF_LUA_C_FUN(::binds::time::delta));
	}
	// `physics` table
	{
		auto physics = state["physics"].get_or_create<sol::table>();
		physics.set("create", UF_LUA_C_FUN(::binds::physics::create));
	}
	// `json` table
	{
		auto json = state["json"].get_or_create<sol::table>();
		json.set("pretty", UF_LUA_C_FUN(::binds::json::pretty));
		json.set("readFromFile", UF_LUA_C_FUN(::binds::json::readFromFile));
		json.set("writeToFile", UF_LUA_C_FUN(::binds::json::writeToFile));
	}
	// `window` table
	{
		auto window = state["window"].get_or_create<sol::table>();
		window.set("keyPressed", UF_LUA_C_FUN(uf::Window::isKeyPressed));
	}
	// `os` table
	{
		auto os = state["os"].get_or_create<sol::table>();
		os.set("arch", UF_LUA_C_FUN(::binds::os::arch));
	}
	// `inputs` table
	{
		auto inputs = state["inputs"].get_or_create<sol::table>();
		inputs.set("key", UF_LUA_C_FUN(uf::inputs::key));
		inputs.set("analog", UF_LUA_C_FUN(uf::inputs::analog));
		inputs.set("analog2", UF_LUA_C_FUN(uf::inputs::analog2));
	}
	run(main);
}
bool ext::lua::run( const uf::stl::string& s, bool safe ) {
	// is file
	if ( uf::io::extension(s) == "lua" ) {
		uf::stl::string resolved = uf::io::resolveURI(s);
		vfs_reader reader(resolved);

		uf::stl::string chunkname = "@" + resolved;
		sol::load_result loaded = state.load(vfs_reader::read, &reader, chunkname.c_str());

		if ( !loaded.valid() ) {
			sol::error err = loaded;
			UF_MSG_ERROR("{}", err.what());
			return false;
		}

		if ( safe ) {
			sol::protected_function script = loaded;
			auto result = script();
			if ( !result.valid() ) {
				sol::error err = result;
				UF_MSG_ERROR("{}", err.what());
				return false;
			}
		} else {
			sol::unsafe_function script = loaded;
			script();
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
	ext::lua::script( filename, script );
	return script;
}
void ext::lua::script( const uf::stl::string& filename, pod::LuaScript& script ) {
	if ( !ext::lua::enabled ) return;
	script.file = filename;
	script.env = sol::environment( ext::lua::state, sol::create, ext::lua::state.globals() );
}
bool ext::lua::run( const pod::LuaScript& s, bool safe ) {
	// is file
	if ( uf::io::extension(s.file) == "lua" ) {
		uf::stl::string resolved = uf::io::resolveURI(s.file);
		vfs_reader reader(resolved);

		uf::stl::string chunkname = "@" + resolved;
		sol::load_result loaded = state.load(vfs_reader::read, &reader, chunkname.c_str());

		if ( !loaded.valid() ) {
			sol::error err = loaded;
			UF_MSG_ERROR("{}", err.what());
			return false;
		}

		sol::protected_function script = loaded;

		s.env.set_on(script);

		if ( safe ) {
			auto result = script();
			if ( !result.valid() ) {
				sol::error err = result;
				UF_MSG_ERROR("{}", err.what());
				return false;
			}
		} else {
			sol::unsafe_function unsafe_script = script;
			unsafe_script();
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
			state.script( s.file, s.env );
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