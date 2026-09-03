#include <uf/utils/io/socket.h>

#include <uf/utils/io/fmt.h>
#include <uf/utils/io/iostream.h>
#include <uf/utils/memory/vector.h>

#include <cstring>
#include <cstdlib>
#include <csignal>

uf::io::socket::line_handler_t uf::io::socket::onLine = nullptr;
uf::io::socket::disconnect_handler_t uf::io::socket::onDisconnect = nullptr;

#if !UF_ENV_WINDOWS
#include <sys/socket.h>
#include <sys/un.h>
#include <unistd.h>
#include <fcntl.h>
#include <errno.h>

namespace {
	struct Client {
		int fd = -1;
		uf::stl::string inbound;
		uf::stl::string outbound;
	};
	struct Server {
		int fd = -1;
		uf::stl::string path;
		uf::stl::vector<Client> clients;
	};
	Server server;

	void closeClient( Client& client ) {
		if ( client.fd >= 0 ) ::close( client.fd );
		client.fd = -1;
	}

	// true = keep the slot; false = peer is dead
	bool flushClient( Client& client ) {
		while ( !client.outbound.empty() ) {
			ssize_t written = ::write( client.fd, client.outbound.data(), client.outbound.size() );
			if ( written > 0 ) {
				client.outbound.erase( 0, (size_t)written );
			} else if ( written < 0 && ( errno == EAGAIN || errno == EWOULDBLOCK ) ) {
				return true; // peer's too slow; finish the flush next poll
			} else {
				return false; // dead peer
			}
		}
		return true;
	}

	void acceptClients() {
		for ( size_t accepted = 0; accepted < uf::io::socket::maxClients; ++accepted ) {
			int fd = ::accept4( server.fd, nullptr, nullptr, SOCK_CLOEXEC | SOCK_NONBLOCK );
			if ( fd < 0 ) break;
			size_t slot = SIZE_MAX;
			for ( size_t i = 0; i < server.clients.size(); ++i ) if ( server.clients[ i ].fd < 0 ) { slot = i; break; }
			if ( slot == SIZE_MAX ) {
				if ( server.clients.size() >= uf::io::socket::maxClients ) { ::close( fd ); break; }
				server.clients.emplace_back();
				slot = server.clients.size() - 1;
			}
			Client& fresh = server.clients[ slot ];
			fresh = Client{};
			fresh.fd = fd;
			UF_MSG_INFO("Socket: client connected (fd {})", fd);
		}
	}
}

bool uf::io::socket::start( const uf::stl::string& path ) {
	// writing to a vanished client must kill nothing; write() returns EPIPE and flushClient drops the slot
	std::signal( SIGPIPE, SIG_IGN );

	uf::stl::string resolved = path.empty() ? "engine.sock" : path;
	int fd = ::socket( AF_UNIX, SOCK_STREAM | SOCK_NONBLOCK | SOCK_CLOEXEC, 0 );
	if ( fd < 0 ) {
		UF_MSG_ERROR("Socket: cannot create socket: {}", strerror(errno));
		return false;
	}
	sockaddr_un address;
	std::memset( &address, 0, sizeof(address) );
	address.sun_family = AF_UNIX;
	if ( resolved.size() >= sizeof(address.sun_path) ) {
		UF_MSG_ERROR("Socket: path too long: {}", resolved);
		::close( fd );
		return false;
	}
	::strncpy( address.sun_path, resolved.c_str(), sizeof(address.sun_path) - 1 );
	::unlink( resolved.c_str() );
	if ( ::bind( fd, (sockaddr*)&address, sizeof(sa_family_t) + resolved.size() ) < 0 || ::listen( fd, (int)maxClients ) < 0 ) {
		UF_MSG_ERROR("Socket: cannot bind {}: {}", resolved, strerror(errno) );
		::close( fd );
		return false;
	}
	server.fd = fd;
	server.path = resolved;
	return true;
}

void uf::io::socket::stop() {
	for ( auto& client : server.clients ) closeClient( client );
	server.clients.clear();
	if ( server.fd >= 0 ) {
		::close( server.fd );
		server.fd = -1;
	}
	if ( !server.path.empty() ) {
		::unlink( server.path.c_str() );
		server.path = "";
	}
}

bool uf::io::socket::listening() {
	return server.fd >= 0;
}

const uf::stl::string& uf::io::socket::getPath() {
	return server.path;
}

void uf::io::socket::poll() {
	if ( server.fd < 0 ) return;
	acceptClients();

	for ( size_t i = 0; i < server.clients.size(); ++i ) {
		Client& client = server.clients[ i ];
		if ( client.fd < 0 ) continue;
		if ( !flushClient( client ) ) { closeClient( client ); continue; }

		bool drop = false;
		char buffer[4096];
		for ( ;; ) {
			ssize_t n = ::read( client.fd, buffer, sizeof(buffer) );
			if ( n > 0 ) {
				client.inbound.append( buffer, (size_t)n );
				uf::stl::string::size_type newline;
				while ( ( newline = client.inbound.find( '\n' ) ) != uf::stl::string::npos ) {
					uf::stl::string line = client.inbound.substr( 0, newline );
					client.inbound.erase( 0, newline + 1 );
					if ( !line.empty() && line.back() == '\r' ) line.pop_back(); // windows insistence on \r
					if ( line.empty() ) continue;
					if ( onLine ) onLine( i, line );
				}
				if ( client.inbound.size() > maxLineBytes ) {
					drop = true;
					break;
				}
				if ( (size_t)n < sizeof(buffer) ) break;
			} else if ( n == 0 ) {
				closeClient( client );
				break;
			} else if ( errno == EAGAIN || errno == EWOULDBLOCK ) break;
			else {
				drop = true;
				break;
			}
		}
		if ( drop ) {
			if ( onDisconnect ) onDisconnect( i ); // session may still send() a parting reply
			closeClient( client );
		}
	}
}

bool uf::io::socket::send( size_t index, const uf::stl::string& text ) {
	if ( index >= server.clients.size() ) return false;
	Client& client = server.clients[ index ];
	if ( client.fd < 0 ) return false;
	client.outbound += text;
	if ( !flushClient( client ) ) closeClient( client );
	return true;
}

void uf::io::socket::broadcast( const uf::stl::string& text ) {
	for ( size_t i = 0; i < server.clients.size(); ++i ) if ( server.clients[ i ].fd >= 0 ) send( i, text );
}

#else
// to-do: winsock port

bool uf::io::socket::start( const uf::stl::string& path ) {
	UF_MSG_WARNING("Socket: unix-domain sockets are not implemented on this platform");
	return false;
}
void uf::io::socket::stop() {}
bool uf::io::socket::listening() { return false; }
const uf::stl::string& uf::io::socket::getPath() {
	static const uf::stl::string empty = "";
	return empty;
}
void uf::io::socket::poll() {}
bool uf::io::socket::send( size_t client, const uf::stl::string& text ) { return false; }
void uf::io::socket::broadcast( const uf::stl::string& text ) {}

#endif
