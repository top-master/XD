# Copyright (C) 2015 The XD Company Ltd.
# SPDX-License-Identifier: Apache-2.0

# Formatter tool (format).
#
# Applies the formatting rules to C and C++ files, all at once, and reports
# what a rule cannot fix on its own; see README.md for its usage.
#
# Links the static QtCore when one is built for this build's kind (debug or
# release, memory-safe or not); hence the tool runs without the shared Qt
# libraries. Without such a static library, it links the shared ones.
QT = core-private
DEFINES += QT_NO_CAST_FROM_ASCII

# formatStaticMissing(keys)
#
# Names the first module of `keys` (`QT` keywords) whose static library is
# not built, or built for another memory-safe mode; prints nothing when every
# module of `keys` has a fitting one. Call it only while `qt_static` is set.
defineReplace(formatStaticMissing) {
    for(key, 1) {
        id = $$qtModuleIdFromKey($$key)
        name = $$eval(QT.$${id}.name)
        isEmpty(name): return($$key)
        lib = $$[QT_INSTALL_LIBS]/$${QMAKE_PREFIX_STATICLIB}$$qt5LibraryTarget($$name)
        !exists($${lib}.$${QMAKE_EXTENSION_STATICLIB})|!exists($${lib}.prl): return($$key)
        built = $$fromfile($${lib}.prl, QMAKE_PRL_CONFIG)
        contains(built, memory_safe): !memory_safe: return($$key)
        !contains(built, memory_safe): memory_safe: return($$key)
    }
    return()
}

# formatUseStatic(keys)
#
# Turns on `qt_static` when every module of `keys` has a fitting static
# library, and leaves `CONFIG` and `DEFINES` as they were otherwise.
defineTest(formatUseStatic) {
    qt_static: return(true)
    config = $$CONFIG
    defines = $$DEFINES
    CONFIG += qt_static
    load(qt_static_resolver)
    missing = $$formatStaticMissing($$1)
    isEmpty(missing): return(true)

    !build_pass: message("Links the shared Qt libraries: no fitting static $$missing module.")
    CONFIG = $$config
    DEFINES = $$defines
    export(CONFIG)
    export(DEFINES)
    load(qt_static_resolver)
    return(false)
}

formatUseStatic($$QT)

include(format.pri)

SOURCES += main.cpp

load(qt_tool)

# Finds the shared Qt libraries in the `lib` folder beside the tool's `bin`
# folder, when it links them and this Qt build names no run path itself.
!qt_static:!contains(QT_CONFIG, rpath): \
    QMAKE_RPATHDIR += $$relative_path($$[QT_INSTALL_LIBS], $$[QT_INSTALL_BINS])

OTHER_FILES += README.md
