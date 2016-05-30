import qbs

QtAutotestProject {
    name: "corelib_io_tests"
    condition: !qbs.targetOS.contains("ios")
    references: [
        "largefile",
        "qabstractfileengine",
        "qbuffer",
        "qdatastream",
        "qdataurl",
        "qdebug",
        "qdir",
        "qdiriterator",
        "qfile",
        "qfileinfo",
        "qfileselector",
        "qfilesystementry",
        "qfilesystemwatcher",
        "qiodevice",
        "qipaddress",
        "qlockfile/qlockfile.qbs",
        "qloggingcategory",
        "qloggingregistry",
        "qnodebug",
        "qprocess",
        "qprocess-noapplication",
        "qprocessenvironment",
        "qresourceengine",
        "qsavefile",
        "qsettings",
        "qstandardpaths",
        "qstorageinfo",
        "qtemporarydir",
        "qtemporaryfile",
        "qtextstream",
        "qurl",
        "qurlinternal",
        "qurlquery",
        "qwinoverlappedionotifier",
    ]
}
