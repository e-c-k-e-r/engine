#pragma once

#include <uf/config.h>
#include <typeindex>
#include <uf/utils/memory/unordered_map.h>

#define UF_TYPE_REGISTER 1

#include <uf/utils/memory/string.h>
namespace pod {
	struct UF_API TypeInfo {
		size_t hash;
		size_t size;
		struct {
			uf::stl::string pretty;
			uf::stl::string compiler;
		} name;
	};
}
namespace std {
	template<> struct hash<pod::TypeInfo> {
	    size_t operator()(const pod::TypeInfo& type) const {
			return type.hash;
	    }
	};
}


namespace uf {
	namespace typeInfo {
		typedef std::type_index index_t;
		typedef pod::TypeInfo type_t;

		extern UF_API uf::stl::unordered_map<index_t, pod::TypeInfo>* types;

		void UF_API registerType( const index_t&, size_t = 0, const uf::stl::string& = "" );
		const pod::TypeInfo& UF_API getType( size_t );
		const pod::TypeInfo& UF_API getType( const index_t& );

		template<typename T>
		const std::type_index getIndex();
		template<typename T>
		void registerType( const uf::stl::string& pretty = "" );
		template<typename T>
		const pod::TypeInfo& getType();
	/*
		template<typename T> const index_t getIndex();
		template<typename T> void registerType( const uf::stl::string& = "" );
		template<typename T> const pod::TypeInfo& getType();
	*/
	}
}

#include "type.inl"