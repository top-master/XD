Project {
    condition: !qbs.targetOS.contains("android") && !qbs.targetOS.contains("ios")
        && !project.disabledFeatures.contains("systemsemaphore")
    references: ["systemsemaphorehelper", "test"]
}
