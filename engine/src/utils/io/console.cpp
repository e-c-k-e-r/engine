#include <uf/utils/io/fmt.h>
#include <uf/utils/io/console.h>
#include <uf/utils/io/socket.h>
#include <uf/utils/hook/hook.h>

#include <uf/engine/ext.h>

#include <uf/engine/entity/entity.h>
#include <uf/engine/scene/scene.h>
#include <uf/utils/math/transform.h>
#include <uf/utils/io/fmt.h>
#include <uf/utils/io/iostream.h>
#include <uf/utils/io/file.h>
#include <uf/utils/io/inputs.h>
#include <uf/utils/time/time.h>
#include <uf/utils/string/ext.h>
#include <uf/utils/window/payloads.h>
#include <uf/utils/window/window.h>
#include <uf/utils/renderer/renderer.h>
#include <uf/utils/image/image.h>
#include <uf/ext/json/json.h>
#if UF_USE_LUA
#include <uf/ext/lua/lua.h>
#endif

#include <atomic>
#include <mutex>
#include <thread>
#include <iostream>
#include <fstream>
#include <functional>
#include <ctime>
#include <cstdlib>
#include <cctype>
#if UF_ENV_LINUX
	// to-do: for windows too
	#include <poll.h>
#endif
#include <unistd.h>
#include <errno.h>

uf::stl::unordered_map<uf::stl::string, uf::console::Command> uf::console::commands;
uf::stl::vector<uf::stl::string> uf::console::log;
uf::stl::vector<uf::stl::string> uf::console::history;

namespace {
	struct QueuedLine {
		size_t transport;
		uf::stl::string line;
	};
	struct Session {
		std::atomic<bool> running = false;
		bool eof = false;
		bool eofAnnounced = false;
		uint64_t commandId = 0;
		std::mutex mutex;
		uf::stl::vector<QueuedLine> queue;
	};
	Session* session = nullptr; // intentionally leaked so the reader thread never touches freed memory

	uf::stl::string trimmed( const uf::stl::string& str ) {
		size_t start = str.find_first_not_of( " \t" );
		if ( start == uf::stl::string::npos ) return "";
		return str.substr( start, str.find_last_not_of( " \t" ) - start + 1 );
	}

	bool commandFailed( const uf::stl::string& output ) {
		static const uf::stl::vector<uf::stl::string> failurePrefixes = { "Unknown command:", "Lua error:", "invalid invocation", "error:" };
		for ( auto& prefix : failurePrefixes ) if ( output.starts_with( prefix ) ) return true;
		return false;
	}

#if UF_USE_LUA
	constexpr int luaJsonMaxDepth = 4;
	constexpr size_t luaJsonMaxChildren = 64;

	ext::json::Value luaNodeToJson( sol::object object, int depth, uf::stl::vector<const void*>& lineage ) {
		switch ( object.get_type() ) {
			case sol::type::boolean: return ext::json::Value( object.as<bool>() );
			case sol::type::number: return ext::json::Value( object.as<double>() );
			case sol::type::string: return ext::json::Value( object.as<uf::stl::string>() );
			case sol::type::table: {
				sol::table table = object.as<sol::table>();
				if ( depth >= luaJsonMaxDepth ) return ext::json::Value( "<table depth-limit>" );
				const void* id = table.pointer();
				for ( auto* seen : lineage ) if ( seen == id ) return ext::json::Value( "<table cycle>" );
				lineage.emplace_back( id );

				// array when keys are exactly 1..n, object of string keys otherwise (same dance as lua.cpp's encodeNode)
				ext::json::Value json;
				bool isArray = true;
				size_t expectedIndex = 1;
				for ( auto& kv : table ) {
					if ( kv.first.get_type() != sol::type::number || kv.first.as<size_t>() != expectedIndex++ ) { isArray = false; break; }
				}
				size_t emitted = 0;
				if ( isArray ) {
					for ( auto& kv : table ) {
						if ( emitted++ >= luaJsonMaxChildren ) { json.emplace_back( "<children truncated>" ); break; }
						json.emplace_back( luaNodeToJson( kv.second, depth + 1, lineage ) );
					}
				} else {
					for ( auto& kv : table ) {
						if ( emitted++ >= luaJsonMaxChildren ) { json["<children>"] = "<children truncated>"; break; }
						if ( kv.first.get_type() != sol::type::string ) continue;
						json[ kv.first.as<uf::stl::string>() ] = luaNodeToJson( kv.second, depth + 1, lineage );
					}
				}

				lineage.pop_back();
				return json;
			} break;
			default: return ext::json::Value();
		}
	}
	uf::stl::string luaObjectToString( sol::object object ) {
		switch ( object.get_type() ) {
			case sol::type::boolean: return object.as<bool>() ? "true" : "false";
			case sol::type::number: return FMT_FORMAT( "{}", object.as<double>() ); // fmt shortens 42.0 to "42"
			case sol::type::string: return object.as<uf::stl::string>();
			case sol::type::table: {
				uf::stl::vector<const void*> lineage;
				return ext::json::encode( luaNodeToJson( object, 0, lineage ) );
			} break;
			default: return "<unsupported>";
		}
	}

