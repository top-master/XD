import qbs

Group {
    name: "xml"
    prefix: path + '/'
    files: [
        "qxmlstream.cpp",
        "qxmlstream.h",
        "qxmlstream_p.h",
        "qxmlutils.cpp",
        "qxmlutils_p.h",
    ]
}
