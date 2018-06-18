import qbs

Group {
    name: "serialization"
    prefix: path + '/'
    files: [
        "qcborarray.h",
        "qcborcommon.h",
        "qcbormap.h",
        "qcborstream.cpp",
        "qcborstream.h",
        "qcborvalue.cpp",
        "qcborvalue.h",
        "qcborvalue_p.h",
        "qdatastream.cpp",
        "qdatastream.h",
        "qdatastream_p.h",
        "qjson.cpp",
        "qjson_p.h",
        "qjsonarray.cpp",
        "qjsonarray.h",
        "qjsoncbor.cpp",
        "qjsondocument.cpp",
        "qjsondocument.h",
        "qjsonobject.cpp",
        "qjsonobject.h",
        "qjsonparser.cpp",
        "qjsonparser_p.h",
        "qjsonvalue.cpp",
        "qjsonvalue.h",
        "qjsonwriter.cpp",
        "qjsonwriter_p.h",
        "qtextstream.cpp",
        "qtextstream.h",
        "qtextstream_p.h",
        "qxmlstream.cpp",
        "qxmlstream.h",
        "qxmlstream_p.h",
        "qxmlutils.cpp",
        "qxmlutils_p.h",
    ]
    Group {
        fileTags: ["hpp"]
        files: [
            "qcborarray.cpp",
            "qcbormap.cpp",
        ]
    }
}
