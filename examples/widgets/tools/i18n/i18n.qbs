import qbs

QtExample {
    name: "i18n"
    condition: Qt.widgets.present &&  (!cpp.defines || !cpp.defines.contains("QT_NO_TRANSLATION")) // FIXME

    Depends { name: "Qt.widgets"; required: false }

    files: [
        "i18n.qrc",
        "languagechooser.cpp",
        "languagechooser.h",
        "main.cpp",
        "mainwindow.cpp",
        "mainwindow.h",
    ]

    Group {
        name: "translations"
        prefix: "translations/"
        files: [
            "i18n_ar.ts",
            "i18n_cs.ts",
            "i18n_de.ts",
            "i18n_el.ts",
            "i18n_en.ts",
            "i18n_eo.ts",
            "i18n_fr.ts",
            "i18n_it.ts",
            "i18n_jp.ts",
            "i18n_ko.ts",
            "i18n_no.ts",
            "i18n_ru.ts",
            "i18n_sv.ts",
            "i18n_zh.ts",
        ]
    }
}
