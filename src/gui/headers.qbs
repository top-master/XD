import qbs

Group {
    name: "headers"
    prefix: project.qtbasePrefix + "src/gui/"
    files: "**/*.h"
    excludeFiles: "doc/**"
}
