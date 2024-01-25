CONFIG += testcase
CONFIG += parallel_test
# Allow more time since examples are compiled, which may take longer on Windows.
win32:testcase.timeout=900
TARGET = tst_qmake
HEADERS += testcompiler.h
SOURCES += tst_qmake.cpp testcompiler.cpp
QT = core testlib

cross_compile: DEFINES += QMAKE_CROSS_COMPILED
debug_and_release {
    CONFIG(debug, debug|release): \
        DEFINES += DEBUG_BUILD
    else: \
        DEFINES += RELEASE_BUILD
}

# Ensures `DESTDIR` gets affixed now, if ever (see `mkspecs/README.md`).
load(exclusive_builds_post)
# Makes absolute for later copy calls.
DESTDIR = $$shadowed($$DESTDIR)

# TRACE/qmake rules: XD forbids building inside of qtbase (sub-)folder #2,
# hence the "testdata" gets copied to beside of `tst_qmake` executable.
isEmpty(BUILDS)|build_pass {
    copy_dir($$PWD/testdata, $$DESTDIR/testdata)
    TESTDATA += $$DESTDIR/testdata/*
}
