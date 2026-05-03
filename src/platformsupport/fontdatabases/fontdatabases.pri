# TRACE/platformsupport mac: compile QBasicFontDatabase on Mac too,
# since `qminimal` (the headless QPA plugin under
# `src/plugins/platforms/minimal/qminimalintegration.cpp`) references it
# unconditionally; the bundled `libqtfreetype.a` covers the freetype
# prerequisite.
!win32|contains(QT_CONFIG, freetype) {
    include($$PWD/basic/basic.pri)
}

unix:!mac {
    CONFIG += qpa/genericunixfontdatabase
    include($$PWD/genericunix/genericunix.pri)
    contains(QT_CONFIG,fontconfig) {
        include($$PWD/fontconfig/fontconfig.pri)
    }
}

mac {
    include($$PWD/mac/coretext.pri)
}

