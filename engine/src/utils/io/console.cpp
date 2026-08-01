#include <uf/utils/io/fmt.h>
#include <uf/utils/io/console.h>
#include <uf/utils/hook/hook.h>

#include <uf/engine/ext.h>

#include <uf/engine/entity/entity.h>
#include <uf/engine/scene/scene.h>
#include <uf/utils/math/transform.h>
#include <uf/utils/io/fmt.h>

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
		if ( arguments.empty() ) return "invalid invocation";

		uf::stl::string hookName;
		uf::stl::string jsonArgs;
		size_t spaceIdx = uf::stl::string::npos;

		// quoted hook names
		if ( arguments[0] == '"' ) {
			size_t endQuote = arguments.find('"', 1);
			if ( endQuote != uf::stl::string::npos ) {
				hookName = arguments.substr(1, endQuote - 1);
				spaceIdx = arguments.find_first_not_of(' ', endQuote + 1);
				if ( spaceIdx != uf::stl::string::npos ) jsonArgs = arguments.substr(spaceIdx);
			}
		} else {
			// unquoted hook names
			spaceIdx = arguments.find(' ');
			if ( spaceIdx != uf::stl::string::npos ) {
				hookName = arguments.substr(0, spaceIdx);
				size_t nextChar = arguments.find_first_not_of(' ', spaceIdx);
				if ( nextChar != uf::stl::string::npos ) jsonArgs = arguments.substr(nextChar);
			} else {
				hookName = arguments;
			}
		}

		uf::stl::vector<pod::Hook::userdata_t> results;
		if ( !jsonArgs.empty() ) {
			ext::json::Value json;
			ext::json::decode( json, jsonArgs );
			results = uf::hooks.call( hookName, json );
		} else {
			results = uf::hooks.call( hookName );
		}

		uf::stl::string s_result = "";
		for ( auto i = 0; i < results.size(); ++i ) {
			auto& res = results[i];
			if ( res.is<uf::stl::string>() ) s_result += FMT_FORMAT("\n[{}] => {}", i, res.as<uf::stl::string>());
			else if ( res.is<ext::json::Value>() ) s_result += FMT_FORMAT("\n[{}] => {}", i, ext::json::encode( res.as<ext::json::Value>() ));
			else s_result += FMT_FORMAT("\n[{}] => Userdata: {}", i, (void*) res);
		}

		return "Hook executed: " + hookName + s_result;
	});
	
	uf::console::registerCommand("json", "Modifies the gamestate by setting a JSON value", [&]( const uf::stl::string& arguments )->uf::stl::string{
		size_t eqPos = arguments.find('=');

		// query
		if ( eqPos == uf::stl::string::npos ) {
			uf::Serializer target = uf::config;
			uf::stl::string query = arguments;
			query.erase(query.find_last_not_of(" \t") + 1);

			return ext::json::encode( query == "" ? target : target.path( query ), {
				.pretty = true
			} );
		}

		// set mode
		uf::stl::string keyString = arguments.substr(0, eqPos);
		keyString.erase(keyString.find_last_not_of(" \t") + 1);

		uf::stl::string valueString = arguments.substr(eqPos + 1);
		valueString.erase(0, valueString.find_first_not_of(" \t"));

		uf::Serializer value;
		value.deserialize(valueString);

		uf::config.path(keyString) = value;
		uf::load( uf::config );

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
			res += FMT_FORMAT("Scene: {}: {}\n", scene->getName(), scene->getUid());
			scene->process(filter, 1);
		}

		return res;
	});

	uf::console::registerCommand("entity", "Modifies the gamestate by setting a JSON value for an entity", [&]( const uf::stl::string& arguments )->uf::stl::string{
		size_t firstSpace = arguments.find(' ');
		uf::stl::string IDstring = arguments.substr(0, firstSpace);
		if ( IDstring.empty() ) return "invalid invocation";

		size_t ID = std::stoi( IDstring );
		uf::Object* entity = (uf::Object*) uf::Entity::globalFindByUid( ID );
		if ( !entity ) return "entity not found: " + IDstring;

		entity->callHook( "object:Serialize.%UID%" );
		auto& metadata = entity->getComponent<uf::Serializer>();

		// only ID provided
		if ( firstSpace == uf::stl::string::npos ) {
			uf::Serializer target = metadata;
			return ext::json::encode( target, { .pretty = true } );
		}

		uf::stl::string remainder = arguments.substr(firstSpace + 1);
		remainder.erase(0, remainder.find_first_not_of(" \t"));

		size_t eqPos = remainder.find('=');

		// query mode
		if ( eqPos == uf::stl::string::npos ) {
			uf::Serializer target = metadata;
			remainder.erase(remainder.find_last_not_of(" \t") + 1);
			return ext::json::encode( remainder != "" ? target.path( remainder ) : target, { .pretty = true } );
		}

		// set mode
		uf::stl::string keyString = remainder.substr(0, eqPos);
		keyString.erase(keyString.find_last_not_of(" \t") + 1);

		uf::stl::string valueString = remainder.substr(eqPos + 1);
		valueString.erase(0, valueString.find_first_not_of(" \t"));

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

	size_t firstChar = input.find_first_not_of(" \t");

	if ( firstChar != uf::stl::string::npos ) {
		size_t spacePos = input.find(' ', firstChar);

		uf::stl::string command;
		uf::stl::string arguments = "";

		if ( spacePos != uf::stl::string::npos ) {
			command = input.substr(firstChar, spacePos - firstChar);

			size_t argStart = input.find_first_not_of(" \t", spacePos);
			if ( argStart != uf::stl::string::npos ) {
				arguments = input.substr(argStart);
			}
		} else {
			command = input.substr(firstChar);
		}

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