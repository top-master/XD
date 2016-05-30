import qbs
import qbs.TextFile

QtAutotest {
    name: "tst_qprocess"
    condition: Qt.network.present
    destinationDirectory: project.buildDirectory + "/qprocess_test"
    Depends { name: "Qt.core-private" }
    Depends { name: "Qt.network"; required: false }

    Depends { name: "fileWriterProcess" }
    Depends { name: "testDetached" }
    Depends { name: "testExitCodes" }
    Depends { name: "testForwarding" }
    Depends { name: "testGuiProcess"; condition: project.widgets }
    Depends { name: "testProcessCrash" }
    Depends { name: "testProcessDeadWhileReading" }
    Depends { name: "testProcessEOF" }
    Depends { name: "testProcessEcho" }
    Depends { name: "testProcessEcho2" }
    Depends { name: "testProcessEcho3" }
    Depends { name: "testProcessEchoGui"; condition: qbs.targetOS.contains("windows") }
    Depends { name: "testProcessEnvironment" }
    Depends { name: "testProcessHang" }
    Depends { name: "testProcessNormal" }
    Depends { name: "testProcessOutput" }
    Depends { name: "nospace" }
    Depends { name: "one space" }
    Depends { name: "two space s" }
    Depends { name: "testSetNamedPipeHandleState"; condition: qbs.targetOS.contains("windows") }
    Depends { name: "testSetWorkingDirectory" }
    Depends { name: "testSoftExit" }
    Depends { name: "testSpaceInName" }

    Transformer {
        // The test code expects a directory "test" to exist next to the executable
        Artifact {
            filePath: project.buildDirectory + "/qprocess_test/test/dummy.txt"
            fileTags: []
        }
        prepare: {
            var cmd = new JavaScriptCommand();
            cmd.silent = true;
            cmd.sourceCode = function() {
                var f = new TextFile(output.filePath, TextFile.WriteOnly);
                f.close();
            };
            return [cmd];
        }
    }

    files: [
        "../tst_qprocess.cpp",
    ]
}

// win32:TESTDATA += ../testBatFiles/*
// mac:CONFIG += insignificant_test # QTBUG-25895 - sometimes hangs
