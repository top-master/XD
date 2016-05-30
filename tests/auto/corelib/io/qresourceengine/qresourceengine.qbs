import qbs

Project {
    Product {
        name: "runtime_resource"
        type: ["runtime_resource"]
        profiles: project.targetProfiles
        destinationDirectory: project.buildDirectory + "/qresourceengine_test"
        Group {
            files: ["testqrc/test.qrc"]
            fileTags: ["custom_qrc"]
        }
        Rule {
            inputs: ["custom_qrc"]
            Artifact {
                filePath: product.destinationDirectory + "/runtime_resource.rcc"
                fileTags: product.type
            }
            prepare: {
                var args = ["-root", "/runtime_resource/", "-binary", input.filePath,
                            "-o", output.filePath];
                var cmd = new Command(project.binDirectory + "/rcc", args);
                cmd.description = "Creating " + output.fileName;
                return [cmd];
            }
        }
    }

    QtAutotest {
        name: "tst_qresourceengine"
        destinationDirectory: project.buildDirectory + "/qresourceengine_test"
        Depends { name: "runtime_resource" }
        files: [
            "tst_qresourceengine.cpp",
        ]
        Group {
            name: "android test data"
            condition: qbs.targetOS.contains("android")
            files: ["android_testdata.qrc"]
        }
    }
}

//TESTDATA += \
//    parentdir.txt \
//    testqrc/*
//GENERATED_TESTDATA = $${runtime_resource.target}
