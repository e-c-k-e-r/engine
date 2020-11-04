#include <uf/ext/lua/lua.h>

sol::state ext::lua::state;
std::string ext::lua::main = "./data/scripts/main.lua";

#include <uf/utils/math/transform.h>
#include <uf/utils/math/physics.h>
#include <uf/utils/serialize/serializer.h>
#include <uf/engine/object/object.h>
#include <uf/engine/object/behaviors/lua.h>
#include <uf/utils/string/io.h>
#include <uf/utils/window/window.h>

void ext::lua::initialize() {
	state.open_libraries(sol::lib::base, sol::lib::package, sol::lib::table, sol::lib::math, sol::lib::string);

	POD_LUA_REGISTER_USERTYPE_BEGIN(Matrix4f)
		"value", []( pod::Matrix4f& self, const size_t& index ) {
			return self[index];
		}
	POD_LUA_REGISTER_USERTYPE_END()
	POD_LUA_REGISTER_USERTYPE_BEGIN(Vector3f)
		POD_LUA_REGISTER_USERTYPE_MEMBER(Vector3f, x),
		POD_LUA_REGISTER_USERTYPE_MEMBER(Vector3f, y),
		POD_LUA_REGISTER_USERTYPE_MEMBER(Vector3f, z)
	POD_LUA_REGISTER_USERTYPE_END()
	POD_LUA_REGISTER_USERTYPE_BEGIN(Vector4f)
		POD_LUA_REGISTER_USERTYPE_MEMBER(Vector4f, x),
		POD_LUA_REGISTER_USERTYPE_MEMBER(Vector4f, y),
		POD_LUA_REGISTER_USERTYPE_MEMBER(Vector4f, z),
		POD_LUA_REGISTER_USERTYPE_MEMBER(Vector4f, w)
	POD_LUA_REGISTER_USERTYPE_END()
	POD_LUA_REGISTER_USERTYPE_BEGIN(Quaternion<>)
		POD_LUA_REGISTER_USERTYPE_MEMBER(Quaternion<>, x),
		POD_LUA_REGISTER_USERTYPE_MEMBER(Quaternion<>, y),
		POD_LUA_REGISTER_USERTYPE_MEMBER(Quaternion<>, z),
		POD_LUA_REGISTER_USERTYPE_MEMBER(Quaternion<>, w)
	POD_LUA_REGISTER_USERTYPE_END()
	POD_LUA_REGISTER_USERTYPE_BEGIN(Transform<>)
		POD_LUA_REGISTER_USERTYPE_MEMBER(Transform<>, position),
		POD_LUA_REGISTER_USERTYPE_MEMBER(Transform<>, scale),
		POD_LUA_REGISTER_USERTYPE_MEMBER(Transform<>, up),
		POD_LUA_REGISTER_USERTYPE_MEMBER(Transform<>, right),
		POD_LUA_REGISTER_USERTYPE_MEMBER(Transform<>, forward),
		POD_LUA_REGISTER_USERTYPE_MEMBER(Transform<>, orientation),
		POD_LUA_REGISTER_USERTYPE_MEMBER(Transform<>, model),
		"move", []( pod::Transform<>& self, sol::variadic_args va ) {
			auto it = va.begin();
			if ( va.size() == 1 ) {
				pod::Vector3f delta = *it++;
				self = uf::transform::move( self, delta );
			} else if ( va.size() == 2 ) {
				pod::Vector3f axis = *it++;
				double delta = *it++;
				self = uf::transform::move( self, axis, delta );
			}
		},
		"rotate", []( pod::Transform<>& self, sol::variadic_args va ) {
			auto it = va.begin();
			if ( va.size() == 1 ) {
				pod::Quaternion<> delta = *it++;
				self = uf::transform::rotate( self, delta );
			} else if ( va.size() == 2 ) {
				pod::Vector3f axis = *it++;
				double delta = *it++;
				self = uf::transform::rotate( self, axis, delta );
			}
		}
	POD_LUA_REGISTER_USERTYPE_END()

	UF_LUA_REGISTER_USERTYPE_BEGIN(Object)
		"uid", &uf::Object::getUid,
		"name", &uf::Object::getName,
		"getComponent", [](uf::Object& self, const std::string& type, sol::this_state L ) {
			sol::variadic_results values;
			if ( type == "Metadata" ) {
				std::string serialized = self.getComponent<uf::Serializer>();
				sol::table table = state["json"]["decode"]( serialized );
				values.push_back( { L, sol::in_place, table } );
			} else if ( type == "Transform" ) {
				values.push_back( { L, sol::in_place, &self.getComponent<pod::Transform<>>() } );
			}
			return values;
		},
		"setComponent", [](uf::Object& self, const std::string& type, sol::variadic_args va ) {
			auto value = *va.begin();
			if ( type == "Metadata" ) {
				std::string encoded = state["json"]["encode"]( value.as<sol::table>() );
				self.getComponent<uf::Serializer>().merge(encoded);
			} else if ( type == "Transform" ) {
				self.getComponent<pod::Transform<>>() = value.as<pod::Transform<>>();
			}
		},
		"bind", [](uf::Object& self, const std::string& type, const sol::function& fun ) {
			if ( !self.hasBehavior<uf::LuaBehavior>() ) uf::instantiator::bind( "LuaBehavior", self );
			pod::Behavior* behaviorPointer = NULL;
			auto& behaviors = self.getBehaviors();
			for ( auto& b : behaviors ) {
				if ( b.type != self.getType<uf::LuaBehavior>() ) continue;
				behaviorPointer = &b;
				break;
			}
			if ( !behaviorPointer ) return false;
			pod::Behavior& behavior = *behaviorPointer;

			pod::Behavior::function_t* functionPointer = NULL;
			if ( type == "initialize" ) functionPointer = &behavior.initialize;
			else if ( type == "tick" ) functionPointer = &behavior.tick;
			else if ( type == "render" ) functionPointer = &behavior.render;
			else if ( type == "destroy" ) functionPointer = &behavior.destroy;
			
			if ( !functionPointer ) return false;
			pod::Behavior::function_t& function = *functionPointer;

			function = [fun]( uf::Object& s ) {
				fun(s);
			};
			return true;
		},
		"findByUid", []( uf::Object& self, const size_t& index )->uf::Object&{
			auto* pointer = self.findByUid( index );
			if ( pointer ) return pointer->as<uf::Object>();
			static uf::Object null;
			return null;
		},
		"findByName", []( uf::Object& self, const std::string& index )->uf::Object&{
			auto* pointer = self.findByName( index );
			if ( pointer ) return pointer->as<uf::Object>();
			static uf::Object null;
			return null;
		},
		"loadChild", []( uf::Object& self, const std::string& filename )->uf::Object&{
			auto* pointer = self.loadChildPointer( filename );
			if ( pointer ) return pointer->as<uf::Object>();
			static uf::Object null;
			return null;
		},
		"getChildren", []( uf::Object& self )->sol::table{
			sol::table table(ext::lua::state, sol::create);
			for ( auto* child : self.getChildren() ) {
				table.add(&child->as<uf::Object>());
			}
			return table;
		},
		"getParent", []( uf::Object& self )->uf::Object&{
			return self.getParent().as<uf::Object>();
		}
	UF_LUA_REGISTER_USERTYPE_END()

	// `hooks` table
	{

		auto hooks = state["hooks"].get_or_create<sol::table>();
		hooks["add"] = []( const std::string& name, const sol::function& function ) {
			uf::hooks.addHook( name, [function]( const std::string& payload )->std::string{
				return function( payload );
			});
		};
		hooks["call"] = []( const std::string& name, const std::string& payload ) {
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
		auto json = state.require_file("json", "./data/scripts/json.lua", true);
		state["json"]["pretty"] = []( const std::string& json )->std::string{
			uf::Serializer serializer = json;
			return serializer.serialize();
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
			auto result = state.safe_script_file( s, sol::script_pass_on_error );
			if ( !result.valid() ) {
				sol::error err = result;
				uf::iostream << "Invalid call to Lua script: " << err.what() << "\n";
				return false;
			}
		} else {
			state.script_file( s );
		}
	// is string with lua
	} else {
		if ( safe ) {
			auto result = state.safe_script( s, sol::script_pass_on_error );
			if ( !result.valid() ) {
				sol::error err = result;
				uf::iostream << "Invalid call to Lua script: " << err.what() << "\n";
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
	script.filename = filename;
	script.env = sol::environment( ext::lua::state, sol::create, ext::lua::state.globals() );
	return script;
}
bool ext::lua::run( const pod::LuaScript& s ) {
	std::string script = s.header + "\n" + uf::io::readAsString( s.filename );
	auto result = state.safe_script( script, s.env, sol::script_pass_on_error );
	if ( !result.valid() ) {
		sol::error err = result;
		uf::iostream << "Invalid call to Lua script: " << err.what() << "\n";
		return false;
	}
	return true;
}
void ext::lua::terminate() {

}