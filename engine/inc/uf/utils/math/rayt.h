#pragma once

#include <uf/config.h>
#include <cstdint>

namespace pod {
	struct UF_API Primitive {
		static const uint32_t EMPTY = (uint32_t) -1; 
		static const uint32_t CUBE = 1;
		static const uint32_t LEAF = 2;
		static const uint32_t TREE = 3;
		static const uint32_t ROOT = 4;
		alignas(16) pod::Vector4f position; 			// 4 * 4 = 16 bytes
		alignas(4) uint32_t type; 						// 4 * 1 = 4 bytes
	};
	struct UF_API Light {
		alignas(16) pod::Vector3f position;
		alignas(16) pod::Vector3f color;
	};
	struct UF_API Tree {
		static const size_t TREE_SIZE = 8;
		alignas(16) pod::Vector4f position; 			// 4 * 4 = 16 bytes
		alignas(4) uint32_t type; 						// 4 * 1 = 4 bytes
		alignas(4) uint32_t children[Tree::TREE_SIZE]; 	// 4 * 8 = 32 bytes
	};
}

namespace uf {
	namespace primitive {
		std::vector<pod::Tree> UF_API populate( const std::vector<pod::Primitive>& cubes );
		std::vector<pod::Tree> UF_API populateEntirely( const std::vector<pod::Primitive>& cubes );
		std::vector<pod::Tree> UF_API populateEntirely( const std::vector<pod::Tree>& trees, bool = false );
		void UF_API test( const std::vector<pod::Primitive>& cubes, const std::vector<pod::Tree>& trees );
	}
}