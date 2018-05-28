import qbs

Project {
    references: "staticplugin"

    QtAutotest {
        name: "tst_qresourceengine"
        type: base.concat("runtime_resource")

        Depends { name: "moctestplugin"; cpp.linkWholeArchive: true } // TODO: be9a56e5e3ced5d0d668fa24e4c65ae928f2e25a

        files: "tst_qresourceengine.cpp"

        Group {
            name: "qrc test data"
            files: "testqrc/test.qrc"
            fileTags: ["custom_qrc", "qt.testdata"]
            overrideTags: false
        }

        Group {
            name: "android test data"
            condition: targetsAndroidProper
            files: "android_testdata.qrc"
        }

        QtTestData {
            name: "other test data"
            files: "parentdir.txt"
        }

        Rule {
            inputs: ["custom_qrc"]
            explicitlyDependsOnFromDependencies: ["qt.rcc-tool"]
            Artifact {
                filePath: "runtime_resource.rcc"
                fileTags: ["runtime_resource", "qt.generated_testdata"]
            }
            prepare: {
                // TODO: Generating runtime resources should be a feature of Qt.core
                var rccPath = explicitlyDependsOn["qt.rcc-tool"][0].filePath;
                var args = ["-root", "/runtime_resource/", "-binary", input.filePath,
                            "-o", output.filePath];
                var cmd = new Command(rccPath, args);
                cmd.description = "Creating " + output.fileName;
                return [cmd];
            }
        }
    }
}
