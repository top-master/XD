import qbs

QtExample {
    name: "affine"
    condition: painting_examples_shared.present

    Depends { name: "painting_examples_shared"; required: false }

    files: [
        "affine.qrc",
        "main.cpp",
        "xform.cpp",
        "xform.h",
    ]
}
