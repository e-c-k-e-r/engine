#pragma once

#include <uf/config.h>
#include <uf/utils/memory/string.h>

#include <functional>

namespace uf {
	namespace io {
		namespace socket {
			// generic non-blocking AF_UNIX line server; one newline-terminated line in per client

			// to-do: winsock port
			inline constexpr size_t maxClients = 8;
			inline constexpr size_t maxLineBytes = 64 * 1024;

			// fired from poll() per complete inbound line (stable client slot + content)
			typedef std::function<void(size_t, const uf::stl::string&)> line_handler_t;
			// fired right before a client gets dropped (oversized line / broken read); the slot is still send()able
			typedef std::function<void(size_t)> disconnect_handler_t;

			extern UF_API line_handler_t onLine;
			extern UF_API disconnect_handler_t onDisconnect;

			bool UF_API start( const uf::stl::string& path = "" ); // empty -> "engine.sock" (relative paths resolve against the CWD)
			void UF_API stop(); // closes every client, unlinks the path
			bool UF_API listening();
			const uf::stl::string& UF_API getPath();
			void UF_API poll(); // accept, flush short-writes, frame inbound lines into onLine
			bool UF_API send( size_t client, const uf::stl::string& text ); // append + best-effort flush; false when the slot is dead
			void UF_API broadcast( const uf::stl::string& text ); // send to every live client
		}
	}
}
