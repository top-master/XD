import qbs

Group {
    name: "headers"
    files: [
        project.qtbasePrefix + "src/platformsupport/**/*.h",
    ]
}
