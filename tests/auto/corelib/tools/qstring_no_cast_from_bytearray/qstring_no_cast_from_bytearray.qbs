import qbs

QtAutotest {
    name: "tst_qstring_no_cast_from_bytearray"
    cpp.defines: base.concat(["QT_NO_CAST_FROM_BYTEARRAY"])
    files: [
        "tst_qstring_no_cast_from_bytearray.cpp",
    ]
}
