#ifndef CORE_RUNNER_HPP
#define CORE_RUNNER_HPP

#include <QObject>
#include "inesfile.hpp"

struct RunnerPrivate;

namespace Ppu {
class SurfaceManager;
class Memory;
}

namespace Cpu {
  class Memory;
}

namespace Core {

/**
 * Facade class constructing and maintaining the NES emulation back-end.
 */
class Runner : public QObject {
  Q_OBJECT
public:

  /**
   * @param ines The loaded \c .nes file
   * @param cpuType Name of the CPU core, \sa Cpu::Base
   * @param surfaces Video display front-end
   */
  explicit Runner(const InesFile &ines, const QString &cpuType, Ppu::SurfaceManager *surfaces, QObject *parent = nullptr);
  ~Runner() override;

  /** The used ines file. */
  InesFile ines() const;

  /** Pointer to the memory as seen by the CPU. */
  Cpu::Memory *ram();

  /** Pointer to the memory as seen by the PPU. */
  Ppu::Memory *vram();

public slots:
  /**
   * Resets the internal state.  \b Must be called before calling \c tick() the
   * first time.
   */
  void reset(bool hard = true);

  /**
   * Advances the simulation by one frame.
   */
  void tick();

private:
  RunnerPrivate *d;
};
}

#endif // CORE_RUNNER_HPP
