contains(QT_CONFIG, freetype) {
    INCLUDEPATH += $$PWD/freetype/include
    LIBS_PRIVATE += -L$$QT_BUILD_TREE/lib -lqtfreetype$$qtPlatformTargetSuffix()
    # freetype's ftgzip.o references zlib. GNU-style linkers (gcc's ld/gold,
    # clang's lld, and the MinGW ld) pull static-archive members left to right, on
    # demand, and freetype is listed after its zlib provider, so the z_* symbols
    # stay unresolved unless the provider is named again AFTER freetype. Qt's
    # bundled zlib is compiled into QtCore; a system zlib is -lz. (MSVC's linker
    # resolves regardless of order, so it only bites GNU toolchains -- including a
    # static-Qt gcc/clang build, not just MinGW.)
    # Name QtCore with qtModuleLib(core), which turns a QT keyword into its `-l`
    # lib name (Qt5Core / Qt5Cored) -- no hardcoded version, and no
    # QT.core.module (which a qt_static build renames and a static-only build may
    # not have set). See qtModuleLib in mkspecs/features/qt_functions.prf.
    contains(QT_CONFIG, system-zlib) {
        LIBS_PRIVATE += -lz
    } else {
        LIBS_PRIVATE += -L$$QT_BUILD_TREE/lib -l$$qtModuleLib(core)
    }
} else:contains(QT_CONFIG, system-freetype) {
    # pull in the proper freetype2 include directory
    include($$QT_SOURCE_TREE/config.tests/unix/freetype/freetype.pri)
}
