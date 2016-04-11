import qbs

Product {
    name: "examples (deployment)"
    profiles: [project.hostProfile] // TODO: Knowledge of target OS required below.
    Group {
        files: [
            "**"
        ]
        excludeFiles: [
            "**/*.qbs",
            "**/doc/**/*",
            "aggregate/examples.pro",

            // The following are not installed when building with qmake, though I don't see
            // in all cases how they are excluded and whether that is on purpose.
            "corelib/threads/queuedcustomtype/*",
            "embedded/**",
            "opengl/legacy/**",
            "qmake/**",
            "qtestlib/tutorial5/containers.cpp",
            "widgets/dialogs/sipdialog/**",
        ].concat(!qbs.targetOS.contains("darwin") ? ["widgets/mac/**"] : [])
        qbs.install: true
        qbs.installDir: project.examplesInstallDir
        qbs.installSourceBase: sourceDirectory
    }
}
