import qbs

QtAutotest {
    name: "tst_qpropertyanimation"
    customCondition: Qt.global.privateConfig.widgets

    Depends { name: "Qt.core_private" }
    Depends { name: "Qt.global" }
    Depends { name: "Qt.widgets" }

    files: [
        "tst_qpropertyanimation.cpp",
    ]
}
