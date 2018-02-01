import qbs

/*
 * Statically compile in code for
 * libxcb-fixes, libxcb-randr, libxcb-shm, libxcb-sync, libxcb-image,
 * libxcb-keysyms, libxcb-icccm, libxcb-renderutil, libxcb-xkb,
 * libxcb-xinerama
 */
QtStaticLibrary {
    condition: project.conditionFunction() && !QtGuiPrivateConfig.system_xcb
    Depends { name: "QtGuiPrivateConfig" }
    property string xcbDir: project.qtbaseDir + "/src/3rdparty/xcb/"
    Export {
        Depends { name: "cpp" }
        cpp.includePaths: [
            product.xcbDir + "include",
            product.xcbDir + "sysinclude",
        ]
    }
    cpp.includePaths: [
        xcbDir + "include",
        xcbDir + "sysinclude",
        xcbDir + "include/xcb",
    ]
    cpp.warningLevel: "none"
    Group {
        prefix: xcbDir
        files: [
            "README",
        ]
    }
    Group {
        prefix: xcbDir + "libxcb/"
        files: [
            "randr.c",
            "render.c",
            "shape.c",
            "shm.c",
            "sync.c",
            "xfixes.c",
            "xinerama.c",
            "xkb.c",
        ]
    }
    Group {
        prefix: xcbDir + "xcb-util/"
        files: [
            "atoms.c",
            "event.c",
            "xcb_aux.c",
        ]
    }
    Group {
        prefix: xcbDir + "xcb-util-image/"
        files: [
            "xcb_image.c",
        ]
    }
    Group {
        prefix: xcbDir + "xcb-util-keysyms/"
        files: [
            "keysyms.c",
        ]
    }
    Group {
        prefix: xcbDir + "xcb-util-renderutil/"
        files: [
            "util.c",
        ]
    }
    Group {
        prefix: xcbDir + "xcb-util-wm/"
        files: [
            "icccm.c",
        ]
    }

}

/*
QMAKE_USE += xcb/nolink
*/
