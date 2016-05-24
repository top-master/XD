import qbs
import qbs.TextFile
import "QtUtils.js" as QtUtils

Project {
    qbsSearchPaths: ["qbs"]
    minimumQbsVersion: "1.5.0"

    readonly property string version: {
        var qmakeConfFile = new TextFile(qtbasePrefix + ".qmake.conf");
        var qmakeConf = qmakeConfFile.readAll();
        qmakeConfFile.close();
        return qmakeConf.match(/^MODULE_VERSION = (\d\d?\.\d\d?\.\d\d?)$/m)[1];
    }
    readonly property var versionParts: version.split('.').map(function(part) { return parseInt(part); })
    readonly property string hostProfile: {
        if (hostMkspec !== targetMkspec || !qbs.architecture || !qbs.architecture.startsWith("x86"))
            print("You appear to be cross-compiling. Consider setting project.hostProfile to an appropriate profile for the host OS.");
        return profile;
    }
    readonly property stringList targetProfiles: [profile]
    readonly property bool crossBuild: !(targetProfiles.length === 1
                                         && hostProfile === targetProfiles[0])
    readonly property string hostMkspec: QtUtils.detectHostMkspec(qbs.hostOS, qbs.toolchain)
    readonly property string targetMkspec: QtUtils.detectTargetMkspec(qbs.targetOS, qbs.toolchain, qbs.architecture)
    property string configPath: buildDirectory + "/.." // ### for configure builds only...
    property string sourcePrefix: sourceDirectory + '/'
    property string qtbasePrefix: sourcePrefix + "qtbase/"
    property string examplesInstallDir: "/examples"

    // This property restricts certain build rules to one variant when we are building for multiple variants
    readonly property bool buildForVariant: debug_and_release ? qbs.buildVariant === "release" : true

    readonly property path binDirectory: buildDirectory + "/bin"
    readonly property path libDirectory: buildDirectory + "/lib"
    readonly property path jarDirectory: buildDirectory + "/jar"

    readonly property var config: []
    readonly property var disabledFeatures: []

    readonly property bool pkg_config: !config.contains("no-pkg-config")

    readonly property bool debug_and_release: config.contains("debug_and_release")
    readonly property bool staticBuild: config.contains("staticBuild")
    readonly property bool precompiledHeaders: config.contains("precompile_header")
    readonly property string corelibPrecompiledHeader: qtbasePrefix + "src/corelib/global/qt_pch.h"

    // Modules
    readonly property bool concurrent: !disabledFeatures.contains("concurrent")
    readonly property bool core: !disabledFeatures.contains("core")
    readonly property bool dbus: !disabledFeatures.contains("dbus")
    readonly property bool gui: !disabledFeatures.contains("gui")
    readonly property bool network: !disabledFeatures.contains("network")
    readonly property bool printsupport: !disabledFeatures.contains("printsupport") && widgets
    readonly property bool sql: !disabledFeatures.contains("sql")
    readonly property bool testlib: !disabledFeatures.contains("testlib")
    readonly property bool widgets: !disabledFeatures.contains("widgets")

    readonly property bool doubleConversion: config.contains("doubleconversion")
    readonly property bool system_doubleConversion: config.contains("system-doubleconversion")

    // SQL
    readonly property stringList sqlPlugins: []
    readonly property stringList cFlagsMysql: []
    readonly property stringList lFlagsMysql: []
    readonly property stringList lFlagsOdbc: []
    readonly property stringList cFlagsPsql: []
    readonly property stringList lFlagsPsql: []
    readonly property stringList cFlagsSqlite: []
    readonly property stringList lFlagsSqlite: []
    readonly property stringList cFlagsTds: []
    readonly property stringList lFlagsTds: []
    readonly property bool system_sqlite: config.contains("system-sqlite")

    // SIMD
    readonly property bool sse2: config.contains("sse2")
    readonly property stringList sse2Flags: []

    readonly property bool sse3: config.contains("sse3")
    readonly property stringList sse3Flags: []

    readonly property bool ssse3: config.contains("ssse3")
    readonly property stringList ssse3Flags: []

    readonly property bool sse4_1: config.contains("sse4_1")
    readonly property stringList sse4_1Flags: []

    readonly property bool sse4_2: config.contains("sse4_2")
    readonly property stringList sse4_2Flags: []

    readonly property bool avx: config.contains("avx")
    readonly property stringList avxFlags: []

    readonly property bool avx2: config.contains("avx2")
    readonly property stringList avx2Flags: []

    readonly property bool neon: config.contains("neon")
    readonly property stringList neonFlags: []

    // Symbol visibility
    readonly property bool private_tests: config.contains("private_tests")
    readonly property bool rpath: config.contains("rpath")
    readonly property bool reduce_exports: config.contains("reduce_exports")
    readonly property bool reduce_relocations: config.contains("reduce_relocations")

    // QtCore
    readonly property bool glib: config.contains("glib")
    readonly property bool iconv: config.contains("iconv")
    readonly property bool icu: config.contains("icu")
    readonly property bool harfbuzz: config.contains("harfbuzz")
    readonly property bool inotify: config.contains("inotify")
    readonly property bool kqueue: qbs.targetOS.contains("darwin") || qbs.targetOS.contains("freebsd") || qbs.targetOS.contains("netbsd") || qbs.targetOS.contains("openbsd")
    readonly property bool pcre: config.contains("pcre")
    readonly property bool system_zlib: config.contains("system-zlib")
    readonly property bool zlib: config.contains("zlib")

    // QtNetwork
    readonly property bool getaddrinfo: config.contains("getaddrinfo")
    readonly property bool getifaddrs: config.contains("getifaddrs")
    readonly property bool ipv6ifname: config.contains("ipv6ifname")
    readonly property bool libproxy: config.contains("libproxy")
    readonly property bool openssl: config.contains("openssl")
    readonly property bool ssl: openssl || qbs.targetOS.contains("winrt") || qbs.targetOS.contains("darwin")

    // QtGui
    readonly property bool accessibility: config.contains("accessibility")
    readonly property bool alsa: config.contains("alsa")
    readonly property bool angle: qbs.targetOS.contains("windows") && opengles2
    readonly property bool atspi_bridge: config.contains("atspi_bridge")
    readonly property bool egl: config.contains("egl")
    readonly property bool evdev: config.contains("evdev")
    readonly property bool eglfs_viv: config.contains("eglfs_viv")
    readonly property bool eglfs_brcm: config.contains("eglfs_brcm")
    readonly property bool eglfs_mali: config.contains("eglfs_mali")
    readonly property bool eglfs_x11: config.contains("egl_x11")
    readonly property bool jpeg: config.contains("jpeg")
    readonly property bool kms: config.contains("kms")
    readonly property bool libinput: config.contains("libinput")
    readonly property bool libudev: config.contains("libudev")
    readonly property bool linuxfb: config.contains("linuxfb")
    readonly property bool mtdev: config.contains("mtdev")
    readonly property bool opengl: config.contains("opengl")
    readonly property bool opengles2: config.contains("opengles2")
    readonly property bool png: config.contains("png")
    readonly property bool system_freetype: config.contains("system-freetype")
    readonly property bool system_jpeg: config.contains("system-jpeg")
    readonly property bool system_png: config.contains("system-png")
    readonly property bool tslib: config.contains("tslib")
    readonly property bool xcb: config.contains("xcb")
    readonly property bool xcb_render: config.contains("xcb-render")
    readonly property bool xcb_glx: config.contains("xcb-glx")
    readonly property bool xcb_xlib: config.contains("xcb-xlib")
    readonly property bool xcb_sm: config.contains("xcb-sm")
    readonly property bool xkbcommon_evdev: config.contains("xkbcommon-evdev")
    readonly property bool xkbcommon_x11: config.contains("xkbcommon-x11")

    // QtPrintSupport
    readonly property bool cups: config.contains("cups")

    // QtWidgets
    readonly property bool androidstyle: config.contains("androidstyle")
    readonly property bool gtkstyle: config.contains("gtkstyle")
    readonly property bool windowscestyle: config.contains("windowscestyle")
    readonly property bool windowsmobilestyle: config.contains("windowsmobilestyle")
    readonly property bool windowsvistastyle: config.contains("windowsvistastyle")
    readonly property bool windowsxpstyle: config.contains("windowsxpstyle")

    // QtMultimedia
    readonly property bool gstreamer_0_10: config.contains("gstreamer_0_10")
    readonly property bool pulseaudio: config.contains("pulseaudio")

    // QtDBus
    readonly property stringList cFlagsDbusHost: []

    // QtOpengl
    readonly property stringList openglIncludeDirs: []
    readonly property stringList opengles2IncludeDirs: []
    readonly property stringList openglLibDirs: []
    readonly property stringList opengles2LibDirs: []
    readonly property stringList openglLibs: []
    readonly property stringList opengles2Libs: []
    readonly property stringList openglFrameworks: []
    readonly property stringList opengles2Frameworks: []

    // Features system
    readonly property bool cursor: !disabledFeatures.contains("cursor")
}
