TEMPLATE = subdirs
SUBDIRS = \
        corelib \
        # Termination comment.

contains(QT_CONFIG, sql): SUBDIRS += sql

# removed-by-refactor qtHaveModule(opengl): SUBDIRS += opengl
qtHaveModule(dbus): SUBDIRS += dbus
qtHaveModule(network): SUBDIRS += network
qtHaveModule(gui): SUBDIRS += gui

check-trusted.CONFIG += recursive
QMAKE_EXTRA_TARGETS += check-trusted
