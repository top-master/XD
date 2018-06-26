import qbs

QtAutotest {
    name: "tst_qmetatype"
    supportsUiKit: true

    cpp.includePaths: base.concat(path + "/../../../other/qvariant_common")
    cpp.defines: {
        var flags = base.concat("QT_DISABLE_DEPRECATED_BEFORE=0");
        if (qbs.toolchain.contains("clang") && qbs.architecture.contains("arm")) {
            // Building for ARM (eg iOS) with clang is affected so much that we disable
            // the template part of the test
            flags.push("TST_QMETATYPE_BROKEN_COMPILER");
        }
        return flags;
    }

    Properties {
        condition: qbs.toolchain.contains("msvc") || targetsUWP
        cpp.cxxFlags: base.concat("/bigobj")
        cpp.optimization: "none" // Reduce compile time
    }

    Properties {
        // clang has some performance problems with the test. Especially
        // with automaticTemplateRegistration which creates few thousands
        // template instantiations (QTBUG-37237). Removing -O2 and -g
        // improves the situation, but it is not solving the problem.
        condition: qbs.toolchain.contains("clang")
        cpp.optimization: "none"
        cpp.debugInformation: false
    }

    files: "tst_qmetatype.cpp"

    QtTestData {
        files: "typeFlags.bin"
    }
}
