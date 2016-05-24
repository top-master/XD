import qbs

Project {
    references: [
        "bearer/bearer.qbs",
        "generic/generic.qbs",
        "imageformats/imageformats.qbs",
        "platforminputcontexts/platforminputcontexts.qbs",
        "platforms/platforms.qbs",
        "platformthemes/platformthemes.qbs",
        "printsupport/printsupport.qbs",
        "sqldrivers/sqldrivers.qbs",
    ]
}