	constexpr int rpcJsonMaxDepth = 8;
	constexpr size_t rpcMaxArgs = 64;

	sol::object jsonToLua( const ext::json::Value& json, int depth ) {
		if ( json.is_null() ) return sol::make_object( ext::lua::state, sol::lua_nil );
		if ( json.is_boolean() ) return sol::make_object( ext::lua::state, json.as<bool>() );
		if ( json.is_number() ) return sol::make_object( ext::lua::state, json.as<double>() );
		if ( json.is_string() ) return sol::make_object( ext::lua::state, json.as<uf::stl::string>() );
		if ( depth >= rpcJsonMaxDepth ) return sol::make_object( ext::lua::state, "<json depth-limit>" );
		if ( json.is_array() ) {
			sol::table table = ext::lua::state.create_table();
			ext::json::forEach( json, [ & ]( size_t index, const ext::json::Value& value ) { table[ index + 1 ] = jsonToLua( value, depth + 1 ); } );
			return table;
		}
		if ( json.is_object() ) {
			sol::table table = ext::lua::state.create_table();
			ext::json::forEach( json, [ & ]( const uf::stl::string& key, const ext::json::Value& value ) { table[ key ] = jsonToLua( value, depth + 1 ); } );
			return table;
		}
		return sol::make_object( ext::lua::state, sol::lua_nil );
	}

	sol::protected_function rpcInvoker() {
		static sol::protected_function invoker = [ & ]() -> sol::protected_function {
			auto loaded = ext::lua::state.safe_script( "local spread = table.unpack or unpack\nreturn function(fn, args) return fn( spread( args, 1, args.n ) ) end", sol::script_pass_on_error );
			if ( !loaded.valid() ) return sol::protected_function();
			return sol::protected_function( loaded );
		}();
		return invoker;
	}
#endif

	// runs a snippet through the lua state and marshals the first returned value into the response
	uf::stl::string runLua( const uf::stl::string& code ) {
		if ( code.empty() ) return "invalid invocation: lua '<code>'";
	#if UF_USE_LUA
		auto result = ext::lua::state.safe_script( code, sol::script_pass_on_error );
		if ( !result.valid() ) {
			sol::error err = result;
			return "Lua error: " + uf::stl::string( err.what() );
		}
		if ( result.return_count() < 1 ) return "Lua executed";
		sol::object returned = result; // first return value; same conversion lua.cpp's hook binds use
		if ( returned.get_type() == sol::type::lua_nil ) return "Lua executed";
		return luaObjectToString( returned );
	#else
		return "lua is not available in this build";
	#endif
	}

	uf::stl::string dumpSceneJson() {
		ext::json::Value scenesJson = ext::json::array();
		bool truncated = false;
		size_t listed = 0;
		for ( uf::Scene* scene : uf::scene::scenes ) {
			if ( !scene ) continue;
			ext::json::Value sceneJson;
			sceneJson["name"] = scene->getName();
			ext::json::Value entitiesJson = ext::json::array();
			std::function<void( uf::Entity*, int )> visit = [&]( uf::Entity* entity, int depth ) {
				if ( listed >= 200 ) { truncated = true; return; }
				++listed;
				ext::json::Value entityJson;
				entityJson["name"] = uf::string::toString( *entity );
				entityJson["depth"] = (double)depth;
				if ( entity->hasComponent<pod::Transform<>>() ) {
					pod::Transform<> t = uf::transform::flatten( entity->getComponent<pod::Transform<>>() );
					entityJson["location"] = uf::string::toString( t.position ) + " " + uf::string::toString( t.orientation );
				}
				entitiesJson.emplace_back( entityJson );
			};
			scene->process( visit, 1 );
			sceneJson["entities"] = entitiesJson;
			scenesJson.emplace_back( sceneJson );
		}
		ext::json::Value dump;
		dump["currentScene"] = uf::scene::scenes.empty() ? "none" : uf::scene::getCurrentScene().getName();
		dump["scenes"] = scenesJson;
		dump["truncated"] = truncated;
		return ext::json::encode( dump );
	}

	// push event stream
	uf::stl::vector<uf::stl::string> watchPatterns;
	uf::stl::unordered_map<uf::hashed_string, uf::stl::string> hookNameTexts;
	bool pushingEvents = false; // re-entrancy guard

	void internHookName( const uf::stl::string& name ) {
		hookNameTexts[ name ] = name;
	}

	bool globMatch( const uf::stl::string_view& pattern, const uf::stl::string_view& text ) { // '*' wildcard only
		size_t p = 0, t = 0, star = uf::stl::string::npos, mark = 0;
		while ( t < text.size() ) {
			if ( p < pattern.size() && ( pattern[ p ] == '?' || pattern[ p ] == text[ t ] ) ) { ++p; ++t; }
			else if ( p < pattern.size() && pattern[ p ] == '*' ) { star = p++; mark = t; }
			else if ( star != uf::stl::string::npos ) { p = star + 1; t = ++mark; }
			else return false;
		}
		while ( p < pattern.size() && pattern[ p ] == '*' ) ++p;
		return p == pattern.size();
	}

