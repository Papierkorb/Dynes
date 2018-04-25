#include <analysis/conditionalinstruction.hpp>

namespace Analysis {
ConditionalInstruction::ConditionalInstruction(Instruction base, Branch *truthy, Branch *falsy)
  : Instruction(base.command, base.addressing, base.cycles, base.op16)
{
  this->m_trueBranch = truthy;
  this->m_falseBranch = falsy;
}

Branch *ConditionalInstruction::trueBranch() const {
  return m_trueBranch;
}

Branch *ConditionalInstruction::falseBranch() const {
  return m_falseBranch;
}
}
