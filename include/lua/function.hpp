#ifndef LUA_FUNCTION_HPP
#define LUA_FUNCTION_HPP

#include <analysis/function.hpp>

struct lua_State;

namespace Lua {

/**
 * Lua function compiled out of a analyzed 6502 function.
 */
class Function {
public:
  Function(const Analysis::Function &analyzed, lua_State *lua, int ref);
  ~Function();

  /** The base function data. */
  Analysis::Function &analyzed() { return this->m_analyzed; }

  /** The base function data. */
  const Analysis::Function &analyzed() const { return this->m_analyzed; }

  /** Function reference id in the Lua state. */
  int ref() const { return this->m_ref; }

  /** Pushes the function onto the Lua stack. */
  void pushOntoStack();

private:
  Analysis::Function m_analyzed;
  lua_State *m_lua;
  int m_ref;
};
}

#endif // LUA_FUNCTION_HPP
