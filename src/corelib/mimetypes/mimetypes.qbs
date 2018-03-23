import qbs

Group {
    name: "mimetypes"
    condition: config.mimetype
    prefix: path + '/'
    files: [
        "mimetypes.qrc",
        "qmimedatabase.cpp",
        "qmimedatabase.h",
        "qmimedatabase_p.h",
        "qmimeglobpattern.cpp",
        "qmimeglobpattern_p.h",
        "qmimemagicrule.cpp",
        "qmimemagicrule_p.h",
        "qmimemagicrulematcher.cpp",
        "qmimemagicrulematcher_p.h",
        "qmimeprovider.cpp",
        "qmimeprovider_p.h",
        "qmimetype.cpp",
        "qmimetype.h",
        "qmimetype_p.h",
        "qmimetypeparser.cpp",
        "qmimetypeparser_p.h",
    ]
}
