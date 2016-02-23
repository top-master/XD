import qbs

Group {
    name: "headers"
    prefix: project.qtbasePrefix + "src/network/"
    files: "**/*.h"
    excludeFiles: "doc/**"
}
