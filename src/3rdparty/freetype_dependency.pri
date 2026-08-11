contains(QT_CONFIG, freetype) {
    INCLUDEPATH += $$PWD/freetype/include
    LIBS_PRIVATE += -L$$QT_BUILD_TREE/lib -lqtfreetype$$qtPlatformTargetSuffix()
    # freetype's ftgzip.o needs zlib named AFTER freetype (GNU ld is order-
    # sensitive); name QtCore's zlib with qtModuleLib(core) for versionless -l.
    contains(QT_CONFIG, system-zlib) {
        LIBS_PRIVATE += -lz
    } else {
        LIBS_PRIVATE += -L$$QT_BUILD_TREE/lib -l$$qtModuleLib(core)
    }
} else:contains(QT_CONFIG, system-freetype) {
    # pull in the proper freetype2 include directory
    include($$QT_SOURCE_TREE/config.tests/unix/freetype/freetype.pri)
}
