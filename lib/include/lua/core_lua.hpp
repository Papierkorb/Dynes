#ifndef LUA_CORE_HPP
#define LUA_CORE_HPP

#include <cpu/base.hpp>
#include <cpu/state.hpp>
#include <cpu/memory.hpp>

namespace Lua {
struct CorePrivate;

class Core : public Cpu::Base {
  Q_OBJECT
public:
  Core(const Cpu::Memory::Ptr &memory, Cpu::State state, QObject *parent = nullptr);
  ~Core() override;

  int run(int cycles) override;
  void jump(uint16_t address) override;
private:
  CorePrivate *d;
};
}

#endif // LUA_CORE_HPP
