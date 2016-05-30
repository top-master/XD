import qbs

Project {
    condition: !qbs.targetOS.contains("winrt")
    qbsSearchPaths: ["."]
    references: [
        "fileWriterProcess",
        "test",
        "testDetached",
        "testExitCodes",
        "testForwarding",
        "testGuiProcess",
        "testProcessCrash",
        "testProcessDeadWhileReading",
        "testProcessEOF",
        "testProcessEcho",
        "testProcessEcho2",
        "testProcessEcho3",
        "testProcessEchoGui",
        "testProcessEnvironment",
        "testProcessHang",
        "testProcessNormal",
        "testProcessOutput",
        "testProcessSpacesArgs/nospace.qbs",
        "testProcessSpacesArgs/onespace.qbs",
        "testProcessSpacesArgs/twospaces.qbs",
        "testSetNamedPipeHandleState",
        "testSetWorkingDirectory",
        "testSoftExit",
        "testSpaceInName",
    ]
}
