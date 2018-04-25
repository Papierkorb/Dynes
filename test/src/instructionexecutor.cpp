#include <instructionexecutor.hpp>

#include <core/runner.hpp>
#include <cpu/memory.hpp>

#include <bmpfile.hpp>
#include <displaystore.hpp>

#include <QFile>
#include <iostream>

namespace Test {
InstructionExecutor::InstructionExecutor(std::unique_ptr<Core::Runner> runner, DisplayStore *display)
  : m_runner(std::move(runner)), m_display(display)
{

}

InstructionExecutor::~InstructionExecutor() {
  //
}

bool InstructionExecutor::execute(const QString &instruction) {
  QStringList parts = instruction.split(' ', QString::SkipEmptyParts);
  QString command = parts.takeFirst().toLower();

  if (command == "advance") return this->advance(parts);
  if (command == "compare") return this->compare(parts);
  if (command == "frame") return this->saveFrame(parts);

  std::cout << "!! ERROR: Unknown command " << command.toStdString() << "\n";
  return false;
}

static void parseInput(Core::Gamepad &pad, const QStringList &args) {
  for (QString arg : args) {
    arg = arg.toLower();

    if (arg == "up") pad.setUp(true);
    else if (arg == "down") pad.setDown(true);
    else if (arg == "left") pad.setLeft(true);
    else if (arg == "right") pad.setRight(true);
    else if (arg == "a") pad.setA(true);
    else if (arg == "b") pad.setB(true);
    else if (arg == "start") pad.setStart(true);
    else if (arg == "select") pad.setSelect(true);
    else std::cout << "  Unknown input button " << arg.toStdString() << "\n";
  }
}

bool InstructionExecutor::advance(QStringList &args) {
  Core::Gamepad &input = this->m_runner->ram()->firstPlayer();
  int frameCount = 1;

  input.reset();

  // The first argument could be the frame count
  if (!args.isEmpty()) {
    bool ok = false;
    frameCount = args.first().toInt(&ok);

    if (ok) args.removeFirst();
    else frameCount = 1;
  }

  // Read input arguments
  parseInput(input, args);

  // Advance the emulator the wanted amount of frames
  for (int i = 0; i < frameCount; i++) {
    this->m_runner->tick();
  }

  return true; // Okay!
}

static BmpFile grabFromDisplay(DisplayStore *display) {
  return BmpFile(display->width(), display->height(), display->frame());
}

bool InstructionExecutor::compare(const QStringList &args) {
  if (args.size() != 1) {
    std::cout << "!! The COMPARE command requires a single argument.\n";
    return false;
  }

  QString fileName = this->replaceVariables(args.first());
  QFile sourceFile(fileName);
  if (!sourceFile.open(QIODevice::ReadOnly)) {
    std::cout << "!! Failed to open BMP file at " << fileName.toStdString() << " \n";
    return false;
  }

  // Read BMP:
  BmpFile expected(&sourceFile);

  // Compare:
  if (expected == grabFromDisplay(this->m_display)) return true;

  std::cout << "   The screen comparison failed!\n";
  return false;
}

bool InstructionExecutor::saveFrame(const QStringList &args) {
  if (args.size() != 1) {
    std::cout << "!! The FRAME command requires a single argument.\n";
    return false;
  }

  QString fileName = this->replaceVariables(args.first());
  QFile file(fileName);
  if (!file.open(QIODevice::WriteOnly)) {
    std::cout << "!! Failed to write BMP file to " << fileName.toStdString() << " \n";
    return false;
  }

  grabFromDisplay(this->m_display).write(&file);
  return true;
}

QString InstructionExecutor::replaceVariables(QString templ) {
  return templ.replace('%', this->m_runner->cpuImplementation());
}
}
