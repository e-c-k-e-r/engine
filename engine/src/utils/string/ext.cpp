#include <uf/utils/string/ext.h>
#include <algorithm>
#include <bitset>
#include <iomanip>
#include <sstream>
#include <cmath>
#include <iostream>

#include <regex>

/*
bool uf::string::match( const uf::stl::string& str, const uf::stl::string& r ) {
	std::regex regex(r);
	std::smatch match;
	return std::regex_search( str, match, regex );
}
*/
bool uf::string::isRegex( const uf::stl::string& str ) {
	return str.front() == '/' && str.back() == '/';
}
uf::stl::vector<uf::stl::string> uf::string::match( const uf::stl::string& str, const uf::stl::string& r ) {
	std::regex regex(uf::string::isRegex(r) ? r.substr(1,r.length()-2) : r);
	std::smatch match;
	uf::stl::vector<uf::stl::string> matches;
	if ( std::regex_search( str, match, regex ) ) {
		for ( auto& m : match ) {
			matches.emplace_back(m.str());
		}
	}
	
	return matches;
}
bool uf::string::matched( const uf::stl::string& str, const uf::stl::string& r ) {
	std::regex regex(uf::string::isRegex(r) ? r.substr(1,r.length()-2) : r);
	std::smatch match;
	return std::regex_search( str, match, regex );
}

uf::stl::string uf::string::lowercase( const uf::stl::string& str ) {
	uf::stl::string lower = str;
	std::transform(lower.begin(), lower.end(), lower.begin(), ::tolower);
	return lower;
}
uf::stl::string uf::string::uppercase( const uf::stl::string& str ) {
	uf::stl::string upper = str;
	std::transform(upper.begin(), upper.end(), upper.begin(), ::toupper);
	return upper;
}
uf::stl::vector<uf::stl::string> uf::string::split( const uf::stl::string& str, const uf::stl::string& delim ) {
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
uf::stl::string uf::string::join( const uf::stl::vector<uf::stl::string>& strings, const uf::stl::string& delim, bool trailing ) {
	uf::stl::stringstream ss;
	size_t len = strings.size();
	for ( size_t i = 0; i < len; ++i ) {
		ss << strings[i];
		if ( trailing || i + 1 < len ) ss << delim;
	}
	return ss.str();
}
*/
uf::stl::vector<const char*> uf::string::cStrings( const uf::stl::vector<uf::stl::string>& strings ) {
	uf::stl::vector<const char*> v;
	v.reserve( strings.size() );

	for ( auto& s : strings ) {
		v.emplace_back(s.c_str());
	}

	return v;
}

uf::stl::string uf::string::replace( const uf::stl::string& string, const uf::stl::string& search, const uf::stl::string& replace ) {
	uf::stl::string result = string;

	if ( uf::string::isRegex(search) ) {
		std::regex regex(search.substr(1,search.length()-2));
		result = std::regex_replace( string, regex, replace );
	} else {
		size_t start_pos = string.find(search);
		if( start_pos == uf::stl::string::npos ) return result;
		result.replace(start_pos, search.length(), replace);
	}
	return result;
}
bool uf::string::contains( const uf::stl::string& string, const uf::stl::string& search ) {
	return string.find(search) != uf::stl::string::npos;
}
uf::stl::string uf::string::si( double value, const uf::stl::string& unit, size_t precision ) {
	static const struct {
		int exp;
		const char* prefix;
	} SI[] = {
		{24, "Y"}, {21, "Z"}, {18, "E"}, {15, "P"}, {12, "T"},
		{ 9, "G"}, { 6, "M"}, { 3, "k"}, { 0, "" },
		{-3, "m"}, {-6, "μ"}, {-9, "n"}, {-12, "p"}
	};

	if (value == 0.0) {
		return "0" + unit;
	}

	int exp = static_cast<int>(std::floor(std::log10(std::fabs(value))));
	int exp3 = exp / 3 * 3; // Round down to nearest multiple of 3

	// Clamp to available SI prefixes
	if (exp3 > 24) exp3 = 24;
	if (exp3 < -12) exp3 = -12;

	// Find matching SI prefix
	const char* prefix = "";
	for (const auto& si : SI) {
		if (exp3 == si.exp) {
			prefix = si.prefix;
			break;
		}
	}

	double scaled = value / std::pow(10, exp3);

	uf::stl::stringstream ss;
	ss << std::fixed << std::setprecision(precision) << scaled << prefix << unit;
	return ss.str();
}