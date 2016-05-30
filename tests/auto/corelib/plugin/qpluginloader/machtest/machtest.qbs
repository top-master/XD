import qbs

Project {
    qbsSearchPaths: ["."]

    QtProduct {
        Depends { name: "osversions" }
        name: "tst_qpluginloadermachtest.i386"
        targetName: "good.i386"
        type: ["dynamiclibrary"]
        destinationDirectory: project.buildDirectory + "/qpluginloader_test/lib"
        files: ["../fakeplugin.cpp"]
        qbs.architecture: "x86"
    }

    QtProduct {
        Depends { name: "osversions" }
        name: "tst_qpluginloadermachtest." + qbs.architecture
        targetName: "good." + qbs.architecture
        type: ["dynamiclibrary"]
        destinationDirectory: project.buildDirectory + "/qpluginloader_test/lib"
        files: ["../fakeplugin.cpp"]
        qbs.architecture: "x86_64"
    }

    QtProduct {
        Depends { name: "osversions" }
        name: "tst_qpluginloadermachtest." + qbs.architecture
        targetName: "good." + qbs.architecture
        type: ["dynamiclibrary"]
        qbs.architecture: "ppc64"

        Rule {
            inputsFromDependencies: ["dynamiclibrary"]

            Artifact {
                filePath: "good.ppc64"
                fileTags: ["dynamiclibrary"]
            }

            prepare: {
                var cmd = new Command("ppcconverter.pl", [input.filePath, output.filePath]);
                cmd.workingDirectory = product.sourceDirectory;
                return cmd;
            }
        }
    }

    QtProduct {
        Depends { name: "tst_qpluginloadermachtest.i386" }
        Depends { name: "tst_qpluginloadermachtest.x86_64" }
        Depends { name: "tst_qpluginloadermachtest.ppc64" }
        Depends { name: "machfat" }

        name: "tst_qpluginloadermachtest_fat.all"
        targetName: "good.fat.all"
        type: ["dynamiclibrary_fat"]
        destinationDirectory: project.buildDirectory + "/qpluginloader_test/lib"
    }

    QtProduct {
        Depends { name: "tst_qpluginloadermachtest.x86_64" }
        Depends { name: "tst_qpluginloadermachtest.ppc64" }
        Depends { name: "machfat" }

        name: "tst_qpluginloadermachtest_fat.no-i386"
        targetName: "good.fat.no-i386"
        type: ["dynamiclibrary_fat"]
        destinationDirectory: project.buildDirectory + "/qpluginloader_test/lib"
    }

    QtProduct {
        Depends { name: "tst_qpluginloadermachtest.i386" }
        Depends { name: "tst_qpluginloadermachtest.ppc64" }
        Depends { name: "machfat" }

        name: "tst_qpluginloadermachtest_fat.no-x86_64"
        targetName: "good.fat.no-x86_64"
        type: ["dynamiclibrary_fat"]
        destinationDirectory: project.buildDirectory + "/qpluginloader_test/lib"
    }

    QtProduct {
        Depends { name: "tst_qpluginloadermachtest.ppc64" }
        Depends { name: "machfat" }

        name: "tst_qpluginloadermachtest_fat.stub-i386"
        targetName: "good.fat.no-i386"
        type: ["dynamiclibrary_fat"]
        destinationDirectory: project.buildDirectory + "/qpluginloader_test/lib"

        machfat.additionalLipoArgs: ["-arch_blank", "i386"]
    }

    QtProduct {
        Depends { name: "tst_qpluginloadermachtest.ppc64" }
        Depends { name: "machfat" }

        name: "tst_qpluginloadermachtest_fat.stub-x86_64"
        targetName: "good.fat.no-x86_64"
        type: ["dynamiclibrary_fat"]
        destinationDirectory: project.buildDirectory + "/qpluginloader_test/lib"

        machfat.additionalLipoArgs: ["-arch_blank", "x86_64"]
    }

    QtProduct {
        type: "dummy"

        Rule {
            multiplex: true

            Artifact {
                filePath: ".dummy"
                fileTags: ["dummy"]
            }

            prepare: {
                var cmd = new Command(product.sourceDirectory + "/generate-bad.pl");
                cmd.workingDirectory = product.buildDirectory;
                return cmd;
            }
        }
    }
}
