/*
#include <uf/utils/string/rle.h>

#include <iostream>
pod::RLE::string_t uf::rle::encode( const std::vector<uint16_t>& source ) {
	pod::RLE::string_t destination;
	destination.reserve( source.size() );

	for ( std::size_t i = 0; i < source.size(); ++i ) {
		pod::RLE reg = { 1, source[i] };
		while ( i + 1 < source.size() && source[i] == source[i + 1] ) ++reg.length, ++i;
		destination.push_back( reg );
	}

	destination.shrink_to_fit();
	return destination;
}

std::vector<uint16_t> uf::rle::decode( const pod::RLE::string_t& source ) {
	std::vector<uint16_t> destination;

	for ( auto& s : source ) {
		destination.reserve( destination.size() + s.length );
		for ( std::size_t i = 0; i < s.length; ++i ) destination.push_back(s.value);
	}

	destination.shrink_to_fit();
	return destination;
}

pod::RLE::string_t uf::rle::wrap( const std::vector<uint16_t>& source ) {
	pod::RLE::string_t destination;
	destination.reserve( source.size() / 2 );

	pod::RLE reg = { 0, 0 };

	for ( auto& s : source ) {
		if ( reg.length == 0 ) {
			reg.length = s;
		} else {
			reg.value = s;
			destination.push_back(reg);
			reg.length = 0;
		}
	}
	return destination;
}

std::vector<uint16_t> uf::rle::unwrap( const pod::RLE::string_t& source ) {
	std::vector<uint16_t> destination;
	destination.reserve( source.size() * 2 );
	for ( auto& s : source ) {
		destination.push_back( s.length );
		destination.push_back( s.value );
	}
	return destination;
}
*/