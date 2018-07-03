import qbs

Project {
    references: [
         "sqldrivers",
        // bearer
        "generic",
        "imageformats",
        "platforminputcontexts",
        "platforms",
        "platformthemes",
        "printsupport",
    ]
}

/*
TEMPLATE = subdirs
QT_FOR_CONFIG += network

qtHaveModule(network):qtConfig(bearermanagement): SUBDIRS += bearer
qtHaveModule(gui) {
    SUBDIRS *= platforms platforminputcontexts platformthemes
    qtConfig(imageformatplugin): SUBDIRS *= imageformats
}
*/
