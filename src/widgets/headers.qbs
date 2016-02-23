import qbs

Group {
    name: "headers"
    prefix: project.qtbasePrefix + "src/widgets/"
    files: "**/*.h"
    excludeFiles: "doc/**"
}
