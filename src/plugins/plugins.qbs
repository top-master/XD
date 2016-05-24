import qbs

Project {
    references: [
        "bearer/bearer.qbs",
        "generic/generic.qbs",
        "imageformats/imageformats.qbs",
        "platforminputcontexts/platforminputcontexts.qbs",
        "platforms/platforms.qbs",
        "platformthemes/platformthemes.qbs",
        "sqldrivers/sqldrivers.qbs",
    ]
}
