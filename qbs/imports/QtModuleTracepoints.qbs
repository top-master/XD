import qbs

Product {
    type: ["hpp", "cpp"]
    name: project.tracepointsProductName
    multiplexByQbsProperties: ["profiles"]
    property string hostProfile: "qt_hostProfile"
    qbs.profiles: [hostProfile]
    Depends { name: "qt_tracepoints" }
    files: [
        "qt" + project.simpleName + ".tracepoints",
    ]
    Export {
        Depends { name: "cpp" }
        cpp.includePaths: product.buildDirectory
    }
}
