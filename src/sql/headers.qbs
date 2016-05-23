import qbs

Group {
    name: "headers"
    files: [
        project.qtbasePrefix + "src/sql/drivers/**/*.h",
        project.qtbasePrefix + "src/sql/kernel/*.h",
        project.qtbasePrefix + "src/sql/models/*.h",
    ]
    overrideTags: true
}
