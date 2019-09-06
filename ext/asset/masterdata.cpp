#include "./masterdata.h"

#include <iostream>

//std::string ext::MasterData::root = "https://el..xyz/mastertable/get/%TABLE%/%KEY%?.json";
std::string ext::MasterData::root = "./data/master/%TABLE%.json";
ext::Asset ext::MasterData::assetLoader;

uf::Serializer ext::MasterData::load( const std::string& table, size_t key ) {
	return this->load( table, std::to_string(key) );
}
uf::Serializer ext::MasterData::load( const std::string& table, const std::string& key ) {
	this->m_table = table;
	this->m_key = key;

	std::string url = uf::string::replace( root, "%TABLE%", table );
	url = uf::string::replace( url, "%KEY%", key );
	std::string filename = assetLoader.cache(url);
	if ( filename != "" ) this->m_data.readFromFile(filename);
	return this->get();
}
uf::Serializer ext::MasterData::get( const std::string& key ) const {
	std::string k = (key == "" ? this->m_key : key);
	if ( k != "" ) return this->m_data[k];
	return this->m_data;
}
const std::string& ext::MasterData::tableName() const {
	return this->m_table;
}
const std::string& ext::MasterData::keyName() const {
	return this->m_key;
}