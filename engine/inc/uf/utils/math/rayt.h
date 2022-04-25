#pragma once
#if 0
#include <uf/config.h>
#include <cstdint>

namespace pod {
	struct UF_API RTPrimitive {
		static const uint32_t EMPTY = (uint32_t) -1; 
		static const uint32_t CUBE = 1;
		static const uint32_t LEAF = 2;
		static const uint32_t TREE = 3;
		static const uint32_t ROOT = 4;
		pod::Vector4f position; 			// 4 * 4 = 16 bytes
		uint32_t type; 						// 4 * 1 = 4 bytes
	};
	struct UF_API Light {
		pod::Vector3f position;
		pod::Vector3f color;
	};
	struct UF_API Tree {
		static const size_t TREE_SIZE = 8;
		pod::Vector4f position; 			// 4 * 4 = 16 bytes
		uint32_t type; 						// 4 * 1 = 4 bytes
		uint32_t children[Tree::TREE_SIZE]; 	// 4 * 8 = 32 bytes
	};
}

namespace uf {
	namespace primitive {
		uf::stl::vector<pod::Tree> UF_API populate( const uf::stl::vector<pod::RTPrimitive>& cubes );
		uf::stl::vector<pod::Tree> UF_API populateEntirely( const uf::stl::vector<pod::RTPrimitive>& cubes );
		uf::stl::vector<pod::Tree> UF_API populateEntirely( const uf::stl::vector<pod::Tree>& trees, bool = false );
		void UF_API test( const uf::stl::vector<pod::RTPrimitive>& cubes, const uf::stl::vector<pod::Tree>& trees );
	}
}
#endif