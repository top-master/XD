import qbs

QtExample {
    name: "classwizard"
    condition: Qt.widgets.present &&

               // FIXME: We just took this pattern from the qmake project files. Of course,
               //        this is horribly backwards and should be done with properties instead.
               //        Also, it does not even work, as not all defines are known at this point.
               (!cpp.defines || !cpp.defines.contains("QT_NO_WIZARD"))

    Depends { name: "Qt.widgets"; required: false }

    files: [
        "classwizard.cpp",
        "classwizard.h",
        "classwizard.qrc",
        "main.cpp",
    ]
}
