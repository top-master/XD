import qbs

QtExample {
    name: "deform"
    condition: painting_examples_shared.present

    Depends { name: "painting_examples_shared"; required: false }

    files: [
        "deform.qrc",
        "main.cpp",
        "pathdeform.cpp",
        "pathdeform.h",
    ]
}
