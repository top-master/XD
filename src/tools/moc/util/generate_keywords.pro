TEMPLATE = app
CONFIG -= moc
mac:CONFIG -= app_bundle
INCLUDEPATH += .

SOURCES += generate_keywords.cpp

# Bootstrap as console-tool.
option(host_build)
CONFIG += force_bootstrap
load(qt_tool)

# But don't move to Qt's bin folder (since is a one-time-use tool).
CONFIG(debug, release|debug) {
    DESTDIR = debug
} else {
    DESTDIR = release
}

DESTDIR = $$shadowed($$DESTDIR)
copy_file($$PWD/generate_files.bat, $$DESTDIR/)
copy_file($$PWD/licenseheader.txt, $$DESTDIR/)
