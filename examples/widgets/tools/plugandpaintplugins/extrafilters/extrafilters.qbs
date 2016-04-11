import qbs

QtExample {
    name: "pnp_extrafilters"
    condition: Qt.widgets.present && !project.disabledFeatures.contains("library")
    type: ["dynamiclibrary"]
    destinationDirectory: project.buildDirectory + "/examples/widgets/tools/plugandpaint/plugins"
    installDir: project.examplesInstallDir + "/widgets/tools/plugandpaint/plugins"

    Depends { name: "Qt.widgets"; required: false }

    cpp.includePaths: base.concat("../..")

    files: [
        "extrafiltersplugin.cpp",
        "extrafiltersplugin.h",
    ]
}

// CONFIG       += plugin
