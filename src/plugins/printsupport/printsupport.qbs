import qbs

Project {
    condition: !qbs.targetOS.contains("winrt") && project.widgets
               && !project.disabledFeatures.contains("printer")
    references: [
        "cocoa",
        "cups",
        "windows",
    ]
}
