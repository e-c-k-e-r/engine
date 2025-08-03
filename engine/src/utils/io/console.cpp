#include <uf/utils/io/fmt.h>
#include <uf/utils/io/console.h>
#include <uf/utils/hook/hook.h>
#include <uf/ext/ext.h>

#include <uf/engine/entity/entity.h>
#include <uf/engine/scene/scene.h>
#include <uf/utils/math/transform.h>

uf::stl::unordered_map<uf::stl::string, uf::console::Command> uf::console::commands;
uf::stl::vector<uf::stl::string> uf::console::log;
uf::stl::vector<uf::stl::string> uf::console::history;

void uf::console::initialize() {
	uf::console::registerCommand("clear", "Clears the console of messages", [&]()->uf::stl::string{
		uf::console::clear();
		return "";
	});
	uf::console::registerCommand("quit", "Quits the game", [&]()->uf::stl::string{
		uf::hooks.call("system:Quit");
		return "";
	});
	
	uf::console::registerCommand("help", "Prints a list of commands and a description of what they do", [&]( const uf::stl::string& name )->uf::stl::string{
		if ( name == "" ) {
			uf::stl::vector<uf::stl::string> outputs = {"List of commands:"};
			for ( auto& pair : uf::console::commands ) {
				outputs.emplace_back(pair.first + ": " + pair.second.description);
			}
			return uf::string::join( outputs, "\n");
		}

		if ( uf::console::commands.count( name ) == 0 ) {
			return "Unknown command: " + name;
		}

		return name + ": " + uf::console::commands[name].description;
	});

	uf::console::registerCommand("callHook", "Calls a hook, passing the arguments as a JSON object", [&]( const uf::stl::string& arguments )->uf::stl::string{
		auto match = uf::string::match( arguments, "/^\"?(.+?)\"?(?: (.+?))?$/" );
		if ( match.empty() ) return "invalid invocation";

		uf::stl::vector<pod::Hook::userdata_t> results;
		if ( match.size() > 2 ) {
			ext::json::Value json;
			ext::json::decode( json, match[2] );

			results = uf::hooks.call( match[1], json );
		} else {
			results = uf::hooks.call( match[1] );
		}

		// this could probably be its own function
		uf::stl::string s_result = "";
		for ( auto i = 0; i < results.size(); ++i ) {
			auto& res = results[i];
			if ( res.is<uf::stl::string>() ) s_result += ::fmt::format("\n[{}] => {}", i, res.as<uf::stl::string>());
			else if ( res.is<ext::json::Value>() ) s_result += ::fmt::format("\n[{}] => {}", i, ext::json::encode( res.as<ext::json::Value>() ));
			else s_result += ::fmt::format("\n[{}] => Userdata: {}", i, (void*) res);
		}

		return "Hook executed: " + match[1] + s_result;
	});
	
	uf::console::registerCommand("json", "Modifies the gamestate by setting a JSON value", [&]( const uf::stl::string& arguments )->uf::stl::string{
		auto match = uf::string::match( arguments, "/^(.+?) *= *(.+?)$/" );
		if ( match.empty() ) {
			uf::Serializer target = ext::config;
			return ext::json::encode( arguments == "" ? target : target.path( arguments ), {
				.pretty = true
			} );
		}
		
		uf::stl::string keyString = match[1];
		uf::stl::string valueString = match[2];

		uf::Serializer value;
		value.deserialize(valueString);

		ext::config.path(keyString) = value;
		ext::load( ext::config );

		return "Value `" + keyString + "` set to `" + ext::json::encode( value ) + "`";
	});

	uf::console::registerCommand("scene", "Prints the scene graph", [&]( const uf::stl::string& arguments )->uf::stl::string{
		uf::stl::string res;

		std::function<void(uf::Entity*, int)> filter = [&]( uf::Entity* entity, int indent ) {
			for ( int i = 0; i < indent; ++i ) res += "\t";
			res += uf::string::toString(entity->as<uf::Object>()) + " ";
			if ( entity->hasComponent<pod::Transform<>>() ) {
				pod::Transform<> t = uf::transform::flatten(entity->getComponent<pod::Transform<>>());
				res += uf::string::toString(t.position) + " " + uf::string::toString(t.orientation);
			}
			res += "\n";
		};
		for ( uf::Scene* scene : uf::scene::scenes ) {
			if ( !scene ) continue;
			res += ::fmt::format("Scene: {}: {}\n", scene->getName(), scene->getUid());
			scene->process(filter, 1);
		}

		return res;
	});

	uf::console::registerCommand("entity", "Modifies the gamestate by setting a JSON value for an entity", [&]( const uf::stl::string& arguments )->uf::stl::string{
		auto match = uf::string::match( arguments, "/^(\\d+)/" );
		if ( match.empty() ) return "invalid invocation";
		
		uf::stl::string IDstring = match[1];
		size_t ID = std::stoi( IDstring );
		uf::Object* entity = (uf::Object*) uf::Entity::globalFindByUid( ID );
		if ( !entity ) return "entity not found: " + IDstring;
		entity->callHook( "object:Serialize.%UID%" );
		auto& metadata = entity->getComponent<uf::Serializer>();

		match = uf::string::match( arguments, "/^\\d+ (.+?) *= *(.+?)$/" );
		if ( match.empty() ) {
			match = uf::string::match( arguments, "/^\\d+ (.+?)$/" );
			uf::Serializer target = metadata;
			return ext::json::encode( !match.empty() && match[1] != "" ? target.path( match[1] ) : target, {
				.pretty = true
			} );
		}

		uf::stl::string keyString = match[1];
		uf::stl::string valueString = match[2];

		uf::Serializer value;
		value.deserialize(valueString);

		metadata.path(keyString) = value;
		entity->callHook( "object:Deserialize.%UID%" );

		return "Value `" + keyString + "` set to `" + ext::json::encode( value ) + "`";
	});
}

