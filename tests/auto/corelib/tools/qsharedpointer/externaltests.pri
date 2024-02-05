SOURCES += $$PWD/externaltests.cpp
HEADERS += $$PWD/externaltests.h
cleanedQMAKESPEC = $$replace(QMAKESPEC, \\\\, /)
DEFINES += DEFAULT_MAKESPEC=\\\"$$cleanedQMAKESPEC\\\"

cross_compile:DEFINES += QTEST_NO_RTTI QTEST_CROSS_COMPILED

DEFINES += EXTERNAL_TESTS_FOLDER=\\\"$$replace(PWD,\\\\,/)\\\"

OTHER_FILES += \
    $$PWD/externaltests-launcher.cpp
