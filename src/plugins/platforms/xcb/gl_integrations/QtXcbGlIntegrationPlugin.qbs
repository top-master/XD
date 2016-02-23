import qbs

QtPlugin {
    targetName: name + "-integration"
    readonly property string basePath: project.qtbasePrefix + "src/plugins/platforms/xcb"

    category: "xcbglintegrations"

    cpp.includePaths: [
        basePath,
    ].concat(base)

    Depends { name: "xcb" }
    Depends { name: "QtXcbQpa" }
    Depends { name: "Qt"; submodules: ["core", "core-private", "gui", "gui-private", "platformheaders", "platformsupport-private"] }
}
