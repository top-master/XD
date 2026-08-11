
include($$PWD/helpers.pri)

# cocoa.mm uses QtWidgets headers; gui doesn't link widgets
mac {
    add_include(QtWidgets)
}

win32 {
    include($$PWD/../windows/windows.pro)
} else:mac {
    include($$PWD/../cocoa/cocoa.pro)
} else {
    # Linux and the other Unixes: always bake in the headless "offscreen" platform
    # -- it has no external windowing-system dependencies, so a static build always
    # links and a console/QCoreApplication app (which never loads a platform) is
    # happy. In addition, bake in every native windowing plugin this configuration
    # actually enables, so a static GUI app on Ubuntu/Debian/Fedora also gets its
    # real platform (xcb/X11, the desktop default) rather than only the headless
    # fallback. Each is guarded on QT_CONFIG, so a build that did not configure a
    # given plugin (e.g. a headless box with no X11) simply keeps offscreen.
    include($$PWD/../offscreen/offscreen.pro)
    contains(QT_CONFIG, xcb) {
        include($$PWD/../xcb/xcb-plugin.pro)
        add_static_lib(QtXcbQpa)
    }
    contains(QT_CONFIG, eglfs):   include($$PWD/../eglfs/eglfs.pro)
    contains(QT_CONFIG, linuxfb): include($$PWD/../linuxfb/linuxfb.pro)
}

HEADERS += $$PWD/static-platform.h

SOURCES += $$PWD/static-platform.cpp

TARGET = QtGui
QT = core-private

# MARK: Links against dependencies that recurse back to QtGui module.
#
# We can't simply do:
# ```
# QT += platformsupport-private
# mac {
#     QT += printsupport-private
# }
# ```
# since the .prl files of above dependencies may not exist yet,
# and hence we may sometimes not link against the dependencies we need.
#
# We can't add to `QT_FOR_PRIVATE`, since that becomes `gui_static.run_depends`
# and the `.run_depends` is just a list of what the runtime may load,
# and hence we would never link against the dependencies we need,
#
# The solution is to use `add_static_lib`.
#
# Note: the `QT` value becomes the `gui_static.depends` value
# in this module's .prl file, and we added `onQtModuleRecursion`
# to allow few recurrsion in `qt_static` builds, since
# without that, any later `QT += ...` resolver would walk `.depends`
# and cause `gui -> printsupport -> widgets -> gui` cycle error,
# however, we ended up never using `QT` variable.
#
add_static_lib(QtPlatformSupport)
mac {
    add_static_lib(QtPrintSupport)
}
