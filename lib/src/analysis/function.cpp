#include <analysis/branch.hpp>
#include <analysis/function.hpp>

namespace Analysis {
Function::Function(uint64_t tag, uint16_t begin, bool cacheable)
  : m_tag(tag), m_begin(begin), m_cacheable(cacheable)
{
}

Function::~Function() {
  if (this->m_branches.isDetached())
    qDeleteAll(this->m_branches);
}

const QMap<uint16_t, Branch *> &Function::branches() const {
  return this->m_branches;
}

Branch *Function::branch(uint16_t address) {
  return this->m_branches.value(address);
}

const Branch *Function::branch(uint16_t address) const {
  return this->m_branches.value(address);
}

uint16_t Function::begin() const {
  return this->m_begin;
}

Branch *Function::root() {
  return this->m_branches.value(this->m_begin);
}

void Function::add(Branch *branch) {
  this->m_branches.insert(branch->start(), branch);
}

QString Function::nativeName() const {
  return QStringLiteral("dynarec6502_%1_%2")
      .arg(this->m_tag, 16, 16, QLatin1Char('0'))
      .arg(this->m_begin, 4, 16, QLatin1Char('0'));
}
}
