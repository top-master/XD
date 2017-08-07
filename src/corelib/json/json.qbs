import qbs

Group {
    name: "json"
    prefix: path + '/'
    files: [
        "qjsonarray.cpp",
        "qjsonarray.h",
        "qjson.cpp",
        "qjson_p.h",
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
    ]
}
