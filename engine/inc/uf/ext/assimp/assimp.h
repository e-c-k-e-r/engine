#pragma once

#include <uf/config.h>
#if defined(UF_USE_ASSIMP)

#include <string>
#include <uf/gl/mesh/mesh.h>
#include <uf/gl/mesh/skeletal/mesh.h>

namespace ext {
    namespace assimp {
       UF_API bool load( const std::string&, uf::Mesh&, bool = false );
       UF_API bool load( const std::string&, uf::SkeletalMesh&, bool = false );
    }
}

#endif