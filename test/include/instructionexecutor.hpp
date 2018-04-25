#ifndef TEST_INSTRUCTIONEXECUTOR_HPP
#define TEST_INSTRUCTIONEXECUTOR_HPP

#include <core/runner.hpp>
#include <memory>

namespace Test {
class DisplayStore;

/**
 * Executor for casette instructions.
 */
class InstructionExecutor {
public:
  InstructionExecutor(std::unique_ptr<Core::Runner> runner, DisplayStore *display);
  ~InstructionExecutor();

  /**
   * Executes the \a instruction.  If it succeeds returns \c true.
   */
  bool execute(const QString &instruction);

private:
  bool advance(QStringList &args);
  bool compare(const QStringList &args);
  bool saveFrame(const QStringList &args);

  QString replaceVariables(QString templ);

  std::unique_ptr<Core::Runner> m_runner;
  DisplayStore *m_display;
};
}

#endif // TEST_INSTRUCTIONEXECUTOR_HPP