void uf::console::clear() {
	uf::console::log.clear();
}
void uf::console::print( const uf::stl::string& str ) {
	uf::console::log.emplace_back( str );
}
uf::stl::string uf::console::execute( const uf::stl::string& input ) {
	uf::console::history.emplace_back( input );
	uf::console::print("> " + input);

	uf::stl::string output;
	auto match = uf::string::match( input, "/^(.+?)(?: (.+?))?$/" );
	if ( !match.empty() ) {
		uf::stl::string command = match[1];
		uf::stl::string arguments = match.size() > 2 ? match[2] : "";
		output = uf::console::execute( command, arguments );
	} else {
		output = "Unknown command: " + input;
	}

	uf::console::print("< " + output);

	return output;
}
uf::stl::string uf::console::execute( const uf::stl::string& command, const uf::stl::string& arguments ) {
	if ( uf::console::commands.count( command ) == 0 ) return "Unknown command: " + command;
	return uf::console::commands[command].callback( arguments );
}

// callback( ["arg1", "arg2"] )
void uf::console::registerCommand( const uf::stl::string& name, const uf::stl::string& description, const std::function<uf::stl::string(const uf::stl::vector<uf::stl::string>&)>& callback ) {
	if ( uf::console::commands.count( name ) > 0 ) {
		UF_MSG_ERROR("Command already registered: {}", name);
		return;
	}
	uf::console::commands[name] = {
		.description = description,
		.callback = [callback](const uf::stl::string& arguments)->uf::stl::string{
			return callback( uf::string::split( arguments, " " ) );
		},
	};
}
// callback( "arg1 arg2" )
void uf::console::registerCommand( const uf::stl::string& name, const uf::stl::string& description, const std::function<uf::stl::string(const uf::stl::string&)>& callback ) {
	if ( uf::console::commands.count( name ) > 0 ) {
		UF_MSG_ERROR("Command already registered: {}", name);
		return;
	}
	uf::console::commands[name] = {
		.description = description,
		.callback = callback,
	};
}
// callback()
void uf::console::registerCommand( const uf::stl::string& name, const uf::stl::string& description, const std::function<uf::stl::string()>& callback ) {
	if ( uf::console::commands.count( name ) > 0 ) {
		UF_MSG_ERROR("Command already registered: {}", name);
		return;
	}
	uf::console::commands[name] = {
		.description = description,
		.callback = [callback](const uf::stl::string&)->uf::stl::string{
			return callback();
		},
	};
}