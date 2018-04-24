#ifndef INTERPRET_CORE_HPP
#define INTERPRET_CORE_HPP

#include <cpu/base.hpp>
#include <cpu/state.hpp>
#include <cpu/memory.hpp>

class InterpretCoreImpl;

namespace Interpret {

/**
 * CPU Core implementation by using a standard interpreter loop.
 */
class Core : public Cpu::Base {
  Q_OBJECT
public:
  Core(const Cpu::Memory::Ptr &mem, Cpu::State state = Cpu::State(), QObject *parent = nullptr);
  ~Core() override;

  /**
   * Runs a \b single instruction and returns afterwards.
   *
   * Mainly for debugging purposes.
   */
  void step();

  virtual int run(int cycles) override;
  virtual void jump(uint16_t address) override;

  /**
   * Executes a single \a instruction in the previously configured memory.
   *
   * This is a facility to easily integrate the interpret into other cores to
   * temporarily implement instructions.
   *
   * Special care must be taken to synchronize the CPU state before and after
   * calling this function.  Also note that this function does not implement
   * hooks.
   */
  void execute(const ::Core::Instruction &instruction);

private:
  InterpretCoreImpl *impl;
};
}

#endif // INTERPRET_CORE_HPP
