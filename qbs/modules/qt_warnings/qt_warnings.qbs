import qbs
import qbs.Utilities
import QtGlobalConfig
import QtGlobalPrivateConfig

Module {
    Depends { name: "cpp" }

    property bool warningClean: true

    // Extra warnings for Qt non-example code, to ensure cleanliness of the sources.
    property var allFlags: {
        var flags = { cxxFlags: [], objcxxFlags: [] };

        // Skip for host tools/libraries, as the host compiler's version and capabilities
        // are not checked.
        if ((product.hostBuild && QtGlobalConfig.cross_compile) || cpp.warningLevel !== "all")
            return flags;

        var isClang = qbs.toolchain.contains("clang");
        var isIcc = qbs.toolchain.contains("icc");
        var isRealGcc = qbs.toolchain.contains("gcc") && !isClang && !isIcc;
        var isDarwin = qbs.targetOS.contains("darwin");
        var compilerVersion = cpp.compilerVersionMajor + "." + cpp.compilerVersionMinor

        var versionCompare = function(version, appleClangVersion) {
            var versionToCheck = appleClangVersion && isDarwin ? appleClangVersion : version;
            return Utilities.versionCompare(compilerVersion, versionToCheck);
        };
        var versionIsAtLeast = function(version, appleClangVersion) {
            return versionCompare(version, appleClangVersion) >= 0;
        };
        var versionIsAtMost = function(version, appleClangVersion) {
            return versionCompare(version, appleClangVersion) <= 0;
        };

        // -Wdate-time: warn if we use __DATE__ or __TIME__ (we want to be able to reproduce
        // the exact same binary)
        if ((isClang && versionIsAtLeast("3.5")) || (isRealGcc && versionIsAtLeast("5")))
            flags.cxxFlags.push("-Wdate-time");

        if (isClang) {
            if (versionIsAtLeast("3.6", "6.3"))
                flags.cxxFlags.push("-Winconsistent-missing-override");

            // Clang/LLVM 5.0 and Xcode 9.0 introduced unguarded availability warnings.
            // The same construct has been a hard error in Swift from the very beginning.
            if (isDarwin && versionIsAtLeast("9")) {
                flags.cxxFlags.push("-Werror=unguarded-availability",
                                    "-Werror=unguarded-availability-new",
                                    "-Werror=unsupported-availability-guard");
            }

            flags.objcxxFlags.push("-Wobjc-interface-ivars", "-Wobjc-method-access",
                                   "-Wobjc-multiple-method-names");
        } else if (isRealGcc) {
            flags.cxxFlags.push("-Wvla"); // use of variable-length arrays (an extension to C++)
            if (versionIsAtLeast("6"))
                flags.cxxFlags.push("-Wshift-overflow=2", "-Wduplicated-cond");
            if (versionIsAtLeast("7")) {
                // GCC 7 has a lot of false positives relating to this, so disable completely
                flags.cxxFlags.push("-Wno-stringop-overflow");
            }

            // GCC 5 fixed -Wmissing-field-initializers for when there are no initializers
            if (Utilities.versionCompare(cpp.compilerVersion, "5") < 0)
                flags.cxxFlags.push("-Wno-missing-field-initializers");
        }

        if (!warningClean || !QtGlobalPrivateConfig.warnings_are_errors)
            return flags;

        // If the module declares that it has does its clean-up of warnings, enable -Werror.
        // This setting is compiler-dependent anyway because it depends on the version of the
        // compiler.
        if (isClang) {
            // Apple clang 4.0-4.2,5.0-5.1,6.0-6.4,7.0-7.3,8.0-8.3,9.0-9.2
            // Regular clang 3.x-6.0
            if (versionIsAtLeast("3", "4") && versionIsAtMost("6", "9.2")) {
                flags.cxxFlags.push("-Werror", "-Wno-error=#warnings",
                                    "-Wno-error=deprecated-declarations")
            }
        } else if (isIcc && qbs.targetOS.contains("linux")) {
            if (versionIsAtLeast("13") && versionIsAtMost("17")) {
                // 177: function "entity" was declared but never referenced
                //      (too aggressive; ICC reports even for functions created due to
                //       template instantiation)
                // 1224: #warning directive
                // 1478: function "entity" (declared at line N) was declared deprecated
                // 1881: argument must be a constant null pointer value
                //       (NULL in C++ is usually a literal 0)
                flags.cxxFlags.push("-Werror","-ww177,1224,1478,1881")
            }
        } else if (isRealGcc && !qbs.toolchain.contains("qcc")) {
            if (versionIsAtLeast("4.6")) {
                flags.cxxFlags.push("-Werror", "-Wno-error=cpp",
                                    "-Wno-error=deprecated-declarations");

                // GCC prints this bogus warning, after it has inlined a lot of code
                // error: assuming signed overflow does not occur when assuming that (X + c) < X
                // is always false
                flags.cxxFlags.push("-Wno-error=strict-overflow");

                // GCC 7 includes -Wimplicit-fallthrough in -Wextra, but Qt is not yet free
                // of implicit fallthroughs.
                if (versionIsAtLeast("7"))
                    flags.cxxFlags.push("-Wno-error=implicit-fallthrough");

                // Work-around for bug https://code.google.com/p/android/issues/detail?id=58135
                if (qbs.toolchain.contains("android"))
                    flags.cxxFlags.push("-Wno-error=literal-suffix");
            }
        } else if (qbs.toolchain.contains("msvc") && !isIcc) {
            // MSVC < 2017
            if (Utilities.versionCompare(cpp.compilerVersion, "19.10") < 0)
                flags.cxxFlags.push("/WX")

        }

        return flags;
    }
    cpp.cxxFlags: allFlags.cxxFlags
    cpp.objcxxFlags: allFlags.cxxFlags.concat(allFlags.objcxxFlags)
}
