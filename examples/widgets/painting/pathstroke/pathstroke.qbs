import qbs

QtExample {
    name: "pathstroke"
    condition: painting_examples_shared.present

    Depends { name: "painting_examples_shared"; required: false }

    files: [
        "main.cpp",
        "pathstroke.cpp",
        "pathstroke.h",
        "pathstroke.qrc",
    ]
}
