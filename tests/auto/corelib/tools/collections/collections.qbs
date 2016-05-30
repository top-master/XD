import qbs

QtAutotest {
    name: "tst_collections"
    files: [
        "tst_collections.cpp",
    ]
}

// DEFINES -= QT_STRICT_ITERATORS
