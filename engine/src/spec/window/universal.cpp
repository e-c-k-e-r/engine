#include <uf/spec/window/window.h>
#include <uf/utils/hook/hook.h>
#include <uf/utils/io/iostream.h>

#include <uf/utils/serialize/serializer.h>
#include <sstream>

bool spec::uni::Window::focused = false;
/*
void spec::uni::Window::pushEvent( const uf::ReadableHook::name_t& name, const uf::ReadableHook::argument_t& argument ) {
	if ( !uf::hooks.prefersReadable() ) return;
	if ( !uf::hooks.exists(name) ) return;
	this->m_events.readable.push({name, argument});
}
void spec::uni::Window::pushEvent( const uf::OptimalHook::name_t& name, const uf::OptimalHook::argument_t& argument ) {
	if ( uf::hooks.prefersReadable() ) {
		uf::userdata::destroy(argument);
		return;
	}
	if ( !uf::hooks.exists(name) ) {
		uf::userdata::destroy(argument);
		return;
	}
	this->m_events.optimal.push({name});
	this->m_events.optimal.back().argument = argument;
}
void spec::uni::Window::pushEvent( const uf::ReadableHook::argument_t& serialized ) {
	if ( !uf::hooks.prefersReadable() ) return;
	uf::Serializer json = serialized;
	if ( !uf::hooks.exists(json["type"].as<uf::stl::string>()) ) return;
	this->m_events.readable.push({json["type"].as<uf::stl::string>(), serialized});
}
void spec::uni::Window::pushEvent( const uf::OptimalHook::argument_t& userdata ) {
	if ( uf::hooks.prefersReadable() ) return;
	// Header userdata
	struct Header {
		uf::stl::string type;
	};
//	Header header = userdata.get<Header>();
	const Header& header = uf::userdata::get<Header>(userdata);
	this->m_events.optimal.push({header.type});
	if ( !uf::hooks.exists(header.type) ) {
		uf::userdata::destroy(userdata);
		return;
	}
	this->m_events.optimal.back().argument = userdata;
}
*/
/*
void spec::uni::Window::pushEvent( const uf::Hooks::name_t& name, const uf::stl::string& payload ) {
	auto& event = this->m_events.emplace();
	event.name = name;
	event.payload.create<ext::json::Value>( uf::Serializer(payload) );
}
void spec::uni::Window::pushEvent( const uf::Hooks::name_t& name, const ext::json::Value& payload ) {
	auto& event = this->m_events.emplace();
	event.name = name;
	event.payload.create<ext::json::Value>( uf::Serializer(payload) );
}
void spec::uni::Window::pushEvent( const uf::Hooks::name_t& name, const uf::Serializer& payload ) {
	auto& event = this->m_events.emplace();
	event.name = name;
	event.payload.create<ext::json::Value>( uf::Serializer(payload) );
}
*/
void spec::uni::Window::pushEvent( const uf::Hooks::name_t& name, const pod::Hook::userdata_t& payload ) {
	auto& event = this->m_events.emplace();
	event.name = name;
	event.payload = payload;
}
/*
void spec::uni::Window::pushEvent( const pod::Hook::userdata_t& payload ) {
	struct Header {
		uf::stl::string type;
	};
	const Header& header = payload.get<Header>();
	if ( !uf::hooks.exists(header.type) ) return;
	this->m_events.push({ header.type, std::move(payload) });
}
*/
#if 0
void spec::uni::Window::processEvents() {
}
bool spec::uni::Window::isKeyPressed(const uf::stl::string& key) {
	return false;
}
bool spec::uni::Window::pollEvents( bool block ) {
	if ( this->m_events.empty() ) {
		do {
			this->processEvents();
		} while ( block && this->m_events.empty() );
	}
	// doesnt get used
	while ( !this->m_events.empty() ) {
		auto& event = this->m_events.front();
	/*
		if ( event.payload.is<uf::stl::string>() ) {
			ext::json::Value payload = uf::Serializer( event.payload.as<uf::stl::string>() );
			uf::hooks.call( "window:Event", payload );
			uf::hooks.call( event.name, payload );
		} else if ( event.payload.is<uf::Serializer>() ) {
			uf::Serializer& payload = event.payload.as<uf::Serializer>();
			uf::hooks.call( "window:Event", payload );
			uf::hooks.call( event.name, payload );
		} else if ( event.payload.is<ext::json::Value>() ) {
			ext::json::Value& payload = event.payload.as<ext::json::Value>();
			uf::hooks.call( "window:Event", payload );
			uf::hooks.call( event.name, payload );
		} else */ {			
			uf::hooks.call( "window:Event", event.payload );
			uf::hooks.call( event.name, event.payload );
		}
		this->m_events.pop();
	}
	return true;
#if 0
	/* Empty; look for something! */ if ( this->m_events.readable.empty() && this->m_events.optimal.empty() ) {
		do {
			this->processEvents();
		} while ( block && this->m_events.readable.empty() && this->m_events.optimal.empty() );
	}

	if ( uf::hooks.prefersReadable() ) // Should we handle events in a readable format...
	/* Process events parsed in a readable format */ {
		// process things
		while ( !this->m_events.readable.empty() ) {
			auto& event = this->m_events.readable.front();
			try {
				uf::hooks.call( "window:Event", event.argument );
				uf::hooks.call( event.name, event.argument );
			//	uf::iostream << event.name << "\t" << event.argument << "\n";
			} catch ( ... ) {
				// Let the hook handler handle the exceptions
			}
			this->m_events.readable.pop();
		}
	//	this->m_events.optimal.clear(); 	// flush queue in case
	} else // ... or in an optimal format?
	/* Process events parsed in an optimal format */ {
		// process things
		while ( !this->m_events.optimal.empty() ) {
			auto& event = this->m_events.optimal.front();
			try {
				uf::hooks.call( "window:Event", event.argument );
				uf::hooks.call( event.name, event.argument );
			//	uf::iostream << event.name << "\t" << event.argument << "\n";
			} catch ( ... ) {
				// Let the hook handler handle the exceptions
			}
			uf::userdata::destroy(event.argument);
			this->m_events.optimal.pop();
		}
	//	this->m_events.readable.clear(); 	// flush queue in case
	}
	return true;
#endif
}
#endif