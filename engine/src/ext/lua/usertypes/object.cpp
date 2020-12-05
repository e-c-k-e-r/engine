#include <uf/ext/lua/lua.h>

#include <uf/utils/math/transform.h>
#include <uf/utils/audio/audio.h>
#include <uf/utils/camera/camera.h>
#include <uf/engine/object/object.h>
#include <uf/engine/asset/asset.h>
#include <uf/engine/object/behaviors/lua.h>

namespace {
	static uf::StaticInitialization TOKEN_PASTE(STATIC_INITIALIZATION_, __LINE__)( []{
		ext::lua::onInitialization( []{
			ext::lua::state.new_usertype<uf::Object>(UF_NS_GET_LAST(uf::Object),
				sol::call_constructor, sol::initializers( []( uf::Object& self, sol::object arg, bool init = true ){
					if ( arg.is<std::string>() ) {
						self.load( arg.as<std::string>() );
					} else if ( arg.is<sol::table>() ) {
						auto encoded = ext::lua::encode( arg.as<sol::table>() );
						if ( encoded ) {
							uf::Serializer json = encoded.value();
							self.load(json);
						}
					}
					if ( init ) {
						self.initialize();
					}
				}),
				"uid", &uf::Object::getUid ,
				"name", &uf::Object::getName ,
				"formatHookName", [](uf::Object& self, const std::string n ){
					return self.formatHookName(n);
				},
				"getComponent", [](uf::Object& self, const std::string& type )->sol::object{
					#define UF_LUA_RETRIEVE_COMPONENT( T )\
						if ( type == UF_NS_GET_LAST(T) ) return sol::make_object( ext::lua::state, std::ref(self.getComponent<T>()) );

					if ( type == "Metadata" ) {
						auto& metadata = self.getComponent<uf::Serializer>();
						auto decoded = ext::lua::decode( metadata );
						if ( decoded ) {
							sol::table table = decoded.value();
							return sol::make_object( ext::lua::state, table );
						}
					}
					UF_LUA_RETRIEVE_COMPONENT(pod::Transform<>)
					UF_LUA_RETRIEVE_COMPONENT(uf::Audio)
					UF_LUA_RETRIEVE_COMPONENT(uf::Asset)
					UF_LUA_RETRIEVE_COMPONENT(uf::Camera)
					return sol::make_object( ext::lua::state, sol::lua_nil );
				},
				"setComponent", [](uf::Object& self, const std::string& type, sol::object value ) {
					#define UF_LUA_UPDATE_COMPONENT( T )\
						else if ( type == UF_NS_GET_LAST(T) ) self.getComponent<T>() = std::move(value.as<T>());

					if ( type == "Metadata" ) {
						auto encoded = ext::lua::encode( value.as<sol::table>() );
						if ( encoded ) {
							auto& metadata = self.getComponent<uf::Serializer>();
							std::string str = encoded.value();
							uf::Serializer hooks = metadata["system"]["hooks"];
							metadata.merge( str, false );
							metadata["system"]["hooks"] = hooks;
						}
					}
					UF_LUA_UPDATE_COMPONENT(pod::Transform<>)
					UF_LUA_UPDATE_COMPONENT(uf::Audio)
					UF_LUA_UPDATE_COMPONENT(uf::Asset)
					UF_LUA_UPDATE_COMPONENT(uf::Camera)
				},
				"bind", [](uf::Object& self, const std::string& type, sol::protected_function fun ) {
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
						auto result = fun(s);
						if ( !result.valid() ) {
							sol::error err = result;
							uf::iostream << err.what() << "\n";
						}
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
				"addChild", []( uf::Object& self, uf::Object& child )->uf::Object&{
					self.addChild( child );
					return self;
				},
				"removeChild", []( uf::Object& self, uf::Object& child )->uf::Object&{
					self.removeChild( child );
					return self;
				},
				"loadChild", []( uf::Object& self, const std::string& filename, bool init = true )->uf::Object&{
					auto* pointer = self.loadChildPointer( filename, init );
					if ( pointer ) return pointer->as<uf::Object>();
					static uf::Object null;
					return null;
				},
				"getChildren", []( uf::Object& self )->sol::table{
					sol::table table = ext::lua::createTable();
					for ( auto* child : self.getChildren() ) {
						table.add(&child->as<uf::Object>());
					}
					return table;
				},
				"getParent", []( uf::Object& self )->uf::Object&{
					return self.getParent().as<uf::Object>();
				},
				"addHook", []( uf::Object& self, const std::string& name, const sol::function& function ) {
					self.addHook( name, [function](ext::json::Value& json){
						std::string payload = json.dump();
						auto decoded = ext::lua::decode( payload );
						if ( !decoded ) return;
						sol::table table = decoded.value();
						auto result = function( table );
						if ( !result.valid() ) {
							sol::error err = result;
							uf::iostream << err.what() << "\n";
							return;
						}
					});
				},
				"callHook", []( uf::Object& self, const std::string& name, sol::table table = ext::lua::createTable() ) {
					uf::Serializer payload = table;
					self.callHook( name, (ext::json::Value&) payload );
				},
				"queueHook", []( uf::Object& self, const std::string& name, sol::table table, float delay = 0.0f ) {
					uf::Serializer payload = table;
					self.queueHook( name, (ext::json::Value&) payload, delay );
				},
				"__tostring", []( uf::Object& self ) {
					return self.getName() + ": " + std::to_string( self.getUid() );
				}
			);
		});
	});
}