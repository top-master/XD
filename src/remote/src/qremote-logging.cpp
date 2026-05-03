
#include "./qremote-logging.h"

// TRACE/corelib #Q_BASIC_ATOMIC_INITIALIZER; nest braces.
QBasicAtomicFlags<QRemote::LogTypes> QRemote::logSkip = { { 0 } };
