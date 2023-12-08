#ifndef MANUALSKIP_H
#define MANUALSKIP_H

#include <QtCore/qstring.h>

#ifdef Q_OS_WIN
enum enumKeys {
    keyBackspace = 0x8,
    keyTab = 0x9,
    keyReturn = 0xD,
    keyShift = 0x10,
    keyControl = 0x11,
    keyAlt = 0x12,
    keyPause = 0x13,
    keyEscape = 0x1B,
    keySpace = 0x20,
    keyEnd = 0x23,
    keyHome = 0x24,
    keyLeft = 0x25,
    KeyUp = 0x26,
    keyRight = 0x27,
    KeyDown = 0x28,
    keyInsert = 0x2D,
    keyDelete = 0x2E,
    keyF1 = 0x70,
    keyF2 = 0x71,
    keyF3 = 0x72,
    keyF4 = 0x73,
    keyF5 = 0x74,
    keyF6 = 0x75,
    keyF7 = 0x76,
    keyF8 = 0x77,
    keyF9 = 0x78,
    keyF10 = 0x79,
    keyF11 = 0x7A,
    keyF12 = 0x7B,
    keyNumLock = 0x90,
    keyScrollLock = 0x91,
    keyCapsLock = 0x14,

    keySnapshot = 0x2C,
    keyTilde = 192
};
bool skipOnKey(const QString &str = QString());
#endif


#endif // MANUALSKIP_H
