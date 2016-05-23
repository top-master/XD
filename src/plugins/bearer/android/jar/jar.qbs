import qbs

JavaJarFile {
    name: "QtAndroidBearer-bundled"
    packageName: name // dummy for Android.sdk
    condition: qbs.targetOS.contains("android")
    Depends { name: "Android.sdk" }
    java.additionalClassPaths: [
        Android.sdk.androidJarFilePath,
        "src",
    ]
    files: [
        "src/org/qtproject/qt5/android/bearer/QtNetworkReceiver.java",
    ]

    Group {
        fileTagsFilter: product.type
        qbs.install: true
        qbs.installDir: "/jar"
    }
}
