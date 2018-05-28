import qbs

QtAutotestProject {
    name: "corelib_io_tests"
    references: [
        "largefile",
        "qabstractfileengine",
        "qbuffer",
        "qdataurl",
        "qdebug",
        "qdir",
        "qdiriterator",
        "qfile/qfile.qbs",
        "qfileinfo",
        "qfileselector",
        "qfilesystementry",
        "qfilesystemmetadata",
        "qfilesystemwatcher",
        "qiodevice",
        "qipaddress",
        "qlockfile/qlockfile.qbs",
        "qloggingcategory",
        "qloggingregistry",
        "qnodebug",
        "qprocess/qprocess.qbs",
        "qprocess-noapplication",
        "qprocessenvironment",
        "qresourceengine",
        "qsavefile",
        "qsettings",
        "qstandardpaths",
        "qstorageinfo",
        "qtemporarydir",
        "qtemporaryfile",
        "qurl",
        "qurlinternal",
        "qurlquery",
    ]
}
