import QtGuiConfig

Group {
    condition: QtGuiConfig.accessibility
    prefix: path + "/"
    files: [
        "complexwidgets.cpp",
        "complexwidgets_p.h",
        "itemviews.cpp",
        "itemviews_p.h",
        "qaccessiblemenu.cpp",
        "qaccessiblemenu_p.h",
        "qaccessiblewidget.cpp",
        "qaccessiblewidget.h",
        "qaccessiblewidgetfactory.cpp",
        "qaccessiblewidgetfactory_p.h",
        "qaccessiblewidgets.cpp",
        "qaccessiblewidgets_p.h",
        "rangecontrols.cpp",
        "rangecontrols_p.h",
        "simplewidgets.cpp",
        "simplewidgets_p.h",
    ]
}
