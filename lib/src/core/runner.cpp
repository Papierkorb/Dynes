#include <core/runner.hpp>

#include <ppu/surfacemanager.hpp>
#include <ppu/memory.hpp>
#include <ppu/renderer.hpp>

#include <cpu/base.hpp>
#include <cpu/dumphook.hpp>
#include <cpu/memory.hpp>

#include <cartridge/base.hpp>

#include <QDebug>

// If defined, installs a Cpu::DumpHook into the CPU.  Cores which support it
// will then log all instructions to STDERR.
// The dynarec Core uses environment variables instead.
//#define TRACE_INSTRUCTIONS

struct RunnerPrivate {
  RunnerPrivate(const QString &type, const Core::InesFile &i) : cpuType(type), ines(i) { }

  QString cpuType;
  Core::InesFile ines;
  Cartridge::Base::Ptr cartridge;

  Ppu::SurfaceManager *surfaces;
  Cpu::Memory::Ptr ram;
  Cpu::Base *cpu;

  Ppu::Memory::Ptr vram;
  Ppu::Renderer *renderer;

  int cycles = 0;
};

namespace Core {
Runner::Runner(const Core::InesFile &ines, const QString &cpuType, Ppu::SurfaceManager *surfaces, QObject *parent)
  : QObject(parent), d(new RunnerPrivate(cpuType, ines))
{
  this->d->surfaces = surfaces;
  this->d->cartridge = Cartridge::Base::createById(ines.mapperType(), ines);

  this->d->vram = Ppu::Memory::Ptr(new Ppu::Memory(this->d->cartridge));
  this->d->ram = Cpu::Memory::Ptr(new Cpu::Memory(this->d->vram, this->d->cartridge));
  this->d->cpu = Cpu::Base::createByName(cpuType, this->d->ram, this);

  qDebug() << "Using CPU core" << cpuType;

#ifdef TRACE_INSTRUCTIONS
  this->d->cpu->setHook(new Cpu::DumpHook);
#endif

  this->d->renderer = new Ppu::Renderer(this->d->vram.get(), surfaces, this->d->cpu);
  this->reset();
}

Runner::~Runner() {
  delete this->d->renderer;
  delete this->d;
}

InesFile Runner::ines() const {
  return this->d->ines;
}

Cpu::Memory *Runner::ram() {
  return this->d->ram.get();
}

Ppu::Memory *Runner::vram() {
  return this->d->vram.get();
}

const QString &Runner::cpuImplementation() const {
  return this->d->cpuType;
}

void Runner::reset(bool hard) {
  if (hard) {
    this->d->ram->reset();
  }

  this->d->vram->reset();
  this->d->cpu->jumpToVector(Cpu::Interrupt::Reset);
}

void Runner::tick() {
  constexpr int TOTAL_CYCLES = 29781;
  constexpr int PER_LINE = TOTAL_CYCLES / 260;
  constexpr int LEFTOVER = TOTAL_CYCLES - PER_LINE * 260; // = 141

  this->d->cycles += LEFTOVER;
  do {
    this->d->cycles = this->d->cpu->run(this->d->cycles + PER_LINE);
  } while (!this->d->renderer->drawScanLine());
}

}
