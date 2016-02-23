import qbs

Group {
    name: "headers"
    prefix: project.qtbasePrefix + "src/xml/"
    files: "**/*.h"
    excludeFiles: "doc/**"
}
