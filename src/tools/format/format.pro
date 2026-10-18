# Copyright (C) 2015 The XD Company Ltd.
# SPDX-License-Identifier: Apache-2.0

# Formatter tool (format).
#
# Applies the formatting rules to C and C++ files, all at once, and reports
# what a rule cannot fix on its own; see README.md for its usage.
QT = core-private
DEFINES += QT_NO_CAST_FROM_ASCII

include(format.pri)

SOURCES += main.cpp

load(qt_tool)

# Finds the shared Qt libraries in the `lib` folder beside the tool's `bin`
# folder, when it links them and this Qt build names no run path itself.
!qt_static:!contains(QT_CONFIG, rpath): \
    QMAKE_RPATHDIR += $$relative_path($$[QT_INSTALL_LIBS], $$[QT_INSTALL_BINS])

OTHER_FILES += README.md
