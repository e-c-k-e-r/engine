#include <uf/ext/lua/lua.h>
#if UF_USE_LUA
#include <uf/utils/math/transform.h>
#include <uf/utils/audio/audio.h>
#include <uf/utils/camera/camera.h>
#include <uf/engine/object/object.h>
#include <uf/engine/asset/asset.h>
#include <uf/utils/math/physics.h>
#include <uf/engine/object/behaviors/lua.h>

namespace binds {
	namespace enums {
		enum Components {
			Metadata,
			Transform,
			Audio,
			Asset,
			Camera,
			Physics,
			PhysicsState,
		};
	
		static uf::StaticInitialization TOKEN_PASTE(STATIC_INITIALIZATION_, __LINE__)( []{
			#define UF_LUA_REGISTER_ENUM(E) #E, enums::Components::E
			ext::lua::onInitialization( []{
				auto enums = ext::lua::state.new_enum("Components",
					UF_LUA_REGISTER_ENUM(Metadata),
					UF_LUA_REGISTER_ENUM(Transform),
					UF_LUA_REGISTER_ENUM(Audio),
					UF_LUA_REGISTER_ENUM(Asset),
					UF_LUA_REGISTER_ENUM(Camera),
					UF_LUA_REGISTER_ENUM(Physics),
					UF_LUA_REGISTER_ENUM(PhysicsState)
				);
			});
		});
	}

