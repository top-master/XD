SOURCES += $$PWD/externaltests.cpp
HEADERS += $$PWD/externaltests.h
cleanedQMAKESPEC = $$replace(QMAKESPEC, \\\\, /)
DEFINES += DEFAULT_MAKESPEC=\\\"$$cleanedQMAKESPEC\\\"

cross_compile:DEFINES += QTEST_NO_RTTI QTEST_CROSS_COMPILED

DEFINES += EXTERNAL_TESTS_FOLDER=\\\"$$replace(PWD,\\\\,/)\\\"

# The external test builds with the qmake, the configuration, and the
# compilers that built this test.
cleanedQMAKE = $$replace(QMAKE_QMAKE, \\\\, /)
DEFINES += EXTERNAL_TESTS_QMAKE=\\\"$$cleanedQMAKE\\\"
memory_safe {
    DEFINES += EXTERNAL_TESTS_MEMORY_SAFE
    externalTestsCompiler = $$system(command -v clang++)
    !isEmpty(externalTestsCompiler) {
        externalTestsToolchain = $$dirname(externalTestsCompiler)
        DEFINES += EXTERNAL_TESTS_TOOLCHAIN=\\\"$$externalTestsToolchain\\\"
    }
}

OTHER_FILES += \
    $$PWD/externaltests-launcher.cpp