	uf::stl::string buildEventPush( const uf::stl::string& name, const pod::Hook::userdata_t& payload ) {
		auto typed = [ & ]( auto* tag ) -> bool {
			using T = std::remove_pointer_t< decltype(tag) >;
			return payload.type() == UF_USERDATA_CTTI( T ) && payload.size() == sizeof( T );
		};
		ext::json::Value event;
		event["event"] = name;
		event["frame"] = (uint64_t)uf::time::frame;
		event["time"] = (double)uf::time::current;
		if ( payload.size() > 0 ) {
			if ( typed( (pod::payloads::windowResized*)nullptr ) ) {
				const auto& data = payload.get<pod::payloads::windowResized>( false );
				event["width"] = (uint64_t)data.window.size.x;
				event["height"] = (uint64_t)data.window.size.y;
			}
			else if ( typed( (pod::payloads::windowFocusedChanged*)nullptr ) ) {
				const auto& data = payload.get<pod::payloads::windowFocusedChanged>( false );
				event["state"] = (int64_t)data.window.state;
			}
			else if ( typed( (pod::payloads::windowMouseCursorVisibility*)nullptr ) ) {
				const auto& data = payload.get<pod::payloads::windowMouseCursorVisibility>( false );
				event["visible"] = data.mouse.visible;
			}
			else if ( typed( (ext::json::Value*)nullptr ) ) {
				event["payload"] = payload.get<ext::json::Value>( false );
			}
		}
		return ext::json::encode( event );
	}

	void pushEvent( const uf::stl::string& name, const pod::Hook::userdata_t& payload ) {
		if ( pushingEvents ) return;
		pushingEvents = true;
		uf::stl::string json = buildEventPush( name, payload );
		UF_MSG_INFO( "@ {}", json );
		uf::io::socket::broadcast( FMT_FORMAT( "@ {}\n", json ) );
		pushingEvents = false;
	}

	void dispatchLine( const QueuedLine& entry ) {
		auto output = uf::console::execute( entry.line );
		if ( !output.empty() ) UF_MSG_INFO("Console: {}", output);
		
		// machine-parseable reply
		ext::json::Value reply;
		reply["ok"] = !commandFailed( output );
		reply["output"] = output;
		uint64_t id = ++session->commandId;
		if ( entry.transport == 0 ) {
			UF_MSG_INFO("#{} {}", id, ext::json::encode( reply ) );
		}
		else uf::io::socket::send( entry.transport - 1, FMT_FORMAT( "#{} {}\n", id, ext::json::encode( reply ) ) );
	}

