load(qt_build_config)
load(qfeatures)

sourcePrefix = $$clean_path($$PWD/../..)
configPath = $$OUT_PWD

FULL_CONFIG = $$CONFIG $$QT_CONFIG
qmakeConfig = $$join(FULL_CONFIG, "\", \"", "\"", "\"")

disabledFeatures = $$join(QT_DISABLED_FEATURES, "\", \"", "\"", "\"")

qbs.input = $$PWD/qt.qbs.in
qbs.output = $$OUT_PWD/qt.qbs
QMAKE_SUBSTITUTES = qbs

# limit what else ends up happening
QT =
CONFIG =
MAKEFILE = QbsDummyMakefile

defineReplace(makeList) {
    spaceSeparatedValues = "$$1"
    jsList = '[''"'$$replace(spaceSeparatedValues,' ','", "')'"'']'
    return($$system_quote("$$jsList"))
}

# TODO: calling qbs tools probably cannot be the real solution (it doesn't really work with
#       Qt Creator, for instance). Put some thought into the concept of "anonymous profiles"
#       or something along these lines.
defineReplace(setupProfile) {
    type = $$1
    equals(type, host):mkspec = $$[QMAKE_SPEC]
    else:mkspec = $$[QMAKE_XSPEC]
    profileName = qt-build-$$type-$$QT_VERSION-$$mkspec
    profileName = $$replace(profileName,\+,-)
    profileName = $$replace(profileName,\.,-)
    profileName = $$replace(profileName,/,-)

    QMAKE_CFLAGS =
    QMAKE_CXXFLAGS =
    DEFINES =
    QMAKE_COMPILER_DEFINES =
    QMAKE_LFLAGS =
    QMAKE_LFLAGS_WINDOWS =
    include($$sourcePrefix/qtbase/mkspecs/$$mkspec/qmake.conf)

    compiler = $$replace(QMAKE_CXX, \bg\+\+$, gcc)
    compiler = $$replace(compiler, clang\+\+$, clang)
    contains(type, target):contains(QMAKE_PLATFORM, android) {
        system(qbs-setup-android --ndk-dir $$NDK_ROOT --sdk-dir $$ANDROID_SDK_ROOT $$profileName)
    } else {
        system(qbs-setup-toolchains $$compiler $$profileName)
    }

    !isEmpty(QMAKE_CFLAGS): system(qbs-config profiles.$${profileName}.cpp.platformCFlags $$makeList($$QMAKE_CFLAGS))
    !isEmpty(QMAKE_CXXFLAGS): system(qbs-config profiles.$${profileName}.cpp.platformCxxFlags $$makeList($$QMAKE_CXXFLAGS))
    !isEmpty(DEFINES): system(qbs-config profiles.$${profileName}.cpp.defines $$makeList($$DEFINES))
    !isEmpty(QMAKE_COMPILER_DEFINES): system(qbs-config profiles.$${profileName}.cpp.compilerDefines $$makeList($$QMAKE_COMPILER_DEFINES))
    lFlags = $$QMAKE_LFLAGS
    !isEmpty(lFlags): system(qbs-config profiles.$${profileName}.cpp.platformLinkerFlags $$makeList($$lFlags))
    equals(type, target): system(qbs-config profiles.$${profileName}.qbs.sysroot $$[QT_SYSROOT])

    return($$profileName)
}

contains(QMAKE_PLATFORM, android) {
    for (arch, ANDROID_ARCHITECTURE) {
        equals(arch, arm): arch = armv7
        targetArchitectures += $${arch}
    }
}

HOST_PROFILE = $$setupProfile(host)
TARGET_PROFILE = $$setupProfile(target)
TARGET_ARCHITECTURES = $$join(targetArchitectures, "\", \"", "\"", "\"")
