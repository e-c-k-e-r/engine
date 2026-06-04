#pragma once

#include <uf/config.h>
#include <uf/utils/image/image.h>

namespace ext {
	namespace valve {
		bool UF_API loadVmt( uf::Serializer& dict, const uf::stl::string& filename );
		bool UF_API loadVtf( pod::Image& image, const uf::stl::string& filename );
	}
}