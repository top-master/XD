# This project is not actually used to build qmake, but to support development
# with Qt Creator. The real build system is made up by the Makefile templates
# and the configures.

option(host_build)
CONFIG += console bootstrap
CONFIG -= qt
DEFINES += \
    QT_BUILD_QMAKE \
    PROEVALUATOR_FULL

## Uncomment below to build with watch mode,
## which will log a warning for each change of variables (in qmake scripts),
## where "QMAKE_WATCH" filters what gets logged by variable-name.
#DEFINES += QMAKE_WATCH
contains(DEFINES, QMAKE_WATCH) {
    win*: LIBS += -lDbgHelp
    !CONFIG(debug, debug|release) {
        !build_pass: warning(QMAKE_WATCH should not be enabled for release)
    }
}
DEFINES += PROJECT_FOLDER=\\\"$$replace(PWD,\\,/)\\\"

VPATH += \
    ../src/corelib/global \
    ../src/corelib/tools \
    ../src/corelib/kernel \
    ../src/corelib/codecs \
    ../src/corelib/plugin \
    ../src/corelib/xml \
    ../src/corelib/io \
    ../src/corelib/json \
    ../tools/shared/windows

INCLUDEPATH += . \
           library \
           generators \
           generators/unix \
           generators/win32 \
           generators/mac \
           generators/integrity \
           ../tools/shared

include(qmake.pri)

load(qt_app)
