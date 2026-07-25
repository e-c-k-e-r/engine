#include <uf/ext/lua/lua.h>
#include <uf/ext/lua/component.h>
#if UF_USE_LUA
#include <uf/utils/math/transform.h>
#include <uf/utils/audio/audio.h>
#include <uf/utils/camera/camera.h>
#include <uf/engine/object/object.h>
#include <uf/engine/asset/asset.h>
#include <uf/utils/math/physics.h>
#include <uf/engine/object/behaviors/lua.h>

uf::stl::unordered_map<uf::stl::string, ext::lua::GetComponent> ext::lua::componentGetters;

namespace binds {
	uf::hashed_string formatHookName(uf::Object& self, const uf::stl::string n ){
		return self.formatHookName(n);
	}

	sol::object getComponent( uf::Object& self, const uf::stl::string& type ) {
		if ( type == "Metadata" ) {
			self.callHook( "object:Serialize.%UID%" );
			auto& metadata = self.getComponent<uf::Serializer>();
			if ( !ext::json::isObject( metadata ) ) {
				sol::table table( ext::lua::state, sol::create );
				return sol::make_object( ext::lua::state, table );
			}

			auto decoded = ext::lua::decode( metadata );
			if ( decoded ) {
				sol::table table = decoded.value();
				return sol::make_object( ext::lua::state, table );
			}
			UF_MSG_ERROR("Failed to deserialize metadata for {}: {}", self.getName(), self.getUid());
		}
		
		auto it = ext::lua::componentGetters.find(type);
		if ( it != ext::lua::componentGetters.end() ) return it->second(self);
		UF_MSG_ERROR("Invalid component of {} requested for {}", type, uf::string::toString(self));
		return sol::make_object(ext::lua::state, sol::lua_nil);
	}
	bool bind(uf::Object& self, const uf::stl::string& type, LUA_FUN fun ) {
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

		auto& metadata = self.getComponent<uf::LuaBehavior::Metadata>();
		uf::stl::vector<LUA_FUN>* functionPointer = NULL;
		if ( type == "initialize" ) functionPointer = &metadata.initialize;
		else if ( type == "tick" ) { functionPointer = &metadata.tick; behavior.traits.ticks = true; behavior.traits.thread = ""; }
		else if ( type == "render" ) { functionPointer = &metadata.render; behavior.traits.renders = true; }
		else if ( type == "destroy" ) functionPointer = &metadata.destroy;
		if ( !functionPointer ) return false;
		functionPointer->emplace_back( fun );

#if 0
		pod::Behavior::function_t* functionPointer = NULL;
		if ( type == "initialize" ) functionPointer = &behavior.initialize;
		else if ( type == "tick" ) { functionPointer = &behavior.tick; behavior.traits.ticks = true; behavior.traits.thread = ""; }
		else if ( type == "render" ) { functionPointer = &behavior.render; behavior.traits.renders = true; }
		else if ( type == "destroy" ) functionPointer = &behavior.destroy;
		
		if ( !functionPointer ) return false;
		pod::Behavior::function_t& function = *functionPointer;

		bool hasExisting = (bool)(function); // check if a function is already bound to this slot
		auto prev = function; // copy the existing function

	#if !UF_LUA_PCALLS
		if ( hasExisting ) {
			function = [prev, fun]( uf::Object& s ) {
				prev(s);
				fun(s);
			};
		} else {
			function = fun;
		}
	#else
		if ( hasExisting ) {
			function = [prev, fun]( uf::Object& s ) {
				prev(s); // call the previous script's tick
				auto result = fun(s); // call the new script's tick
				if ( !result.valid() ) {
					sol::error err = result;
					UF_MSG_ERROR("{}", err.what());
				}
			};
		} else {
			function = [fun]( uf::Object& s ) {
				auto result = fun(s);
				if ( !result.valid() ) {
					sol::error err = result;
					UF_MSG_ERROR("{}", err.what());
				}
			};
		}
	#endif
#endif
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
	uf::Object& globalFindByUid( uf::Object& self, size_t index ) {
		auto* pointer = self.globalFindByUid( index );
		if ( pointer ) return pointer->as<uf::Object>();
		static uf::Object null;
		return null;
	}
	uf::Object& globalFindByName( uf::Object& self, const uf::stl::string& index ){
		auto* pointer = self.globalFindByName( index );
		if ( pointer ) return pointer->as<uf::Object>();
		static uf::Object null;
		return null;
	}
	uf::Object& createChild( uf::Object& self, sol::optional<bool> init ) {
		return self.createChild( init.value_or( true ) );
	}
	uf::Object& addChild( uf::Object& self, uf::Object& child ) {
		self.addChild( child );
		return self;
	}
	uf::Object& removeChild( uf::Object& self, uf::Object& child ){
		self.removeChild( child );
		return self;
	}
	uf::Object& loadChild( uf::Object& self, sol::optional<uf::stl::string> filename, sol::optional<bool> init ) {
		auto* pointer = self.loadChildPointer( filename.value_or(""), init.value_or(true) );
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
	void addHook( uf::Object& self, const uf::stl::string& name, LUA_FUN fun ) {
		self.addHook( name, [fun](ext::json::Value& json){
			// cringe
			if ( ext::json::isNull( json ) ) {
				auto result = fun();
			#if UF_LUA_PCALLS
				if ( !result.valid() ) {
					sol::error err = result;
					UF_MSG_ERROR("{}", err.what());
					return;
				}
			#endif
				return;
			}

			auto decoded = ext::lua::decode( json );
			if ( !decoded ) return;
			sol::table table = decoded.value();
			auto result = fun( table );
		#if UF_LUA_PCALLS
			if ( !result.valid() ) {
				sol::error err = result;
				UF_MSG_ERROR("{}", err.what());
				return;
			}
		#endif
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
	void queueHook( uf::Object& self, const uf::stl::string& name, sol::table table, sol::optional<float> delay ) {
		ext::json::Value payload = uf::Serializer(table);
		self.queueHook( name, payload, delay.value_or(0.0f) );
	}
	uf::stl::string toString( uf::Object& self ) {
		return uf::string::toString( self );
	}

	size_t getUid( const uf::Object& o ) { return o.getUid(); }
	uf::stl::string getName( const uf::Object& o ) { return o.getName(); }
}


UF_LUA_REGISTER_USERTYPE(uf::Object,
	sol::call_constructor, sol::initializers(
		[]( uf::Object& self, sol::object arg, sol::optional<bool> init ){
			if ( arg.is<uf::stl::string>() ) {
				self.load( arg.as<uf::stl::string>() );
			} else if ( arg.is<sol::table>() ) {
				auto encoded = ext::lua::encode( arg.as<sol::table>() );
				if ( encoded ) {
					uf::Serializer json = encoded.value();
					self.load(json);
				}
			}
			if ( init.value_or(true) ) self.initialize();
		}
	),
	UF_LUA_REGISTER_USERTYPE_DEFINE( uid, UF_LUA_C_FUN(::binds::getUid) ),
	UF_LUA_REGISTER_USERTYPE_DEFINE( name, UF_LUA_C_FUN(::binds::getName) ),
	UF_LUA_REGISTER_USERTYPE_DEFINE( formatHookName, UF_LUA_C_FUN(::binds::formatHookName) ), 
	//UF_LUA_REGISTER_USERTYPE_DEFINE( getComponent, UF_LUA_C_FUN(::binds::getComponentFromEnum) ),
	//UF_LUA_REGISTER_USERTYPE_DEFINE( getComponent, UF_LUA_C_FUN(::binds::getComponentFromString) ),
	//UF_LUA_REGISTER_USERTYPE_DEFINE( setComponent, UF_LUA_C_FUN(::binds::setComponent) ),
	UF_LUA_REGISTER_USERTYPE_DEFINE( getComponent, UF_LUA_C_FUN(::binds::getComponent) ),
	UF_LUA_REGISTER_USERTYPE_DEFINE( bind, UF_LUA_C_FUN(::binds::bind) ),
	UF_LUA_REGISTER_USERTYPE_DEFINE( findByUid, UF_LUA_C_FUN(::binds::findByUid) ),
	UF_LUA_REGISTER_USERTYPE_DEFINE( findByName, UF_LUA_C_FUN(::binds::findByName) ),
	UF_LUA_REGISTER_USERTYPE_DEFINE( globalFindByUid, UF_LUA_C_FUN(::binds::globalFindByUid) ),
	UF_LUA_REGISTER_USERTYPE_DEFINE( globalFindByName, UF_LUA_C_FUN(::binds::globalFindByName) ),
	UF_LUA_REGISTER_USERTYPE_DEFINE( createChild, UF_LUA_C_FUN(::binds::createChild) ),
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