	// registered from uf::console::initialize() when uf::headless (names/descriptions/replies are the old client-side ones)
	void initializeHeadlessCommands() {
		uf::console::registerCommand( "lua", "Executes a Lua snippet in the engine state; responds with the first returned value", [&]( const uf::stl::string& code ) -> uf::stl::string {
			return runLua( code );
		});
		// to-do: should probably instead map through the VFS (requires the interfacer to mount its scratchspace as a VFS first)
		uf::console::registerCommand( "exec", "Runs a Lua file from the raw filesystem", [&]( const uf::stl::string& arguments ) -> uf::stl::string {
			uf::stl::string path = trimmed( arguments );
			if ( path.empty() ) return "invalid invocation: exec <file>";

			std::ifstream file( path, std::ios::binary );
			if ( !file ) return "error: cannot read file: " + path;
			file.seekg( 0, std::ios::end );
			std::streamoff size = file.tellg();
			file.seekg( 0, std::ios::beg );
			uf::stl::string code;
			code.resize( (size_t)size );
			file.read( code.data(), size );
			if ( code.empty() ) return "error: file is empty: " + path;
			return runLua( code );
		});

		uf::console::registerCommand( "status", "Engine status readout (key=value)", [&]()->uf::stl::string{
			static size_t lastFrame = 0;
			static double lastTime = 0;
			double fps = uf::time::delta > 0 ? 1.0 / uf::time::delta : 0;
			double msPerFrame = uf::time::delta * 1000.0;
			if ( lastTime > 0 && uf::time::current > lastTime && uf::time::frame > lastFrame ) {
				double elapsed = uf::time::current - lastTime;
				double frames = (double)( uf::time::frame - lastFrame );
				fps = frames / elapsed;
				msPerFrame = elapsed / frames * 1000.0;
			}
			lastFrame = uf::time::frame;
			lastTime = uf::time::current;

			size_t entityCount = 0;
			for ( uf::Scene* scene : uf::scene::scenes ) if ( scene ) entityCount += scene->getGraph().size();
			uf::stl::string sceneName = uf::scene::scenes.empty() ? "none" : uf::scene::getCurrentScene().getName();

			uf::stl::string renderModeNames;
			for ( auto* renderMode : uf::renderer::renderModes ) {
				if ( !renderModeNames.empty() ) renderModeNames += ",";
				renderModeNames += renderMode->getName();
			}
			if ( renderModeNames.empty() ) renderModeNames = "none";

			uf::stl::string state = "running";
			if ( uf::paused ) state = uf::stepBudget > 0 ? FMT_FORMAT( "stepping={}", uf::stepBudget ) : "paused";

		#if UF_USE_VULKAN
			return FMT_FORMAT( "frame={} fps={:.2f} ms/frame={:.3f} entities={} scene={} renderModes={} surfaceless={} state={}", uf::time::frame, fps, msPerFrame, entityCount, sceneName, renderModeNames, uf::renderer::device.surfaceless, state );
		#else
			return FMT_FORMAT( "frame={} fps={:.2f} ms/frame={:.3f} entities={} scene={} renderModes={} state={}", uf::time::frame, fps, msPerFrame, entityCount, sceneName, renderModeNames, state );
		#endif
		});

		// time control
		uf::console::registerCommand( "pause", "Freezes simulation & rendering (I/O stays responsive); 'step' or 'resume' to continue", [&]()->uf::stl::string{
			uf::paused = true;
			return "paused";
		});
		uf::console::registerCommand( "resume", "Resumes simulation & rendering after pause/step", [&]()->uf::stl::string{
			uf::paused = false;
			uf::stepBudget = 0;
			return "running";
		});
		uf::console::registerCommand( "step", "Advances n frames (default 1) while paused, then re-pauses", [&]( const uf::stl::string& arguments ) -> uf::stl::string {
			uint32_t frames = 1;
			uf::stl::string arg = trimmed( arguments );
			if ( !arg.empty() ) {
				uint64_t parsed = 0;
				for ( char c : arg ) {
					if ( c < '0' || c > '9' ) return "invalid invocation: step [n]";
					parsed = parsed * 10 + ( c - '0' );
					if ( parsed > 1000000 ) return "invalid invocation: step [n]";
				}
				frames = (uint32_t)parsed;
			}
			uf::paused = true;
			uf::stepBudget = frames;
			return FMT_FORMAT( "stepping={}", frames );
		});
		uf::console::registerCommand( "speed", "Frame pacing multiplier vs refresh rate: 1.0 = 60fps-ish, 0 = unthrottled (fastest sim), 2 = 2x, 0.5 = half", [&]( const uf::stl::string& arguments ) -> uf::stl::string {
			uf::stl::string arg = trimmed( arguments );
			if ( arg.empty() ) return "invalid invocation: speed <multiplier>";
			char* end = nullptr;
			double multiplier = std::strtod( arg.c_str(), &end );
			if ( !end || *end != '\0' || multiplier < 0.0 ) return "invalid invocation: speed <multiplier>";
			size_t refreshRate = uf::config["window"]["refresh rate"].as<size_t>( 60 );
			uf::frameLimiter = multiplier > 0 ? 1.0 / ( (double)refreshRate * multiplier ) : 0.0f;
			return FMT_FORMAT( "limiter={:.6f} ({} fps target)", uf::frameLimiter, multiplier > 0 ? (double)refreshRate * multiplier : 0.0 );
		});

		uf::console::registerCommand( "entities", "Lists entities (name + location); entities [max], default 100", [&]( const uf::stl::string& arguments ) -> uf::stl::string {
			size_t cap = 100;
			uf::stl::string arg = trimmed( arguments );
			if ( !arg.empty() ) {
				size_t parsed = 0;
				for ( char c : arg ) {
					if ( c < '0' || c > '9' ) return "invalid invocation: entities [max]";
					parsed = parsed * 10 + ( c - '0' );
				}
				cap = parsed;
			}

			uf::stl::vector<uf::stl::string> lines;
			size_t total = 0;
			std::function<void( uf::Entity*, int )> visit = [&]( uf::Entity* entity, int depth ) {
				++total;
				if ( lines.size() >= cap ) return;
				uf::stl::string indent = ""; for ( auto i = 1; i < depth; ++i ) indent += "\t";
				uf::stl::string location = "";
				if ( entity->hasComponent<pod::Transform<>>() ) {
					pod::Transform<> t = uf::transform::flatten( entity->getComponent<pod::Transform<>>() );
					location = uf::string::toString( t.position ) + " " + uf::string::toString( t.orientation );
				}
				lines.emplace_back( FMT_FORMAT( "{}{} {}", indent, uf::string::toString( *entity ), location ) );
			};
			for ( uf::Scene* scene : uf::scene::scenes ) {
				if ( !scene ) continue;
				lines.emplace_back( FMT_FORMAT( "Scene: {}", scene->getName() ) );
				scene->process( visit, 1 );
			}
			if ( total > lines.size() ) lines.emplace_back( FMT_FORMAT( "truncated: {} of {} entities listed", lines.size(), total ) );
			return uf::string::join( lines, "\n" );
		});

		// the json dump supersedes the builtin graph dump when headless; registered after it, so a plain overwrite suffices
		uf::console::commands[ "scene" ] = {
			.description = "Dumps the current scene as JSON (scene names + entity names/locations, capped at 200 entities)",
			.callback = []( const uf::stl::string& ) -> uf::stl::string {
				return dumpSceneJson();
			},
		};

		// input injection
		uf::console::registerCommand( "key", "Sets a key state directly (key <name> down|up); persists until set again (no window input headless)", [&]( const uf::stl::string& arguments ) -> uf::stl::string {
			uf::stl::string arg = trimmed( arguments );
			size_t space = arg.find( ' ' );
			if ( space == uf::stl::string::npos ) return "invalid invocation: key <name> down|up";
			uf::stl::string name = trimmed( arg.substr( 0, space ) );
			uf::stl::string state = trimmed( arg.substr( space + 1 ) );
			for ( auto& c : name ) c = (char)std::tolower( (unsigned char)c );
			bool down;
			if ( state == "down" ) down = true;
			else if ( state == "up" ) down = false;
			else return "invalid invocation: key <name> down|up";

			// surely there's a better way to do this......
			static const uf::stl::vector<std::pair<uf::stl::string, uf::inputs::state_t*>> keys = [](){
				using namespace uf::inputs::kbm::states;
				uf::stl::vector<std::pair<uf::stl::string, uf::inputs::state_t*>> table;
				const uf::stl::string letters = "QWERTYUIOPASDFGHJKLZXCVBNM";
				uf::stl::vector<uf::inputs::state_t*> letterStates = { &Q,&W,&E,&R,&T,&Y,&U,&I,&O,&P,&A,&S,&D,&F,&G,&H,&J,&K,&L,&Z,&X,&C,&V,&B,&N,&M };
				for ( size_t i = 0; i < letterStates.size(); ++i ) table.emplace_back( uf::stl::string( 1, (char)std::tolower( (unsigned char)letters[i] ) ), letterStates[i] );
				const std::pair<uf::stl::string, uf::inputs::state_t*> named[] = {
					{ "left", &Left }, { "right", &Right }, { "up", &Up }, { "down", &Down },
					{ "space", &Space }, { "enter", &Enter }, { "tab", &Tab }, { "escape", &uf::inputs::kbm::states::Escape }, { "backspace", &BackSpace },
					{ "shift", &LShift }, { "rshift", &RShift }, { "alt", &LAlt }, { "ralt", &RAlt },
					{ "ctrl", &LControl }, { "rctrl", &RControl }, { "lshift", &LShift }, { "lalt", &LAlt }, { "lctrl", &LControl },
					{ "insert", &Insert }, { "home", &Home }, { "end", &End }, { "pageup", &PageUp }, { "pagedown", &PageDown }, { "delete", &Delete },
				};
				for ( auto& e : named ) table.emplace_back( e );
				uf::stl::vector<uf::inputs::state_t*> functionKeys = { &F1,&F2,&F3,&F4,&F5,&F6,&F7,&F8,&F9,&F10,&F11,&F12,&F13,&F14,&F15 };
				for ( size_t i = 0; i < functionKeys.size(); ++i ) table.emplace_back( FMT_FORMAT( "f{}", i + 1 ), functionKeys[i] );
				return table;
			}();
			for ( auto& entry : keys ) if ( entry.first == name ) {
				*entry.second = down;
				return FMT_FORMAT( "{}={}", name, down ? "down" : "up" );
			}
			return "error: unknown key: " + name;
		});

		// mouse injection
		uf::console::registerCommand( "mouse", "Injects mouse input: 'mouse button <left|right|middle> down|up' sets a button state (persists until set again, like key), 'mouse move <x> <y>' warps the window's mouse position", [&]( const uf::stl::string& arguments ) -> uf::stl::string {
			uf::stl::string arg = trimmed( arguments );
			size_t space = arg.find( ' ' );
			if ( space == uf::stl::string::npos ) return "invalid invocation: mouse button <left|right|middle> down|up | mouse move <x> <y>";
			uf::stl::string mode = trimmed( arg.substr( 0, space ) );
			uf::stl::string rest = trimmed( arg.substr( space + 1 ) );

			if ( mode == "button" ) {
				size_t cut = rest.find( ' ' );
				if ( cut == uf::stl::string::npos ) return "invalid invocation: mouse button <left|right|middle> down|up";
				uf::stl::string name = trimmed( rest.substr( 0, cut ) );
				uf::stl::string state = trimmed( rest.substr( cut + 1 ) );
				for ( auto& c : name ) c = (char)std::tolower( (unsigned char)c );
				bool down;
				if ( state == "down" ) down = true;
				else if ( state == "up" ) down = false;
				else return "invalid invocation: mouse button <left|right|middle> down|up";

				static const uf::stl::vector<std::pair<uf::stl::string, uf::inputs::state_t*>> buttons = {
					{ "left", &uf::inputs::kbm::states::Mouse1 }, { "right", &uf::inputs::kbm::states::Mouse2 }, { "middle", &uf::inputs::kbm::states::Mouse3 },
				};
				for ( auto& entry : buttons ) if ( entry.first == name ) {
					*entry.second = down;
					return FMT_FORMAT( "{}={}", name, down ? "down" : "up" );
				}
				return "error: unknown button: " + name;
			}

			if ( mode == "move" ) {
				size_t cut = rest.find( ' ' );
				if ( cut == uf::stl::string::npos ) return "invalid invocation: mouse move <x> <y>";
				auto parse = [ & ]( const uf::stl::string& token, int32_t& out ) -> bool {
					size_t i = ( !token.empty() && token.front() == '-' ) ? 1 : 0;
					if ( i >= token.size() ) return false;
					long long value = 0;
					for ( ; i < token.size(); ++i ) {
						if ( token[i] < '0' || token[i] > '9' ) return false;
						value = value * 10 + ( token[i] - '0' );
						if ( value > 2147483647 ) return false;
					}
					out = (int32_t)( token.front() == '-' ? -value : value );
					return true;
				};
				int32_t x = 0, y = 0;
				if ( !parse( trimmed( rest.substr( 0, cut ) ), x ) || !parse( trimmed( rest.substr( cut + 1 ) ), y ) ) return "invalid invocation: mouse move <x> <y>";
				if ( !uf::Window::get() ) return "error: no window"; // null window stores the position; getMousePosition reports it back
				uf::Window::get()->setMousePosition( { x, y } );
				auto position = uf::Window::get()->getMousePosition();
				return FMT_FORMAT( "mouse at {} {}", position.x, position.y );
			}

			return "invalid invocation: mouse button <left|right|middle> down|up | mouse move <x> <y>";
		});

	#if UF_USE_LUA
		// rpc registry
		{
			// to-do: migrate to ext::lua
			sol::table uf = ext::lua::state[ "uf" ].get_or_create<sol::table>();
			uf[ "rpc" ].get_or_create<sol::table>();
		}
		uf::console::registerCommand( "rpc", "Calls uf.rpc.<name> in the lua state (rpc <name> [jsonArray]); first return value marshalled like 'lua' does", [&]( const uf::stl::string& arguments ) -> uf::stl::string {
			uf::stl::string arg = trimmed( arguments );
			size_t space = arg.find( ' ' );
			uf::stl::string name = space == uf::stl::string::npos ? arg : trimmed( arg.substr( 0, space ) );
			uf::stl::string jsonArg = space == uf::stl::string::npos ? "" : trimmed( arg.substr( space + 1 ) );
			if ( name.empty() ) return "invalid invocation: rpc <name> [jsonArray]";
			if ( jsonArg.size() > uf::io::socket::maxLineBytes ) return "error: payload too large";

			ext::json::Value decoded;
			if ( !jsonArg.empty() ) {
				if ( jsonArg.front() != '[' ) return "invalid invocation: rpc <name> [jsonArray]";
				if ( !ext::json::Value::accept( jsonArg ) ) return "error: invalid json payload"; // decode() would abort on garbage (JSON_NOEXCEPTION)
				ext::json::decode( decoded, jsonArg );
			}
			const ext::json::Value& argsJson = decoded;
			if ( !jsonArg.empty() && !argsJson.is_array() ) return "invalid invocation: rpc <name> [jsonArray]";
			if ( argsJson.size() > rpcMaxArgs ) return FMT_FORMAT( "error: too many rpc arguments (max {})", rpcMaxArgs );

			sol::table registry = ext::lua::state[ "uf" ].get_or_create<sol::table>()[ "rpc" ].get_or_create<sol::table>();
			sol::object entry = registry.get<sol::object>( name );
			if ( entry.get_type() == sol::type::lua_nil ) return FMT_FORMAT( "error: no rpc '{}'", name );
			if ( entry.get_type() != sol::type::function ) return FMT_FORMAT( "error: rpc '{}' is not callable", name );

			sol::protected_function invoker = rpcInvoker();
			if ( !invoker.valid() ) return "error: rpc invoker unavailable (lua state not initialized)";

			sol::table args = ext::lua::state.create_table();
			size_t count = 0;
			ext::json::forEach( argsJson, [ & ]( size_t index, const ext::json::Value& value ) { args[ index + 1 ] = jsonToLua( value, 0 ); ++count; } );
			args[ "n" ] = count;

			auto result = invoker( entry, args );
			if ( !result.valid() ) {
				sol::error err = result;
				return "Lua error: " + uf::stl::string( err.what() );
			}
			if ( result.return_count() < 1 ) return "Lua executed";
			sol::object returned = result;
			if ( returned.get_type() == sol::type::lua_nil ) return "Lua executed";
			return luaObjectToString( returned );
		});
	#endif

		uf::console::registerCommand( "event", "Fires a hook (event <hookName> [json]); well-known window:* hooks get typed payloads, everything else receives the JSON object", [&]( const uf::stl::string& arguments ) -> uf::stl::string {
			uf::stl::string arg = trimmed( arguments );
			size_t space = arg.find( ' ' );
			uf::stl::string name = space == uf::stl::string::npos ? arg : trimmed( arg.substr( 0, space ) );
			uf::stl::string jsonArg = space == uf::stl::string::npos ? "" : trimmed( arg.substr( space + 1 ) );
			if ( name.empty() ) return "invalid invocation: event <hookName> [json]";
			internHookName( name ); // hash-keyed dispatch forgets the text; the push stream needs it
			if ( jsonArg.size() > uf::io::socket::maxLineBytes ) return "error: payload too large";

			ext::json::Value payloadJson;
			if ( !jsonArg.empty() ) {
				if ( !ext::json::Value::accept( jsonArg ) ) return "error: invalid json payload";
				ext::json::decode( payloadJson, jsonArg );
			}
			bool listeners = uf::hooks.exists( name );

			auto integer = []( const ext::json::Value& json, const char* key )->long long {
				return json.isObject() && json.contains( key ) ? json[key].as<int64_t>() : 0;
			};
			if ( name == "window:Resized" ) {
				pod::payloads::windowResized event;
				event.type = name;
				event.invoker = payloadJson.isObject() && payloadJson.contains( "invoker" ) ? payloadJson["invoker"].as<uf::stl::string>() : "agent";
				event.window.size.x = (decltype(event.window.size.x))integer( payloadJson, "width" );
				event.window.size.y = (decltype(event.window.size.y))integer( payloadJson, "height" );
				uf::hooks.call( name, event );
			} else if ( name == "window:Focus.Changed" ) {
				pod::payloads::windowFocusedChanged event;
				event.type = name;
				event.invoker = payloadJson.isObject() && payloadJson.contains( "invoker" ) ? payloadJson["invoker"].as<uf::stl::string>() : "agent";
				event.window.state = (int_fast8_t)integer( payloadJson, "state" );
				uf::hooks.call( name, event );
			} else if ( name == "window:Mouse.CursorVisibility" ) {
				pod::payloads::windowMouseCursorVisibility event;
				event.type = name;
				event.invoker = payloadJson.isObject() && payloadJson.contains( "invoker" ) ? payloadJson["invoker"].as<uf::stl::string>() : "agent";
				event.mouse.visible = payloadJson.isObject() && payloadJson.contains( "visible" ) ? payloadJson["visible"].as<bool>() : true;
				uf::hooks.call( name, event );
			} else {
				uf::hooks.call( name, payloadJson );
			}
			return listeners ? "ok" : "ok (no listeners)";
		});

		uf::console::registerCommand( "watch", "Streams matching hook fires to ALL socket clients as '@ {json}' lines (patterns: exact name or '*' glob, space separated; state is global); no args lists current watches", [&]( const uf::stl::string& arguments ) -> uf::stl::string {
			uf::stl::string arg = trimmed( arguments );
			if ( arg.empty() ) {
				if ( watchPatterns.empty() ) return "no watches";
				return "watching: " + uf::string::join( watchPatterns, " " );
			}
			size_t added = 0;
			size_t start = 0;
			while ( start <= arg.size() ) {
				size_t space = arg.find( ' ', start );
				uf::stl::string pattern = trimmed( arg.substr( start, space == uf::stl::string::npos ? uf::stl::string::npos : space - start ) );
				if ( !pattern.empty() ) {
					if ( pattern.find( '*' ) == uf::stl::string::npos ) internHookName( pattern );
					watchPatterns.emplace_back( pattern );
					++added;
				}
				if ( space == uf::stl::string::npos ) break;
				start = space + 1;
			}
			return added > 0 ? FMT_FORMAT( "watching {} pattern(s)", added ) : "invalid invocation: watch [pattern ...]";
		});
		uf::console::registerCommand( "unwatch", "Clears all watch patterns (stops event pushes)", [&]( ) -> uf::stl::string {
			size_t had = watchPatterns.size();
			watchPatterns.clear();
			return FMT_FORMAT( "cleared {} watch(es)", had );
		});

	#if UF_USE_VULKAN
		uf::console::registerCommand( "screenshot", "Saves a screenshot to [path] (default: {io root}/screenshots/{timestamp}.png)", [&]( const uf::stl::string& arguments ) -> uf::stl::string {
			if ( !uf::renderer::states::initialized ) return "error: renderer not initialized";
			uf::stl::string filename = trimmed( arguments );
			if ( filename.empty() ) {
				std::time_t t = std::time( nullptr );
				filename = FMT_FORMAT( "{}/screenshots/{:%Y-%m-%d_%H-%M-%S}.png", uf::io::root, ::fmt::localtime( t ) );
			}
			auto& renderMode = uf::renderer::getRenderMode( "Swapchain", true );
			auto image = renderMode.screenshot( 0 );
			if ( !image.save( filename ) ) return "error: failed to save screenshot: " + filename;
			return "screenshot saved: " + filename;
		});
	#endif

		session = new Session();
		session->running = true;
		// this could probably be done better (like tied to uf::threads)
	#if UF_ENV_LINUX
		std::thread( [](){
			int fd = STDIN_FILENO;
			bool connected = true;
			uf::stl::string partial;
			char chunk[4096];
			while ( session->running.load() ) {
				if ( connected ) {
					pollfd p = { fd, POLLIN, 0 };
					int r = poll( &p, 1, 100 );
					if ( r < 0 ) {
						if ( errno == EINTR ) continue;
						connected = false;
						continue;
					}
					if ( r == 0 ) continue; // timeout: re-check running
					if ( p.revents & POLLIN ) {
						ssize_t n = read( fd, chunk, sizeof(chunk) );
						if ( n > 0 ) {
							partial.append( chunk, static_cast<size_t>(n) );
							size_t nl;
							while ( ( nl = partial.find('\n') ) != uf::stl::string::npos ) {
								uf::stl::string line = partial.substr( 0, nl );
								partial.erase( 0, nl + 1 );
								while ( !line.empty() && ( line.back() == '\r' || line.back() == '\n' ) ) line.pop_back();
								if ( line.empty() ) continue;
								std::lock_guard<std::mutex> lock( session->mutex );
								session->queue.emplace_back( QueuedLine{ 0, line } );
							}
						}
						else if ( n == 0 ) connected = false; // EOF
						else if ( errno != EINTR && errno != EAGAIN && errno != EWOULDBLOCK ) connected = false;
					}
					else if ( p.revents & ( POLLHUP | POLLERR | POLLNVAL ) ) connected = false;
				}
				else {
					std::this_thread::sleep_for( std::chrono::milliseconds( 100 ) );
				}
			}
			if ( !connected ) {
				std::lock_guard<std::mutex> lock( session->mutex );
				session->eof = true;
			}
		}).detach();
	#endif

		uf::io::socket::onLine = []( size_t slot, const uf::stl::string& line ) {
			std::lock_guard<std::mutex> lock( session->mutex );
			session->queue.emplace_back( QueuedLine{ slot + 1, line } );
		};
		uf::io::socket::onDisconnect = []( size_t slot ) {
			++session->commandId;
			uf::io::socket::send( slot, FMT_FORMAT( "#{} {{\"ok\":false,\"output\":\"error: line too long\"}}\n", session->commandId ) );
		};
		bool socketStarted = false;
		if ( uf::socketRequested ) socketStarted = uf::io::socket::start( uf::socketPath );
		if ( socketStarted ) UF_MSG_INFO("Headless: command socket listening on {}", uf::io::socket::getPath() );

		// hook-call stream observer
		uf::hooks.callObserver = []( const uf::Hooks::name_t& name, const pod::Hook::userdata_t& payload ) {
			if ( watchPatterns.empty() ) return;
			auto found = hookNameTexts.find( name );
			if ( found == hookNameTexts.end() ) return;
			for ( auto& pattern : watchPatterns ) if ( globMatch( pattern, found->second ) ) { pushEvent( found->second, payload ); return; }
		};
		for ( const char* known : { "window:Resized", "window:Focus.Changed", "window:Mouse.CursorVisibility", "window:Title.Changed" } ) internHookName( known );

		if ( socketStarted ) {
			UF_MSG_INFO("Headless: command channels ready (stdin + {}; 'help' for a list)", uf::io::socket::getPath() );
		} else {
			UF_MSG_INFO("Headless: command channel ready (send command lines on stdin, 'help' for a list)");
		}
	}
}

