#pragma once

#include <uf/config.h>
#include <uf/utils/memory/string.h>

namespace pod {
	struct UF_API Checkpoint {
		enum Type {
			GENERIC = 0,
			BEGIN,
			END,
		};

    	pod::Checkpoint::Type type = Type::GENERIC;
		uf::stl::string name = "";
		uf::stl::string info = "";
    	pod::Checkpoint* previous = NULL;
	};
}

namespace uf {
	namespace checkpoint {
		pod::Checkpoint* UF_API allocate( const pod::Checkpoint& );
		pod::Checkpoint* UF_API allocate( pod::Checkpoint::Type, const uf::stl::string&, const uf::stl::string&, pod::Checkpoint* = NULL );
		void UF_API deallocate( pod::Checkpoint& );
		void UF_API deallocate( pod::Checkpoint* );
		uf::stl::string UF_API traverse( pod::Checkpoint& );
		uf::stl::string UF_API traverse( pod::Checkpoint* );
	}
}

#define UF_CHECKPOINT_INFO(...) ::fmt::format("{}:{}@{}", __FILE__, __FUNCTION__, __LINE__)
#define UF_CHECKPOINT_MARK(COMMAND_BUFFER, TYPE, NAME, ...) markCommandBuffer( COMMAND_BUFFER, TYPE, NAME, UF_CHECKPOINT_INFO() );