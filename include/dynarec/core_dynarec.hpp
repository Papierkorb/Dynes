#ifndef DYNAREC_CORE_HPP
#define DYNAREC_CORE_HPP

#include <cpu/base.hpp>

namespace Dynarec {
struct CorePrivate;

/** A dynamic-recompiling CPU core using LLVM. */
class Core : public Cpu::Base {
  Q_OBJECT
public:
  Core(const Cpu::Memory::Ptr &mem, Cpu::State state = Cpu::State(), QObject *parent = nullptr);
  ~Core() override;

  virtual int run(int cycles) override;
  virtual void jump(uint16_t address) override;

private:
  CorePrivate *d;
};
}

#endif // DYNAREC_CORE_HPP
