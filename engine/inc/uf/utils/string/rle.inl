template<typename T, typename U>
typename pod::RLE<T,U>::string_t uf::rle::encode( const uf::stl::vector<T>& source ) {
	typename pod::RLE<T,U>::string_t destination;
	if ( source.empty() ) return destination;

	destination.reserve( source.size() );

	for ( size_t i = 0; i < source.size(); ++i ) {
		auto reg = pod::RLE<T,U>{ 1, source[i] };

		while ( i + 1 < source.size() && source[i] == source[i + 1] && reg.length < std::numeric_limits<U>::max() ) {
			++reg.length;
			++i;
		}
		destination.emplace_back( reg );
	}

	destination.shrink_to_fit();
	return destination;
}

template<typename T, typename U>
uf::stl::vector<T> uf::rle::decode( const uf::stl::vector<pod::RLE<T,U>>& source ) {
	uf::stl::vector<T> destination;
	if ( source.empty() ) return destination;

	for ( const auto& s : source ) {
		destination.reserve( destination.size() + s.length );
		for ( size_t i = 0; i < s.length; ++i ) {
			destination.emplace_back(s.value);
		}
	}

	destination.shrink_to_fit();
	return destination;
}