import qbs

Group {
    name: "headers"
    prefix: project.qtbasePrefix + "src/testlib/"
    files: "**/*.h"
    excludeFiles: "doc/**"
}
