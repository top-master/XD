option(host_build)
CONFIG += force_bootstrap

DEFINES += QT_MOC QT_NO_CAST_FROM_ASCII QT_NO_CAST_FROM_BYTEARRAY QT_NO_COMPRESS

## Other defines were moved to `moc-config.h` file.
#DEFINES += DEBUG_MOC

include(moc.pri)
HEADERS += $$PWD/../../corelib/tools/qdatetime_p.h \
    $$PWD/../../../qmake/library/udebug.h
SOURCES += main.cpp \
    $$PWD/../../../qmake/library/udebug.cpp

load(qt_tool)

OTHER_FILES	+= \
        $$PWD/../../../mkspecs/features/moc.prf \
        keywords.cpp ppkeywords.cpp