void uf::console::initialize() {
	uf::console::registerCommand("clear", "Clears the console of messages", [&]()->uf::stl::string{
		uf::console::clear();
		return "";
	});
	uf::console::registerCommand("quit", "Quits the game", [&]()->uf::stl::string{
		// to-do: allow for empty arg'd call dispatches to also dispatch an empty JSON payload
		ext::json::Value payload;
		uf::hooks.call("system:Quit", payload);
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


	if ( uf::headless ) initializeHeadlessCommands();
}

void uf::console::pump() {
	if ( !session ) return;
	uf::io::socket::poll(); // no-op when no socket was requested; queues socket lines into the session

	uf::stl::vector<QueuedLine> lines;
	bool announceEOF = false;
	{
		std::lock_guard<std::mutex> lock( session->mutex );
		lines.swap( session->queue );
		if ( session->eof && !session->eofAnnounced ) {
			session->eofAnnounced = true;
			announceEOF = true;
		}
	}
	for ( auto& line : lines ) dispatchLine( line );
	if ( announceEOF ) {
		session->running = false;
		UF_MSG_INFO("Headless: stdin closed; stdin command channel disabled ('quit' or SIGTERM stops the engine)");
	}
}

void uf::console::terminate() {
	if ( session ) session->running = false;
	uf::io::socket::stop();
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
