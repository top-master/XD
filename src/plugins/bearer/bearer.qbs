import qbs

Project {
    condition: project.network && !project.disabledFeatures.contains("bearermanagement")
    references: [
        "android",
        "connman",
        "corewlan",
        "generic",
        "nativewifi",
        "networkmanager",
    ]
}
