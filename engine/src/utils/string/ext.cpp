#include <uf/utils/string/ext.h>
#include <algorithm>
#include <bitset>
#include <iomanip>
#include <sstream>
#include <cmath>

std::string UF_API uf::string::lowercase( const std::string& str ) {
	std::string lower = str;
	std::transform(lower.begin(), lower.end(), lower.begin(), ::tolower);
	return lower;
}
std::string UF_API uf::string::uppercase( const std::string& str ) {
	std::string upper = str;
	std::transform(upper.begin(), upper.end(), upper.begin(), ::toupper);
	return upper;
}
#include <iostream>
std::vector<std::string> UF_API uf::string::split( const std::string& str, const std::string& delim ) {
	std::vector<std::string> tokens;
	size_t prev = 0, pos = 0;
	do {
		pos = str.find(delim, prev);
		if (pos == std::string::npos) pos = str.length();
		std::string token = str.substr(prev, pos-prev);
		if (!token.empty()) tokens.push_back(token);
		prev = pos + delim.length();
	} while (pos < str.length() && prev < str.length());
	if ( tokens.empty() ) tokens.emplace_back(str);
    return tokens;
/*
	std::vector<std::string> cont;
	size_t last = 0, next = 0;
	while ((next = str.find(needle, last)) != std::string::npos) {
		cont.push_back(str.substr(last, next-last));
		last = next + 1;
	}
	cont.push_back( str.substr(last) );
	return cont;
*/
}
std::string UF_API uf::string::replace( const std::string& string, const std::string& search, const std::string& replace ) {
	std::string result = string;
	size_t start_pos = string.find(search);
	if( start_pos == std::string::npos ) return result;
	result.replace(start_pos, search.length(), replace);
	return result;
}
std::string UF_API uf::string::si( double value, const std::string& unit, size_t precision ) {
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

	std::stringstream ss;
	ss << std::fixed << std::setprecision(precision) << base;
	std::string string = ss.str();

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