#ifndef ANALYSIS_CONDITIONALINSTRUCTION_HPP
#define ANALYSIS_CONDITIONALINSTRUCTION_HPP

#include <core/instruction.hpp>

namespace Analysis {
class Branch;

/**
 * Like \c Core::Instruction, but also stores which (cached) branches to take in
 * each scenario.
 */
class ConditionalInstruction : public Core::Instruction {
public:
  ConditionalInstruction(Instruction base, Branch *truthy, Branch *falsy);

  /** The branch to follow if the condition was met. */
  Branch *trueBranch() const;

  /** The branch to follow if the condition was \b not met. */
  Branch *falseBranch() const;

private:
  Branch *m_trueBranch;
  Branch *m_falseBranch;
};
}

#endif // ANALYSIS_CONDITIONALINSTRUCTION_HPP
