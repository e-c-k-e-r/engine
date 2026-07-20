#include <uf/utils/memory/reader.h>

uf::stl::reader::reader( const uf::stl::vector<uint8_t>& buffer, uint32_t offset, uint32_t length, bool zeroCopy ) : m_buffer(buffer), m_offset(offset), m_endOffset(offset + length), m_zeroCopy( zeroCopy ) {

}

template<>
const char* uf::stl::reader::read<char>( size_t readSize ) {
	if ( m_offset + readSize > m_endOffset ) return nullptr;

	const char* ptr = (const char*)(m_buffer.data() + m_offset);
	m_offset += readSize;
	return ptr;
}