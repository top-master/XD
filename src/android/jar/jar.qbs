import qbs

JavaJarFile {
    name: "QtAndroid"
    readonly property string packageName: name
    readonly property string prefix: project.qtbasePrefix + "src/android/jar/src/org/qtproject/qt5/android/"

    Depends { name: "Android.sdk" }
    Depends { name: "java" }

    java.additionalClassPaths: [
        Android.sdk.androidJarFilePath,
    ]

    Group {
        name: "sources"
        prefix: product.prefix
        files: [
            "accessibility/QtAccessibilityDelegate.java",
            "accessibility/QtNativeAccessibility.java",
            "ExtractStyle.java",
            "QtActivityDelegate.java",
            "QtEditText.java",
            "QtInputConnection.java",
            "QtLayout.java",
            "QtMessageDialogHelper.java",
            "QtNative.java",
            "QtNativeLibrariesDir.java",
            "QtSurface.java",
        ]
    }

    Group {
        fileTagsFilter: "java.jar"
        qbs.installDir: project.jarDirectory
        qbs.install: true
    }
}
