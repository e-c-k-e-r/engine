#include <uf/spec/window/window.h>
#include <uf/utils/hook/hook.h>
#include <uf/utils/io/iostream.h>

#include <uf/utils/serialize/serializer.h>
#include <sstream>

void UF_API_CALL spec::uni::Window::pushEvent( const uf::ReadableHook::name_t& name, const uf::ReadableHook::argument_t& argument ) {
	if ( !uf::hooks.prefersReadable() ) return;
	if ( !uf::hooks.exists(name) ) return;
	this->m_events.readable.push({name, argument});
}
void UF_API_CALL spec::uni::Window::pushEvent( const uf::OptimalHook::name_t& name, const uf::OptimalHook::argument_t& argument ) {
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
void UF_API_CALL spec::uni::Window::pushEvent( const uf::ReadableHook::argument_t& serialized ) {
	if ( !uf::hooks.prefersReadable() ) return;
	uf::Serializer json = serialized;
	if ( !uf::hooks.exists(json["type"].as<std::string>()) ) return;
	this->m_events.readable.push({json["type"].as<std::string>(), serialized});
}
void UF_API_CALL spec::uni::Window::pushEvent( const uf::OptimalHook::argument_t& userdata ) {
	if ( uf::hooks.prefersReadable() ) return;
	// Header userdata
	struct Header {
		std::string type;
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
void UF_API_CALL spec::uni::Window::processEvents() {
}
bool UF_API_CALL spec::uni::Window::isKeyPressed(const std::string& key) {
	return false;
}
bool UF_API_CALL spec::uni::Window::pollEvents( bool block ) {
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
}