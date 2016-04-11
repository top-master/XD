import qbs

QtExample {
    name: "pnp_basictools"
    condition: Qt.widgets.present && !project.disabledFeatures.contains("library")
    type: ["staticlibrary"]
    destinationDirectory: project.buildDirectory + "/examples/widgets/tools/plugandpaint/plugins"
    installDir: project.examplesInstallDir + "/widgets/tools/plugandpaint/plugins"

    Depends { name: "Qt.widgets"; required: false }

    cpp.includePaths: base.concat("../..")
    cpp.defines: base.concat(["QT_PLUGIN", "QT_STATICPLUGIN"])

    files: [
        "basictoolsplugin.cpp",
        "basictoolsplugin.h",
    ]
}
