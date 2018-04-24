#ifndef ANALYSIS_FUNCTIONDISASSEMBLER_HPP
#define ANALYSIS_FUNCTIONDISASSEMBLER_HPP

#include <core/data.hpp>

namespace Analysis {
class Function;
struct FunctionDisassemblerImpl;

/**
 * Disassembler with function boundary discovery.
 */
class FunctionDisassembler {
public:
  FunctionDisassembler(Core::Data::Ptr &data);
  ~FunctionDisassembler();

  Analysis::Function disassemble(uint16_t address);

private:
  FunctionDisassemblerImpl *impl;
};
}

#endif // ANALYSIS_FUNCTIONDISASSEMBLER_HPP
