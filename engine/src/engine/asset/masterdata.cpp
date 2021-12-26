#include <uf/engine/asset/masterdata.h>
#include <iostream>

//uf::stl::string uf::MasterData::root = "https://el..xyz/mastertable/get/%TABLE%/%KEY%?.json";
uf::stl::string uf::MasterData::root = uf::io::root + "/master/%TABLE%.json";
uf::Asset uf::MasterData::assetLoader;

uf::Serializer uf::MasterData::load( const uf::stl::string& table, size_t key ) {
	return this->load( table, std::to_string(key) );
}
uf::Serializer uf::MasterData::load( const uf::stl::string& table, const uf::stl::string& key ) {
	this->m_table = table;
	this->m_key = key;

	uf::stl::string url = uf::string::replace( root, "%TABLE%", table );
	auto payload = uf::Asset::resolveToPayload( uf::string::replace( url, "%KEY%", key ) );
	uf::stl::string filename = assetLoader.cache(payload);
	if ( filename != "" ) this->m_data.readFromFile(filename);
	return this->get();
}
uf::Serializer uf::MasterData::get( const uf::stl::string& key ) const {
	uf::stl::string k = (key == "" ? this->m_key : key);
	if ( k != "" ) return this->m_data[k];
	return this->m_data;
}
const uf::stl::string& uf::MasterData::tableName() const {
	return this->m_table;
}
const uf::stl::string& uf::MasterData::keyName() const {
	return this->m_key;
}