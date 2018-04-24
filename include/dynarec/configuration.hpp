#ifndef DYNAREC_CONFIGURATION_HPP
#define DYNAREC_CONFIGURATION_HPP

namespace Dynarec {
/**
 * Access to application global, environment variable controlled,
 * configuration.  This is mainly interesting for debugging the dynarec itself,
 * so it's not exposed to the front-end.
 *
 * \sa CONFIGURATION
 */
struct Configuration {
  /**
   * Trace the execution of all instruction at run-time?
   * Controlled by \c DYNAREC_TRACE.
   */
  bool trace = false;

  /**
   * When tracing, also show the state.
   * Controlled by \c DYNAREC_TRACE_VERBOSE.
   */
  bool verboseTrace = false;

  /**
   * Dump generated LLVM modules
   * Controlled by \c DYNAREC_DUMP.
   */
  bool dump = false;

  /**
   * Optimize generated code before execution.
   * Controlled by \c DYNAREC_OPTIMIZE.
   */
  bool optimize = true;

};

/** Configuration */
extern Configuration CONFIGURATION;
}

#endif // DYNAREC_CONFIGURATION_HPP
