Project {
    condition: project.config.contains("testcase_targets")
        && !qbs.targetOS.contains("android") && !qbs.targetOS.contains("ios")
        && !project.disabledFeatures.contains("systemsemaphore")
    references: ["systemsemaphorehelper", "test"]
}
