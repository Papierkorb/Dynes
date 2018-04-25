#ifndef TEST_CASETTEPLAYER_HPP
#define TEST_CASETTEPLAYER_HPP

#include <QStringList>

namespace Test {

/**
 * Class to play a casette with test instructions.
 */
class CasettePlayer {
public:
  CasettePlayer(const QStringList &instructions);

  /**
   * Creates a NES environment using the given \a cpuImpl and then plays the
   * casette.  Returns \c true if it succeeded.
   */
  bool play(const QString &cpuImpl);

private:
  QStringList m_instructions;
};
}

#endif // TEST_CASETTEPLAYER_HPP
