#pragma once

#include "vector.h"

namespace uf {
	namespace stl {
		// technically NOT a templated class, but it works with templates
		class UF_API reader {
		private:
			const uf::stl::vector<uint8_t>& m_buffer;
			uint32_t m_offset;
			uint32_t m_endOffset;
			bool m_zeroCopy; // if true, returns a pointer to the original buffer in memory 
		public:
			reader( const uf::stl::vector<uint8_t>& buffer, uint32_t offset, uint32_t length, bool zeroCopy = true );

			inline bool eof() const { return m_offset >= m_endOffset; }
			inline uint32_t offset() const { return m_offset; }
			inline uint32_t remaining() const { return m_endOffset - m_offset; }
			inline void skip( size_t bytes ) { m_offset += bytes; }

			template<typename T>
			const T* read( size_t readSize = sizeof(T) ) {
				if ( m_offset + readSize > m_endOffset ) return nullptr;
				if ( !m_zeroCopy ) {
					static thread_local T copy;
					memset(&copy, 0, sizeof(T));
					size_t copySize = std::min(sizeof(T), readSize);
					memcpy(&copy, m_buffer.data() + m_offset, copySize);
					m_offset += readSize;
					return &copy;
				}
				
				const T* ptr = (const T*)(m_buffer.data() + m_offset);
				m_offset += readSize;
				return ptr;
			}

			template<typename T>
			bool read( size_t count, uf::stl::vector<T>& outArray ) {
				size_t bytes = count * sizeof(T);
				if ( m_offset + bytes > m_endOffset ) {
					bytes = m_endOffset - m_offset;
					count = bytes / sizeof(T);
					if (count == 0) return false;
					bytes = count * sizeof(T);
				}

				if ( !m_zeroCopy ) {
					outArray.resize(count);
					memcpy(outArray.data(), m_buffer.data() + m_offset, bytes);
				} else {
					outArray.assign(
						(const T*)(m_buffer.data() + m_offset),
						(const T*)(m_buffer.data() + m_offset + bytes)
					);
				}
				m_offset += bytes;
				return true;
			}

		// to-do: #if C++ >= 20 or something
			template<typename T>
			std::span<const T> view( size_t count ) {
				size_t bytes = count * sizeof(T);
				if (m_offset + bytes > m_endOffset) return {};
				std::span<const T> view((const T*)(m_buffer.data() + m_offset), count);
				m_offset += bytes;
				return view;
			}

			template<typename T>
			const T* peek() const {
				if ( m_offset + sizeof(T) > m_endOffset ) return nullptr;
				return (const T*)(m_buffer.data() + m_offset);
			}
		};
	}
}

template<> const char* uf::stl::reader::read<char>( size_t readSize );