#ifndef AMD64_CORE_HPP
#define AMD64_CORE_HPP

#include <cpu/base.hpp>
#include <cpu/state.hpp>
#include <cpu/memory.hpp>

namespace Amd64 {
using Stream = std::vector<uint8_t>;

struct CoreImpl;

class Core : public Cpu::Base {
  Q_OBJECT
public:
  Core(const Cpu::Memory::Ptr &mem, Cpu::State state = Cpu::State(), QObject *parent = nullptr);
  ~Core() override;

  virtual int run(int cycles) override;
  virtual void jump(uint16_t address) override;

private:
  CoreImpl *impl;
};
}

#endif // AMD64_CORE_HPP
