#ifndef ANALYSIS_BRANCH_HPP
#define ANALYSIS_BRANCH_HPP

#include <core/instruction.hpp>
#include "conditionalinstruction.hpp"

#include <vector>
#include <variant>
#include <tuple>

namespace Analysis {

/**
 * Branch of an analyzed \c Function.
 *
 * Instances by this class are managed by its owning \c Function.
 */
class Branch {
public:
  using Instruction = std::variant<Core::Instruction, ConditionalInstruction>;
  using Element = std::pair<uint16_t, Instruction>;
  using List = std::vector<Element>;

  Branch(uint16_t start, const List &elements = List());
  ~Branch();

  /** Returns the instruction list. */
  List &elements() { return this->m_elements; }

  /** Returns the instruction list. */
  const List &elements() const { return this->m_elements; }

  /** Start address of this branch. */
  uint16_t start() const { return this->m_start; }

private:
  uint16_t m_start;
  List m_elements;
};
}

#endif // ANALYSIS_BRANCH_HPP
