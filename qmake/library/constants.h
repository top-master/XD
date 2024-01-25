#ifndef QMAKE_CONSTANTS_H
#define QMAKE_CONSTANTS_H

#include <QString>

namespace Constants {

inline QString superfileReason() {
    return QString(QStringLiteral(
        "since this file is in the build directory of your project or one of its parent directories it pre includes to your project's .pro file."
    ));
}

}

#endif //QMAKE_CONSTANTS_H