	uf::stl::string formatHookName(uf::Object& self, const uf::stl::string n ){
		return self.formatHookName(n);
	}
	sol::object getComponentFromEnum( uf::Object& self, binds::enums::Components type ) {
	#define UF_LUA_RETRIEVE_COMPONENT_FROM_ENUM( E, T )\
		case enums::Components::E: return sol::make_object( ext::lua::state, std::ref(self.getComponent<T>()) );

		switch ( type ) {
			case enums::Components::Metadata: {
				self.callHook( "object:Serialize.%UID%" );
				auto& metadata = self.getComponent<uf::Serializer>();
				auto decoded = ext::lua::decode( metadata );
				if ( decoded ) {
					sol::table table = decoded.value();
					return sol::make_object( ext::lua::state, table );
				}
				UF_MSG_ERROR("Failed to deserialize metadata for {}: {}", self.getName(), self.getUid());
			} break;
			UF_LUA_RETRIEVE_COMPONENT_FROM_ENUM( Transform, pod::Transform<> );
			UF_LUA_RETRIEVE_COMPONENT_FROM_ENUM( Audio, uf::Audio );
			UF_LUA_RETRIEVE_COMPONENT_FROM_ENUM( Asset, uf::Asset );
			UF_LUA_RETRIEVE_COMPONENT_FROM_ENUM( Camera, uf::Camera );
			UF_LUA_RETRIEVE_COMPONENT_FROM_ENUM( Physics, pod::Physics );
			UF_LUA_RETRIEVE_COMPONENT_FROM_ENUM( PhysicsState, pod::PhysicsState );
		}
		UF_MSG_ERROR("Invalid component of {} requested for {}: {}", type, self.getName(), self.getUid());

		return sol::make_object( ext::lua::state, sol::lua_nil );
	}
	sol::object getComponentFromString( uf::Object& self, const uf::stl::string& type ) {
		#define UF_LUA_RETRIEVE_COMPONENT_FROM_STRING( T )\
			else if ( type == UF_NS_GET_LAST(T) ) return sol::make_object( ext::lua::state, std::ref(self.getComponent<T>()) );

		if ( type == "Metadata" ) {
			self.callHook( "object:Serialize.%UID%" );
			auto& metadata = self.getComponent<uf::Serializer>();
			auto decoded = ext::lua::decode( metadata );
			if ( decoded ) {
				sol::table table = decoded.value();
				return sol::make_object( ext::lua::state, table );
			}
			UF_MSG_ERROR("Failed to deserialize metadata for {}: {}", self.getName(), self.getUid());
		}
		UF_LUA_RETRIEVE_COMPONENT_FROM_STRING(pod::Transform<>)
		UF_LUA_RETRIEVE_COMPONENT_FROM_STRING(uf::Audio)
		UF_LUA_RETRIEVE_COMPONENT_FROM_STRING(uf::Asset)
		UF_LUA_RETRIEVE_COMPONENT_FROM_STRING(uf::Camera)
		UF_LUA_RETRIEVE_COMPONENT_FROM_STRING(pod::Physics)
		UF_LUA_RETRIEVE_COMPONENT_FROM_STRING(pod::PhysicsState)
		UF_MSG_ERROR("Invalid component of {} requested for {}: {}", type, self.getName(), self.getUid());
		return sol::make_object( ext::lua::state, sol::lua_nil );
	}
	void setComponent(uf::Object& self, const uf::stl::string& type, sol::object value ) {
		#define UF_LUA_UPDATE_COMPONENT( T )\
			else if ( type == UF_NS_GET_LAST(T) ) self.getComponent<T>() = std::move(value.as<T>());

		if ( type == "Metadata" ) {
			auto encoded = ext::lua::encode( value.as<sol::table>() );
			if ( encoded ) {
			/*
				self.callHook( "object:Serialize.%UID%" );
				auto& metadata = self.getComponent<uf::Serializer>();
				uf::stl::string str = encoded.value();
				metadata.merge( str, false );
				self.callHook( "object:Deserialize.%UID%" );
			*/
				uf::stl::string str = encoded.value();
				ext::json::Value json;
				ext::json::decode( json, str );
				self.callHook( "object:Deserialize.%UID%", json );
			}
		}
		UF_LUA_UPDATE_COMPONENT(pod::Transform<>)
		UF_LUA_UPDATE_COMPONENT(uf::Audio)
		UF_LUA_UPDATE_COMPONENT(uf::Asset)
		UF_LUA_UPDATE_COMPONENT(uf::Camera)
		UF_LUA_UPDATE_COMPONENT(pod::Physics)
		UF_LUA_UPDATE_COMPONENT(pod::PhysicsState)
	}
	bool bind(uf::Object& self, const uf::stl::string& type, sol::protected_function fun ) {
	//	if ( !self.hasBehavior({.type = uf::LuaBehavior::type}) ) uf::instantiator::bind( "LuaBehavior", self );
		if ( !self.hasBehavior({.type = TYPE(uf::LuaBehavior::Metadata)}) ) uf::instantiator::bind( "LuaBehavior", self );
		pod::Behavior* behaviorPointer = NULL;
		auto& behaviors = self.getBehaviors();
		for ( auto& b : behaviors ) {
		//	if ( b.type != uf::LuaBehavior::type ) continue;
			if ( b.type != TYPE(uf::LuaBehavior::Metadata) ) continue;
			behaviorPointer = &b;
			break;
		}
		if ( !behaviorPointer ) return false;
		pod::Behavior& behavior = *behaviorPointer;

		pod::Behavior::function_t* functionPointer = NULL;
		if ( type == "initialize" ) functionPointer = &behavior.initialize;
		else if ( type == "tick" ) { functionPointer = &behavior.tick; behavior.traits.ticks = true; behavior.traits.multithread = false; }
		else if ( type == "render" ) { functionPointer = &behavior.render; behavior.traits.renders = true; }
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
		self.generateGraph();
		return true;
	}
	uf::Object& findByUid( uf::Object& self, size_t index ) {
		auto* pointer = self.findByUid( index );
		if ( pointer ) return pointer->as<uf::Object>();
		static uf::Object null;
		return null;
	}
	uf::Object& findByName( uf::Object& self, const uf::stl::string& index ){
		auto* pointer = self.findByName( index );
		if ( pointer ) return pointer->as<uf::Object>();
		static uf::Object null;
		return null;
	}
	uf::Object& addChild( uf::Object& self, uf::Object& child ) {
		self.addChild( child );
		return self;
	}
	uf::Object& removeChild( uf::Object& self, uf::Object& child ){
		self.removeChild( child );
		return self;
	}
	uf::Object& loadChild( uf::Object& self, const uf::stl::string& filename, bool init = true ) {
		auto* pointer = self.loadChildPointer( filename, init );
		if ( pointer ) return pointer->as<uf::Object>();
		static uf::Object null;
		return null;
	}
	sol::table getChildren( uf::Object& self ){
		sol::table table = ext::lua::createTable();
		for ( auto* child : self.getChildren() ) {
			table.add(&child->as<uf::Object>());
		}
		return table;
	}
	uf::Object& getParent( uf::Object& self ){
		return self.getParent().as<uf::Object>();
	}
	void addHook( uf::Object& self, const uf::stl::string& name, sol::function function ) {
		self.addHook( name, [function](ext::json::Value& json){
			uf::stl::string payload = json.dump();
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
	}
	void callHook( uf::Object& self, const uf::stl::string& name, sol::table table = ext::lua::createTable() ) {
		ext::json::Value payload = uf::Serializer(table);
		self.callHook( name, payload );
	}
	void lazyCallHook( uf::Object& self, const uf::stl::string& name, sol::table table = ext::lua::createTable() ) {
		ext::json::Value payload = uf::Serializer(table);
		self.lazyCallHook( name, payload );
	}
	void queueHook( uf::Object& self, const uf::stl::string& name, sol::table table, float delay = 0.0f ) {
		ext::json::Value payload = uf::Serializer(table);
		self.queueHook( name, payload, delay );
	}
	uf::stl::string toString( uf::Object& self ) {
		return self.getName() + ": " + std::to_string( self.getUid() );
	}

	size_t getUid( const uf::Object& o ) { return o.getUid(); }
	uf::stl::string getName( const uf::Object& o ) { return o.getName(); }
}


UF_LUA_REGISTER_USERTYPE(uf::Object,
	sol::call_constructor, sol::initializers(
		[]( uf::Object& self, sol::object arg, bool init = true ){
			if ( arg.is<uf::stl::string>() ) {
				self.load( arg.as<uf::stl::string>() );
			} else if ( arg.is<sol::table>() ) {
				auto encoded = ext::lua::encode( arg.as<sol::table>() );
				if ( encoded ) {
					uf::Serializer json = encoded.value();
					self.load(json);
				}
			}
			if ( init ) self.initialize();
		}
	),
	UF_LUA_REGISTER_USERTYPE_DEFINE( uid, UF_LUA_C_FUN(::binds::getUid) ),
	UF_LUA_REGISTER_USERTYPE_DEFINE( name, UF_LUA_C_FUN(::binds::getName) ),
	UF_LUA_REGISTER_USERTYPE_DEFINE( formatHookName, UF_LUA_C_FUN(::binds::formatHookName) ), 
	UF_LUA_REGISTER_USERTYPE_DEFINE( getComponent, UF_LUA_C_FUN(::binds::getComponentFromEnum) ),
	UF_LUA_REGISTER_USERTYPE_DEFINE( getComponent, UF_LUA_C_FUN(::binds::getComponentFromString) ),
	UF_LUA_REGISTER_USERTYPE_DEFINE( setComponent, UF_LUA_C_FUN(::binds::setComponent) ),
	UF_LUA_REGISTER_USERTYPE_DEFINE( bind, UF_LUA_C_FUN(::binds::bind) ),
	UF_LUA_REGISTER_USERTYPE_DEFINE( findByUid, UF_LUA_C_FUN(::binds::findByUid) ),
	UF_LUA_REGISTER_USERTYPE_DEFINE( findByName, UF_LUA_C_FUN(::binds::findByName) ),
	UF_LUA_REGISTER_USERTYPE_DEFINE( addChild, UF_LUA_C_FUN(::binds::addChild) ),
	UF_LUA_REGISTER_USERTYPE_DEFINE( removeChild, UF_LUA_C_FUN(::binds::removeChild) ),
	UF_LUA_REGISTER_USERTYPE_DEFINE( loadChild, UF_LUA_C_FUN(::binds::loadChild) ),
	UF_LUA_REGISTER_USERTYPE_DEFINE( getChildren, UF_LUA_C_FUN(::binds::getChildren) ),
	UF_LUA_REGISTER_USERTYPE_DEFINE( getParent, UF_LUA_C_FUN(::binds::getParent) ),
	UF_LUA_REGISTER_USERTYPE_DEFINE( addHook, UF_LUA_C_FUN(::binds::addHook) ),
	UF_LUA_REGISTER_USERTYPE_DEFINE( callHook, UF_LUA_C_FUN(::binds::callHook) ),
	UF_LUA_REGISTER_USERTYPE_DEFINE( lazyCallHook, UF_LUA_C_FUN(::binds::lazyCallHook) ),
	UF_LUA_REGISTER_USERTYPE_DEFINE( queueHook, UF_LUA_C_FUN(::binds::queueHook) ),
	UF_LUA_REGISTER_USERTYPE_DEFINE( __tostring, UF_LUA_C_FUN(::binds::toString) )
)
#endif