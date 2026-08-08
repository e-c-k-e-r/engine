#include <uf/utils/memory/writer.h>

uf::stl::writer::writer( uf::stl::vector<uint8_t>& buffer, uint32_t offset, bool aligned ) : m_buffer(buffer), m_offset(offset), m_aligned(aligned) {

}

uf::stl::writer::writer( uf::stl::vector<uint8_t>& buffer, bool aligned ) : m_buffer(buffer), m_offset(0), m_aligned(aligned) {

}