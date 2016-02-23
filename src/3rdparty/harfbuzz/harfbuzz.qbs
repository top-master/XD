import qbs

StaticLibrary {
    destinationDirectory: project.libDirectory
    profiles: project.targetProfiles
    builtByDefault: false

    Depends { name: "Android.ndk"; condition: qbs.targetOS.contains("android") }
    Depends { name: "cpp" }

    Group {
        name: "sources"
        prefix: "src/"
        files: [
            "harfbuzz-buffer.c",
            "harfbuzz-gdef.c",
            "harfbuzz-gpos.c",
            "harfbuzz-gsub.c",
            "harfbuzz-impl.c",
            "harfbuzz-open.c",
            "harfbuzz-shaper-all.cpp",
            "harfbuzz-stream.c",
        ]
    }

    Export {
        Depends { name: "cpp" }
        cpp.includePaths: [
            project.qtbasePrefix + "src/3rdparty/harfbuzz/src",
        ]
    }
}
