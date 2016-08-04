import qbs

PkgConfigDependency {
    name: "zlib"
    type: project.zlib ? ["staticlibrary"] : []
    condition: true
    profiles: project.targetProfiles.concat(project.crossBuild ? [project.hostProfile] : [])
    runProbe: project.pkg_config && project.system_zlib

    Export {
        Depends { name: "cpp" }
        cpp.includePaths: project.system_zlib ? product.includePaths : [
            project.qtbasePrefix + "src/3rdparty/zlib",
            project.configPath + "src/corelib/global", // for qconfig.h
            project.buildDirectory + "/include",
            project.buildDirectory + "/include/QtCore",
        ]
        Properties {
            condition: project.system_zlib && !product.found // e.g. Android
            cpp.dynamicLibraries: ["z"]
        }
    }

    Depends { name: "Android.ndk"; condition: qbs.targetOS.contains("android") }
    Depends { name: "cpp" }
    Depends {
        name: "QtCoreHeaders"
        profiles: project.targetProfiles
        condition: !project.system_zlib
    }

    Properties {
        condition: !project.system_zlib
        cpp.includePaths: [
            project.configPath + "/src/corelib/global", // for qconfig.h
            project.buildDirectory + "/include",
            project.buildDirectory + "/include/QtCore",
        ].concat(base)
    }

    Group {
        name: "headers"
        files: [
            "zutil.h",
            "crc32.h",
            "deflate.h",
            "gzguts.h",
            "inffast.h",
            "inffixed.h",
            "inflate.h",
            "inftrees.h",
            "trees.h",
            "zconf.h",
            "zlib.h",
        ]
    }

    Group {
        name: "sources"
        files: [
            "zutil.c",
            "adler32.c",
            "compress.c",
            "crc32.c",
            "deflate.c",
            "gzclose.c",
            "gzlib.c",
            "gzread.c",
            "gzwrite.c",
            "infback.c",
            "inffast.c",
            "inflate.c",
            "inftrees.c",
            "minigzip.c",
            "trees.c",
            "uncompr.c",
        ]
    }
}
