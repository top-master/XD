import qbs

StaticLibrary {
    condition: project.xcb
    builtByDefault: false
    destinationDirectory: project.libDirectory
    profiles: project.targetProfiles

    readonly property path basePath: project.qtbasePrefix + "src/3rdparty/xcb"

    Depends { name: "cpp" }
    //Depends { name: "sm" }
    //Depends { name: "x11" }
    //Depends { name: "xcb-iccm" }
    //Depends { name: "xcb-image" }
    //Depends { name: "xcb-keysyms" }
    //Depends { name: "xcb-randr" }
    //Depends { name: "xcb-sync" }
    //Depends { name: "xcb-xfixes" }

    cpp.cFlags: [
        "-Wno-implicit-function-declaration",
        "-Wno-return-type",
        "-Wno-sign-compare",
        "-Wno-tautological-compare",
        "-Wno-unused-function",
        "-Wno-unused-parameter",
    ].concat(base)

    cpp.includePaths: [
        basePath + "/include",
        basePath + "/include/xcb",
        basePath + "/sysinclude",
    ].concat(base)

    Group {
        name: "headers"
        files: basePath + "/include/xcb/*.h"
    }

    Group {
        name: "sources"
        prefix: basePath + "/"
        files: [
            "libxcb/*.c",
            "xcb-util/*.c",
            "xcb-util-image/*.c",
            "xcb-util-keysyms/*.c",
            "xcb-util-renderutil/*.c",
            "xcb-util-wm/*.c",
        ]
    }

    Export {
        Depends { name: "cpp" }
        cpp.includePaths: [
            project.qtbasePrefix + "src/3rdparty/xcb/include",
            project.qtbasePrefix + "src/3rdparty/xcb/sysinclude",
        ]
    }
}
