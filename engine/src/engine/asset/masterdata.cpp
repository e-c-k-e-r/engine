#include <uf/engine/asset/masterdata.h>
#include <iostream>

//std::string uf::MasterData::root = "https://el..xyz/mastertable/get/%TABLE%/%KEY%?.json";
std::string uf::MasterData::root = "./data/master/%TABLE%.json";
uf::Asset uf::MasterData::assetLoader;

uf::Serializer uf::MasterData::load( const std::string& table, size_t key ) {
	return this->load( table, std::to_string(key) );
}
uf::Serializer uf::MasterData::load( const std::string& table, const std::string& key ) {
	this->m_table = table;
	this->m_key = key;

	std::string url = uf::string::replace( root, "%TABLE%", table );
	url = uf::string::replace( url, "%KEY%", key );
	std::string filename = assetLoader.cache(url);
	if ( filename != "" ) this->m_data.readFromFile(filename);
	return this->get();
}
uf::Serializer uf::MasterData::get( const std::string& key ) const {
	std::string k = (key == "" ? this->m_key : key);
	if ( k != "" ) return this->m_data[k];
	return this->m_data;
}
const std::string& uf::MasterData::tableName() const {
	return this->m_table;
}
const std::string& uf::MasterData::keyName() const {
	return this->m_key;
}