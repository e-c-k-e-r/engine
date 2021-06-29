template<typename T, typename U>
typename pod::RLE<T,U>::string_t uf::rle::encode( const uf::stl::vector<T>& source ) {
	typename pod::RLE<T,U>::string_t destination;
	destination.reserve( source.size() );

	for ( std::size_t i = 0; i < source.size(); ++i ) {
		pod::RLE<T,U> reg = { 1, source[i] };
		while ( i + 1 < source.size() && source[i] == source[i + 1] ) ++reg.length, ++i;
		destination.push_back( reg );
	}

	destination.shrink_to_fit();
	return destination;
}
template<typename T, typename U>
uf::stl::vector<T> uf::rle::decode( const pod::RLE<T,U>& source ) {
	uf::stl::vector<T> destination;

	for ( auto& s : source ) {
		destination.reserve( destination.size() + s.length );
		for ( std::size_t i = 0; i < s.length; ++i ) destination.push_back(s.value);
	}

	destination.shrink_to_fit();
	return destination;
}