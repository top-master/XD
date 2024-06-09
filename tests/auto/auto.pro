TEMPLATE = subdirs

SUBDIRS += \
    corelib \
    dbus \
    gui \
    network \
    opengl

contains(QT_CONFIG, sql): SUBDIRS += sql

SUBDIRS += \
    testlib \
    tools \
    xml \
    concurrent \
    other \
    widgets \
    printsupport \
    cmake \
    installed_cmake

installed_cmake.depends = cmake

ios: SUBDIRS  = corelib gui

wince:                                      SUBDIRS -= printsupport
cross_compile:                              SUBDIRS -= tools
!qtHaveModule(opengl):                      SUBDIRS -= opengl
!qtHaveModule(gui):                         SUBDIRS -= gui
!qtHaveModule(widgets):                     SUBDIRS -= widgets
!qtHaveModule(printsupport):                SUBDIRS -= printsupport
!qtHaveModule(concurrent):                  SUBDIRS -= concurrent
!qtHaveModule(network):                     SUBDIRS -= network

# Disable the QtDBus tests if we can't connect to the session bus
qtHaveModule(dbus) {
    !system("dbus-send --session --type=signal / local.AutotestCheck.Hello >/dev/null 2>&1") {
        !build_pass {
            contains(QT_CONFIG, dbus-linked): \
                error("QtDBus is enabled but session bus is not available. Please check the installation.")
            else: \
                warning("QtDBus is enabled with runtime support, but session bus is not available. Skipping QtDBus tests.")
        }
        SUBDIRS -= dbus
    }
} else {
    SUBDIRS -= dbus
}

isEmpty(QT_SOURCE_TREE): \
    error("The root .qmake.conf file needs to be loaded for Test-cases that are in Qt-source-tree.")
