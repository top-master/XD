import qbs

QtPlugin {
    condition: project.eglfs_x11
    category: "egldeviceintegrations"

    Depends { name: "xcb" }
    Depends { name: "x11-xcb" }
    Depends { name: "Qt"; submodules: ["core", "gui", "gui-private", "egldeviceintegration"] }

    cpp.defines: [
        "MESA_EGL_NO_X11_HEADERS",
    ].concat(base)

    cpp.includePaths: [
        path + "/../..",
    ].concat(base)

    files: [
        "eglfs_x11.json",
        "qeglfsx11integration.cpp",
        "qeglfsx11integration.h",
        "qeglfsx11main.cpp",
    ]
}
