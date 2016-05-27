import qbs

QtPlugin {
    category: "xcbglintegrations"

    Depends { name: "Qt.core-private" }
    Depends { name: "Qt.gui" }
    Depends { name: "Qt.gui-private" }
    Depends { name: "Qt.platformsupport-private" }
    Depends { name: "Qt.xcbqpa" }

    cpp.includePaths: base.concat(["..", "../.."])
    cpp.defines: {
        var defines = base;
        if (project.xcb_xlib) {
            // needed by Xcursor ...
            defines.push("XCB_USE_XLIB");
            if (project.xinput2)
                defines.push("XCB_USE_XINPUT2")
        }
        if (project.xcb_render || project.config.contains("xcb-qt"))
            defines.push("XCB_USE_RENDER"); // to support custom cursors with depth > 1
        if (project.xcb_sm)
            defines.push("XCB_USE_SM"); // build with session management support
        return defines;
    }
    cpp.dynamicLibraries: {
        var libs = base.concat(["xcb"]);
        if (project.config.contains("xcb-qt")) {
            //            XCB_DIR = $$clean_path($$PWD/../../../../3rdparty/xcb)
            //            INCLUDEPATH += $$XCB_DIR/include $$XCB_DIR/include/xcb $$XCB_DIR/sysinclude
            //            LIBS += -lxcb -L$$OUT_PWD/xcb-static -lxcb-static
        } else {
            libs.push("xcb-image", "xcb-icccm", "xcb-sync", "xcb-xfixes", "xcb-shm", "xcb-randr",
                      "xcb-shape", "xcb-keysyms");
            // !contains(DEFINES, QT_NO_XKB):LIBS += -lxcb-xkb
        }
        return libs;
    }
}

// CONFIG += qpa/genericunixfontdatabase
