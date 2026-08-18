
#include "./qremote-logging.h"

QBasicAtomicFlags<QRemote::LogTypes> QRemote::logSkip = Q_BASIC_ATOMIC_FLAGS(0);
