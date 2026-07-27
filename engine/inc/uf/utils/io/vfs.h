#pragma once

#include <uf/config.h>
#include <uf/utils/singletons/pre_main.h>
#include <uf/utils/memory/string.h>
#include <uf/utils/memory/vector.h>
#include <uf/utils/userdata/userdata.h>
#include <functional>

namespace pod {
	struct Range;

	struct File {
		void* handle = nullptr;

		size_t (*read)(void* handle, void* buffer, size_t bytes) = nullptr;
		bool   (*seek)(void* handle, long offset, int origin) = nullptr;
		size_t (*tell)(void* handle) = nullptr;
		void   (*close)(void* handle) = nullptr;

		operator bool() const { return handle != nullptr; }
	};

	struct Mount {
		uf::stl::string prefix = "";
		uf::stl::string path = "";
		int priority = 0;
		pod::PointeredUserdata userdata;

		bool   (*exists)    (pod::Mount&, const uf::stl::string&) = nullptr;
		size_t (*size)      (pod::Mount&, const uf::stl::string&) = nullptr;
		size_t (*mtime)     (pod::Mount&, const uf::stl::string&) = nullptr;
		bool   (*read)      (pod::Mount&, const uf::stl::string&, uf::stl::vector<uint8_t>&) = nullptr;
		size_t (*write)     (pod::Mount&, const uf::stl::string&, const void*, size_t) = nullptr;
		bool   (*mkdir)     (pod::Mount&, const uf::stl::string&) = nullptr;
		bool   (*stream)    (pod::Mount&, const uf::stl::string&, size_t, std::function<bool(const uint8_t* data, size_t size)>) = nullptr;
		pod::File (*open)   (pod::Mount&, const uf::stl::string&) = nullptr;

		uf::stl::vector<uf::stl::string> (*list)(pod::Mount&, const uf::stl::string&, const uf::stl::string&, bool) = nullptr;

		bool   (*readRange) (pod::Mount&, const uf::stl::string&, size_t, size_t, uf::stl::vector<uint8_t>&) = nullptr;
		bool   (*readRanges)(pod::Mount&, const uf::stl::string&, const uf::stl::vector<pod::Range>&, uf::stl::vector<uint8_t>&) = nullptr;
	};

}

namespace uf {
	namespace vfs {
		extern UF_API uf::stl::vector<pod::Mount> mounts;
		struct UF_API Mount {
			size_t hash = {};
			const pod::Mount* ptr = NULL;
			bool temp = false;
			~Mount();
		};

		uf::vfs::Mount UF_API mount( const pod::Mount& mount, bool = false );
		bool UF_API unmount( size_t );
		bool UF_API unmount( const uf::vfs::Mount& );
		bool UF_API unmount( const uf::stl::string& prefix, const uf::stl::string& base );

		bool UF_API exists( const uf::stl::string& path );
		size_t UF_API size( const uf::stl::string& path );
		size_t UF_API mtime( const uf::stl::string& path );
		bool UF_API read( const uf::stl::string& path, uf::stl::vector<uint8_t>& buffer );
		
		size_t UF_API write( const uf::stl::string& path, const void* data, size_t len );
		size_t UF_API write( const uf::stl::string& path, uf::stl::vector<uint8_t>& buffer );
		bool UF_API mkdir( const uf::stl::string& path );
		uf::stl::vector<uf::stl::string> UF_API list( const uf::stl::string& path, const uf::stl::string& extension = "", bool recursive = false );
		bool UF_API stream( const uf::stl::string& path, size_t chunkSize, std::function<bool(const uint8_t* data, size_t size)> callback );
		pod::File UF_API open( const uf::stl::string& path );
		
		bool UF_API readRange( const uf::stl::string& path, size_t start, size_t len, uf::stl::vector<uint8_t>& buffer );
		bool UF_API readRanges( const uf::stl::string& path, const uf::stl::vector<pod::Range>& ranges, uf::stl::vector<uint8_t>& buffer );
		
		pod::Mount UF_API createDiskMount( const uf::stl::string& uri, int priority = 0 );
		uf::stl::string UF_API resolveBase( const uf::stl::string& path );
	}
}

#include "macros.inl"