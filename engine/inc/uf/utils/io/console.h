#pragma once

#include <uf/config.h>
#include <uf/utils/string/string.h>
#include <uf/utils/math/vector.h>
#include <uf/utils/memory/unordered_map.h>

//#define UF_INPUT_USE_ENUM_MOUSE 1

namespace uf {
	namespace console {
		struct Command {
			typedef std::function<uf::stl::string( const uf::stl::string& )> function_t;

			uf::stl::string description;
			function_t callback;
		};
		extern UF_API uf::stl::unordered_map<uf::stl::string, uf::console::Command> commands;
		extern UF_API uf::stl::vector<uf::stl::string> log;
		extern UF_API uf::stl::vector<uf::stl::string> history;

		void UF_API initialize();

		void UF_API clear();
		void UF_API print( const uf::stl::string& );

		uf::stl::string UF_API execute( const uf::stl::string& );
		uf::stl::string UF_API execute( const uf::stl::string&, const uf::stl::string& );

		// callback( ["arg1", "arg2"] )
		void UF_API registerCommand( const uf::stl::string&, const uf::stl::string&, const std::function<uf::stl::string( const uf::stl::vector<uf::stl::string>& )>& );
		// callback( "arg1 arg2" )
		void UF_API registerCommand( const uf::stl::string&, const uf::stl::string&, const std::function<uf::stl::string(const uf::stl::string&)>& );
		// callback()
		void UF_API registerCommand( const uf::stl::string&, const uf::stl::string&, const std::function<uf::stl::string()>& );
	}
}