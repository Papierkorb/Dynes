#include <casetteplayer.hpp>
#include <instructionexecutor.hpp>

#include <memory>
#include <iostream>
#include <displaystore.hpp>

namespace Test {
static std::unique_ptr<InstructionExecutor> buildExecutor(const QString &onfail, const QString &romFile,
                                                          const QString &cpuImpl, DisplayStore *store) {
  try {
    std::unique_ptr<Core::Runner> runner(new Core::Runner(Core::InesFile::load(romFile), cpuImpl, store));
    return std::make_unique<InstructionExecutor>(std::move(runner), store);
  } catch (std::runtime_error err) {
    std::cout << "!! Failed to open NES ROM at " << romFile.toStdString() << "\n";

    if (!onfail.isEmpty()) {
      std::cout << "   " << onfail.toStdString() << "\n";
    }

    abort();
  }
}

CasettePlayer::CasettePlayer(const QStringList &instructions)
  : m_instructions(instructions)
{
}

bool CasettePlayer::play(const QString &cpuImpl) {
  std::unique_ptr<InstructionExecutor> exec;
  DisplayStore display;
  QString onfail;

  for (QString instr : this->m_instructions) {
    instr = instr.trimmed(); // Get rid of surrounding whitespace

    if (instr.isEmpty()) continue; // Skip empty lines
    if (instr.startsWith('#')) continue; // Skip comments

    if (instr.startsWith(QLatin1Literal("ONFAIL "), Qt::CaseInsensitive)) {
      onfail = instr.mid(7);
    } else if (instr.startsWith(QLatin1Literal("OPEN "), Qt::CaseInsensitive)) {
      QString romFile = instr.mid(5);
      std::cout << "*  Opening ROM " << romFile.toStdString() << "\n";
      exec = buildExecutor(onfail, romFile, cpuImpl, &display);

      onfail.clear();
    } else if (!exec) {
      std::cout << "!! Casette is missing OPEN instruction before any other instruction\n";
      return false;
    } else { // Normal instruciton
      std::cout << instr.toStdString() << "\n";

      if (exec->execute(instr) == false) {
        if (!onfail.isEmpty()) std::cout<< "* " << onfail.toStdString() << "\n";
        return false;
      }

      onfail.clear();
    }
  }

  return true;
}

}
