import qbs

Project {
    condition: qbs.targetOS.contains("android")
    references: [
        "jar/jar.qbs",
    ]
}
