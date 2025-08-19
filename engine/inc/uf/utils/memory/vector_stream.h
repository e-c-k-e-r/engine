#pragma once

#include <uf/config.h>
#include <streambuf>

#include "vector.h"

// cringe......
namespace uf {
	namespace stl {
		class vector_streambuf : public std::streambuf {
			uf::stl::vector<uint8_t>& buffer;
		public:
			explicit vector_streambuf( uf::stl::vector<uint8_t>& buf ) : buffer(buf) {}
		protected:
			int_type overflow(int_type ch) override {
				if ( ch != EOF ) {
					buffer.emplace_back(ch);
				}
				return ch;
			}

			std::streamsize xsputn(const char* s, std::streamsize count) override {
				buffer.insert(buffer.end(), s, s + count);
				return count;
			}
		};

		class vector_stream : public std::ostream {
			vector_streambuf buf;
		public:
			explicit vector_stream( uf::stl::vector<uint8_t>& vec) : std::ostream(&buf), buf(vec) {}
		};
	}
}