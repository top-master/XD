function detectHostMkspec(hostOS, toolchain)
{
    if (hostOS.contains("linux")) {
        if (toolchain.contains("icc"))
            return "linux-icc";
        if (toolchain.contains("clang"))
            return "linux-clang";
        if (toolchain.contains("gcc"))
            return "linux-g++";
    } else if (hostOS.contains("osx")) {
        if (toolchain.contains("icc"))
            return "macx-icc";
        if (toolchain.contains("clang"))
            return "macx-clang";
        if (toolchain.contains("gcc"))
            return "macx-g++";
    } else if (hostOS.contains("windows")) {
        if (toolchain.contains("msvc"))
            return "win32-msvc2005"; // All msvc mkspecs point to the same platformdefs
        if (toolchain.contains("mingw"))
            return "win32-g++";
    }
    print("Unable to determine host mkspec.");
    return "unknown";
}

function detectTargetMkspec(targetOS, toolchain, architecture)
{
    if (targetOS.contains("android")) {
        return "android-g++";
    } else if (targetOS.contains("ios")) {
        if (toolchain.contains("clang"))
            return "macx-ios-clang";
    } else if (targetOS.contains("linux")) {
        if (toolchain.contains("clang"))
            return "linux-clang";
        else if (toolchain.contains("gcc"))
            return "linux-g++";
    } else if (hostOS.contains("osx")) {
        var architectureSuffix = architecture === "x86" ? "-32" : "";
        if (toolchain.contains("icc"))
            return "macx-icc";
        if (toolchain.contains("clang"))
            return "macx-clang" + architectureSuffix;
        if (toolchain.contains("gcc"))
            return "macx-g++" + architectureSuffix;
    } else if (targetOS.contains("winphone")) {
        switch (architecture) {
        case "x86":
            return "winphone-x86-msvc2013";
        case "x86_64":
            return "winphone-x64-msvc2013";
        case "armv7":
            return "winphone-arm-msvc2013";
        }
    } else if (targetOS.contains("winrt")) {
        switch (architecture) {
        case "x86":
            return "winrt-x86-msvc2013";
        case "x86_64":
            return "winrt-x64-msvc2013";
        case "armv7":
            return "winrt-arm-msvc2013";
        }
    } else if (targetOS.contains("windows")) {
        if (toolchain.contains("mingw"))
            return "win32-g++";
        else if (toolchain.contains("msvc"))
            return "win32-msvc2005"; // All msvc mkspecs point to the same platformdefs
    }
    return "";
}

function includesForModule(module, base, qtVersion) {
    var includes = [base];
    if (module.endsWith("-private")) {
        module = module.slice(0, -8);
        includes.push(base + "/" + module + "/" + qtVersion);
        includes.push(base + "/" + module + "/" + qtVersion + "/" + module);
        includes.push(base + "/" + module + "/" + qtVersion + "/" + module + "/private");
        if (module === "QtGui")
            includes.push(base + "/" + module + "/" + qtVersion + "/" + module + "/qpa");
    }
    includes.push(base + '/' + module);
    return includes;
}

function includePaths(cflags) {
    var includePaths = [];
    for (var i in cflags) {
        if (cflags[i].startsWith("-I"))
            includePaths.push(cflags[i].slice(2));
        else
            includePaths.push(cflags[i]);
    }
    return includePaths;
}

function libraryPaths(libs) {
    var libraryPaths = [];
    for (var i in libs) {
        if (libs[i].startsWith("-L"))
            libraryPaths.push(libs[i].slice(2));
    }
    return libraryPaths;
}

function dynamicLibraries(libs) {
    var dynamicLibraries = [];
    for (var i in libs) {
        if (libs[i].startsWith("-l"))
            dynamicLibraries.push(libs[i].slice(2));
    }
    return dynamicLibraries;
}

function frameworks(flags) {
    var frameworks = [];
    for (var i = 0; i < flags.length; ++i) {
        if (flags[i] === "-framework" && i < flags.length - 1)
            frameworks.push(flags[++i]);
    }
    return frameworks;
}
