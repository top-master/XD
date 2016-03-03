Project {
    condition: !qbs.targetOS.contains("android") && !qbs.targetOS.contains("ios")
        && !project.disabledFeatures.contains("systemsemaphore") && project.private_tests
    references: ["sharedmemoryhelper", "test"]
}
