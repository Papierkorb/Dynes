#ifndef LUA_CODEGENERATOR_HPP
#define LUA_CODEGENERATOR_HPP

#include <string>

namespace Analysis { class Function; }

namespace Lua {
struct MachineSpecifics;

class CodeGenerator {
public:
  /** Target machine. */
  enum Machine {
    LuaJit,
    Lua53,
  };

  CodeGenerator(Machine machine = Lua53);
  ~CodeGenerator();

  std::string translate(Analysis::Function &func);
private:
  MachineSpecifics *m_machine;
};
}

#endif // LUA_CODEGENERATOR_HPP
