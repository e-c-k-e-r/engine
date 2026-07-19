#pragma once

#include <uf/config.h>
#include <uf/utils/singletons/pre_main.h>
#include <uf/utils/memory/string.h>
#include <uf/utils/memory/vector.h>
#include <uf/utils/userdata/userdata.h>
#include <functional>

namespace pod {
	struct Range;

	struct Mount {
		uf::stl::string prefix = "";
		uf::stl::string path = "";
		int priority = 0;
		pod::PointeredUserdata userdata;

		std::function<bool(pod::Mount&, const uf::stl::string&)> exists;
		std::function<size_t(pod::Mount&, const uf::stl::string&)> size;
		std::function<size_t(pod::Mount&, const uf::stl::string&)> mtime;
		std::function<bool(pod::Mount&, const uf::stl::string&, uf::stl::vector<uint8_t>&)> read;
		std::function<size_t(pod::Mount&, const uf::stl::string&, const void*, size_t)> write;
		std::function<bool(pod::Mount&, const uf::stl::string&)> mkdir;
		std::function<uf::stl::vector<uf::stl::string>(pod::Mount&, const uf::stl::string&, const uf::stl::string&, bool)> list;

		std::function<bool(pod::Mount&, const uf::stl::string&, size_t, size_t, uf::stl::vector<uint8_t>&)> readRange;
		std::function<bool(pod::Mount&, const uf::stl::string&, const uf::stl::vector<pod::Range>&, uf::stl::vector<uint8_t>&)> readRanges;
		
		std::function<bool(pod::Mount&, const uf::stl::string&, size_t, std::function<bool(const uint8_t* data, size_t size)>)> stream;
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
		
		bool UF_API readRange( const uf::stl::string& path, size_t start, size_t len, uf::stl::vector<uint8_t>& buffer );
		bool UF_API readRanges( const uf::stl::string& path, const uf::stl::vector<pod::Range>& ranges, uf::stl::vector<uint8_t>& buffer );
		
		bool UF_API stream( const uf::stl::string& path, size_t chunkSize, std::function<bool(const uint8_t* data, size_t size)> callback );

		pod::Mount UF_API createDiskMount( const uf::stl::string& uri, int priority = 0 );
		uf::stl::string UF_API resolveBase( const uf::stl::string& path );
	}
}

#include "macros.inl"