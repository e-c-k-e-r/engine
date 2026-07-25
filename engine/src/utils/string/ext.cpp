#include <uf/utils/string/ext.h>
#include <algorithm>
#include <bitset>
#include <iomanip>
#include <sstream>
#include <cmath>
#include <re.h>

bool uf::string::isRegex( const uf::stl::string& str ) {
	return str.size() > 2 && str.front() == '/' && str.back() == '/';
}

bool uf::string::matched( const uf::stl::string& str, const uf::stl::string& r ) {
	uf::stl::string pattern = uf::string::isRegex(r) ? r.substr(1, r.length() - 2) : r;

	int match_length;
	int match_idx = re_match(pattern.c_str(), str.c_str(), &match_length);

	return (match_idx != -1);
}

uf::stl::vector<uf::stl::string> uf::string::match( const uf::stl::string& str, const uf::stl::string& r ) {
	uf::stl::vector<uf::stl::string> matches;
	uf::stl::string pattern = uf::string::isRegex(r) ? r.substr(1, r.length() - 2) : r;

	re_t regex = re_compile(pattern.c_str());
	if ( !regex ) return matches;

	const char* cursor = str.c_str();
	int match_length = 0;

	int match_idx;
	while ( (match_idx = re_matchp(regex, cursor, &match_length)) != -1 ) {
		matches.emplace_back(cursor + match_idx, match_length);
		cursor += match_idx + match_length;
		if ( match_length == 0 ) {
			if (*cursor == '\0') break;
			cursor++;
		}
	}

	return matches;
}

uf::stl::string uf::string::replace( const uf::stl::string& string, const uf::stl::string& search, const uf::stl::string& replace, bool regexp ) {
	uf::stl::string result;

	if ( !regexp ) {
		result = string;
		if ( search.empty() ) return result;
		size_t start_pos = 0;
		while ( (start_pos = result.find(search, start_pos)) != uf::stl::string::npos ) {
			result.replace(start_pos, search.length(), replace);
			start_pos += replace.length();
		}
		return result;
	}

	uf::stl::string pattern = search.substr(1, search.length() - 2);
	re_t regex = re_compile(pattern.c_str());
	if ( !regex ) return string;

	const char* cursor = string.c_str();
	int match_length = 0;
	int match_idx = 0;

	while ( (match_idx = re_matchp(regex, cursor, &match_length)) != -1 ) {
		result.append(cursor, match_idx);

		result.append(replace);

		cursor += match_idx + match_length;

		if ( match_length == 0 ) {
			if (*cursor == '\0') break;
			result.push_back(*cursor);
			cursor++;
		}
	}

	result.append(cursor);
	return result;
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
}
uf::stl::vector<uf::stl::string> uf::string::split( const uf::stl::string& str, char delim ) {
	uf::stl::string d = FMT_FORMAT("{}", delim); // because it's such a pain apparently to convert a char to str
	return uf::string::split( str, d );
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

uf::stl::string uf::string::ltrim( const uf::stl::string& str ) {
	auto s = str;
	s.erase(s.begin(), std::find_if(s.begin(), s.end(), [](unsigned char ch) {
		return !std::isspace(ch);
	}));
	return s;
}

uf::stl::string uf::string::rtrim( const uf::stl::string& str ) {
	auto s = str;
	s.erase(std::find_if(s.rbegin(), s.rend(), [](unsigned char ch) {
		return !std::isspace(ch);
	}).base(), s.end());
	return s;
}
uf::stl::string uf::string::trim( const uf::stl::string& str ) {
	return uf::string::rtrim( uf::string::ltrim( str ) );
}

uf::stl::string uf::string::replace( const uf::stl::string& string, const uf::stl::string& search, const uf::stl::string& replace ) {
	return uf::string::replace( string, search, replace, uf::string::isRegex( search ) );
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

uf::stl::string uf::string::namespaceGetLast( const uf::stl::string& name ) {
	return uf::string::replace( uf::string::split( name, "::" ).back(), "<>", "" );
}
uf::stl::string uf::string::namespaceRemoveFirst( const uf::stl::string& name ) {
	auto split = uf::string::split( name, "::" );
	split.erase(split.begin());
	return uf::string::replace( uf::string::join( split, "::" ), "<>", "" );
}