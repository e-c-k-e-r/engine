#pragma once

#include "./asset.h"

namespace ext {
	class MasterData {
	protected:
		std::string m_table;
		std::string m_key;
		uf::Serializer m_data;

		static ext::Asset assetLoader;
		static std::string root;
	public:
		uf::Serializer load( const std::string&, size_t );
		uf::Serializer load( const std::string&, const std::string& = "" );
		uf::Serializer get( const std::string& k = "" ) const;
		const std::string& tableName() const;
		const std::string& keyName() const;
	};
}