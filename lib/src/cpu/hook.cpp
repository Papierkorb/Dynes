#include <cpu/hook.hpp>

namespace Cpu {
Hook::~Hook() {
}

void Hook::beforeInstruction(Core::Instruction instruction, State &state) {
  Q_UNUSED(instruction);
  Q_UNUSED(state);
}

void Hook::afterInstruction(Core::Instruction instruction, State &state) {
  Q_UNUSED(instruction);
  Q_UNUSED(state);
}
}
