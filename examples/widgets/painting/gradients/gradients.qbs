import qbs

QtExample {
    name: "gradients"
    condition: painting_examples_shared.present

    Depends { name: "painting_examples_shared"; required: false }

    files: [
        "gradients.cpp",
        "gradients.h",
        "gradients.qrc",
        "main.cpp",
    ]
}
