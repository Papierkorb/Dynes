#include <dynarec/configuration.hpp>

#include <QByteArray>

namespace Dynarec {

// Helper to check if an env var contains a true value.
static bool envBoolConfig(const char *varName, bool defaultValue) {
  QByteArray value = qgetenv(varName);

  if (value.isEmpty()) {
    return defaultValue;
  } else if (value == "1" || value == "true" || value == "t") {
    return true;
  } else {
    return false;
  }
}

// GCC doesn't (yet) support initializing globals using complex/non-trivial
// initialization through default designation constructors.
// As work-around, we use a dummy structure, which can be called easily at
// program start by the compiler.
//
// Error message: `sorry, unimplemented: non-trivial designated initializers not supported`

#if 1
Configuration CONFIGURATION;

struct ConfigurationBootstrapper {
  ConfigurationBootstrapper() {
    Configuration defaultConf;

    CONFIGURATION.optimize = envBoolConfig("DYNAREC_OPTIMIZE", defaultConf.optimize);
    CONFIGURATION.dump = envBoolConfig("DYNAREC_DUMP", defaultConf.dump);
    CONFIGURATION.trace = envBoolConfig("DYNAREC_TRACE", defaultConf.trace);
    CONFIGURATION.verboseTrace = envBoolConfig("DYNAREC_TRACE_VERBOSE", defaultConf.verboseTrace);
  }
};

static ConfigurationBootstrapper bootstrap;
#else
static Configuration defaultConf;
Configuration CONFIGURATION {
  .optimize = envBoolConfig("DYNAREC_OPTIMIZE", defaultConf.optimize),
  .dump     = envBoolConfig("DYNAREC_DUMP", defaultConf.dump),
  .trace    = envBoolConfig("DYNAREC_TRACE", defaultConf.trace),
};
#endif
}
