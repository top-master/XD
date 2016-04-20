load(qt_build_config)
load(qfeatures)

sourcePrefix = $$clean_path($$PWD/../..)
configPath = $$OUT_PWD

FULL_CONFIG = $$CONFIG $$QT_CONFIG
qmakeConfig = $$join(FULL_CONFIG, "\", \"", "\"", "\"")

disabledFeatures = $$join(QT_DISABLED_FEATURES, "\", \"", "\"", "\"")
sqlPlugins = $$join(sql-plugins, "\", \"", "\"", "\"")
qtCFlagsMysql = $$join(QT_CFLAGS_MYSQL, "\", \"", "\"", "\"")
qtLFlagsMysql = $$join(QT_LFLAGS_MYSQL, "\", \"", "\"", "\"")
qtLFlagsOdbc = $$join(QT_LFLAGS_ODBC, "\", \"", "\"", "\"")
qtCFlagsPsql = $$join(QT_CFLAGS_PSQL, "\", \"", "\"", "\"")
qtLFlagsPsql = $$join(QT_LFLAGS_PSQL, "\", \"", "\"", "\"")
qtCFlagsSqlite = $$join(QT_CFLAGS_SQLITE, "\", \"", "\"", "\"")
qtLFlagsSqlite = $$join(QT_LFLAGS_SQLITE, "\", \"", "\"", "\"")
qtCFlagsTds = $$join(QT_CFLAGS_TDS, "\", \"", "\"", "\"")
qtLFlagsTds = $$join(QT_LFLAGS_TDS, "\", \"", "\"", "\"")

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
    QMAKE_CFLAGS_SSE2 =
    QMAKE_CFLAGS_SSE3 =
    QMAKE_CFLAGS_SSSE3 =
    QMAKE_CFLAGS_SSE4_1 =
    QMAKE_CFLAGS_SSE4_2 =
    QMAKE_CFLAGS_AVX =
    QMAKE_CFLAGS_AVX2 =
    QMAKE_CFLAGS_NEON =
    QMAKE_CXXFLAGS =
    DEFINES =
    QMAKE_COMPILER_DEFINES =
    QMAKE_LFLAGS =
    QMAKE_LFLAGS_WINDOWS =
    include($$sourcePrefix/qtbase/mkspecs/$$mkspec/qmake.conf)

    sse2Flags = $$join(QMAKE_CFLAGS_SSE2, "\", \"", "\"", "\"")
    export(sse2Flags)
    sse3Flags = $$join(QMAKE_CFLAGS_SSE3, "\", \"", "\"", "\"")
    export(sse3Flags)
    ssse3Flags = $$join(QMAKE_CFLAGS_SSSE3, "\", \"", "\"", "\"")
    export(ssse3Flags)
    sse4_1Flags = $$join(QMAKE_CFLAGS_SSE4_1, "\", \"", "\"", "\"")
    export(sse4_1Flags)
    sse4_2Flags = $$join(QMAKE_CFLAGS_SSE4_2, "\", \"", "\"", "\"")
    export(sse4_2Flags)
    avxFlags = $$join(QMAKE_CFLAGS_AVX, "\", \"", "\"", "\"")
    export(avxFlags)
    avx2Flags = $$join(QMAKE_CFLAGS_AVX2, "\", \"", "\"", "\"")
    export(avx2Flags)
    neonFlags = $$join(QMAKE_CFLAGS_NEON, "\", \"", "\"", "\"")
    export(neonFlags)

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
qmake_spec = $$[QMAKE_SPEC]
equals(qmake_spec, $$[QMAKE_XSPEC]):TARGET_PROFILE = $$HOST_PROFILE
else:TARGET_PROFILE = $$setupProfile(target)
TARGET_ARCHITECTURES = $$join(targetArchitectures, "\", \"", "\"", "\"")
