
#include "./qremote-logging.h"

QBasicAtomicFlags<QRemote::LogTypes> QRemote::logSkip = Q_BASIC_ATOMIC_INITIALIZER(0);
