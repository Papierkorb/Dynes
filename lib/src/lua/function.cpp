#include <lua/function.hpp>

#include <lua.hpp>

namespace Lua {
Function::Function(const Analysis::Function &analyzed, lua_State *lua, int ref)
  : m_analyzed(analyzed), m_lua(lua), m_ref(ref)
{
}

Function::~Function() {
  luaL_unref(this->m_lua, LUA_REGISTRYINDEX, this->m_ref);
}

void Function::pushOntoStack() {
  lua_rawgeti(this->m_lua, LUA_REGISTRYINDEX, this->m_ref);
}

}
