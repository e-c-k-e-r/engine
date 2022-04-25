#include <uf/utils/string/ext.h>
#include <algorithm>
#include <bitset>
#include <iomanip>
#include <sstream>
#include <cmath>
#include <iostream>

#include <regex>

/*
bool UF_API uf::string::match( const uf::stl::string& str, const uf::stl::string& r ) {
	std::regex regex(r);
	std::smatch match;
	return std::regex_search( str, match, regex );
}
*/
bool UF_API uf::string::isRegex( const uf::stl::string& str ) {
	return str.front() == '/' && str.back() == '/';
}
uf::stl::vector<uf::stl::string> UF_API uf::string::matches( const uf::stl::string& str, const uf::stl::string& r ) {

	std::regex regex(r.substr(1,r.length()-2));
	std::smatch match;
	uf::stl::vector<uf::stl::string> matches;
	if ( std::regex_search( str, match, regex ) ) {
		for ( auto& m : match ) {
			UF_MSG_DEBUG(m);
			matches.emplace_back(m.str());
		}
	}
	
	return matches;
}

uf::stl::string UF_API uf::string::lowercase( const uf::stl::string& str ) {
	uf::stl::string lower = str;
	std::transform(lower.begin(), lower.end(), lower.begin(), ::tolower);
	return lower;
}
uf::stl::string UF_API uf::string::uppercase( const uf::stl::string& str ) {
	uf::stl::string upper = str;
	std::transform(upper.begin(), upper.end(), upper.begin(), ::toupper);
	return upper;
}
uf::stl::vector<uf::stl::string> UF_API uf::string::split( const uf::stl::string& str, const uf::stl::string& delim ) {
	uf::stl::vector<uf::stl::string> tokens;
	size_t prev = 0, pos = 0;
	do {
		pos = str.find(delim, prev);
		if (pos == uf::stl::string::npos) pos = str.length();
		uf::stl::string token = str.substr(prev, pos-prev);
		if (!token.empty()) tokens.push_back(token);
		prev = pos + delim.length();
	} while (pos < str.length() && prev < str.length());
	if ( tokens.empty() ) tokens.emplace_back(str);
    return tokens;
/*
	uf::stl::vector<uf::stl::string> cont;
	size_t last = 0, next = 0;
	while ((next = str.find(needle, last)) != uf::stl::string::npos) {
		cont.push_back(str.substr(last, next-last));
		last = next + 1;
	}
	cont.push_back( str.substr(last) );
	return cont;
*/
}
/*
uf::stl::string UF_API uf::string::join( const uf::stl::vector<uf::stl::string>& strings, const uf::stl::string& delim, bool trailing ) {
	uf::stl::stringstream ss;
	size_t len = strings.size();
	for ( size_t i = 0; i < len; ++i ) {
		ss << strings[i];
		if ( trailing || i + 1 < len ) ss << delim;
	}
	return ss.str();
}
*/
uf::stl::string UF_API uf::string::replace( const uf::stl::string& string, const uf::stl::string& search, const uf::stl::string& replace ) {
	uf::stl::string result = string;
	size_t start_pos = string.find(search);
	if( start_pos == uf::stl::string::npos ) return result;
	result.replace(start_pos, search.length(), replace);
	return result;
}
bool UF_API uf::string::contains( const uf::stl::string& string, const uf::stl::string& search ) {
	return string.find(search) != uf::stl::string::npos;
}
uf::stl::string UF_API uf::string::si( double value, const uf::stl::string& unit, size_t precision ) {
	int power = floor(std::log10( value ));
	double base = value / std::pow( 10, power );

//	std::cout << base << " x 10^" << power << " -> ";

	size_t pValue = -1;
	#define REDUCE(VAL)\
		while ( pValue > power && power > VAL ) {\
			--power;\
			base *= 10;\
		}\
		pValue = VAL;
	#define INCREASE(VAL)\
		if ( pValue < power && power < VAL ) std::cout << " (increasing (" << pValue << ", " << VAL << ")) ";\
		while ( pValue < power && power < VAL ) {\
			++power;\
			base /= 10;\
		}\
		pValue = VAL;
	
	REDUCE(24)
	REDUCE(21)
	REDUCE(18)
	REDUCE(15)
	REDUCE(12)
	REDUCE( 9)
	REDUCE( 6)
	REDUCE( 3)
//	REDUCE( 2)
//	REDUCE( 1)
	REDUCE( 0)
/*
//	INCREASE(-1)
	INCREASE(-2)
	INCREASE(-3)
	INCREASE(-6)
	INCREASE(-9)
	INCREASE(-12)
*/

	// std::cout << base << " x 10^" << power << std::endl;

	uf::stl::stringstream ss;
	ss << std::fixed << std::setprecision(precision) << base;
	uf::stl::string string = ss.str();

	switch ( power ) {
		case 24: string += "Y"; break;
		case 21: string += "Z"; break;
		case 18: string += "E"; break;
		case 15: string += "P"; break;
		case 12: string += "T"; break;
		case  9: string += "G"; break;
		case  6: string += "M"; break;
		case  3: string += "k"; break;
	//	case  2: string += "h"; break;
	//	case  1: string += "da"; break;
		case  0: string += ""; break;
	//	case -1: string += "d"; break;
		case -2: string += "c"; break;
		case -3: string += "m"; break;
		case -6: string += "μ"; break;
		case -9: string += "n"; break;
		case-12: string += "p"; break;
	}
	string += unit;
	return string;
}