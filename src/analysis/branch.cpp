#include <analysis/branch.hpp>

namespace Analysis {
Branch::Branch(uint16_t start, const List &elements)
  : m_start(start), m_elements(elements)
{
}

Branch::~Branch() {
}
}
