TARGET = qcocoa


# In qt_static builds we are included from gui/, hence register own dir explicitly,
# otherwise qmake adds root-pro-file's PWD by default.
INCLUDEPATH += $$PWD

OBJECTIVE_SOURCES += $$PWD/main.mm \
    $$PWD/qcocoaintegration.mm \
    $$PWD/qcocoatheme.mm \
    $$PWD/qcocoabackingstore.mm \
    $$PWD/qcocoawindow.mm \
    $$PWD/qnsview.mm \
    $$PWD/qnsviewaccessibility.mm \
    $$PWD/qnswindowdelegate.mm \
    $$PWD/qcocoanativeinterface.mm \
    $$PWD/qcocoaeventdispatcher.mm \
    $$PWD/qcocoaapplicationdelegate.mm \
    $$PWD/qcocoaapplication.mm \
    $$PWD/qcocoamenu.mm \
    $$PWD/qcocoamenuitem.mm \
    $$PWD/qcocoamenubar.mm \
    $$PWD/qcocoamenuloader.mm \
    $$PWD/qcocoahelpers.mm \
    $$PWD/qmultitouch_mac.mm \
    $$PWD/qcocoaaccessibilityelement.mm \
    $$PWD/qcocoaaccessibility.mm \
    $$PWD/qcocoacolordialoghelper.mm \
    $$PWD/qcocoafiledialoghelper.mm \
    $$PWD/qcocoafontdialoghelper.mm \
    $$PWD/qcocoacursor.mm \
    $$PWD/qcocoaclipboard.mm \
    $$PWD/qcocoadrag.mm \
    $$PWD/qmacclipboard.mm \
    $$PWD/qcocoasystemsettings.mm \
    $$PWD/qcocoainputcontext.mm \
    $$PWD/qcocoaservices.mm \
    $$PWD/qcocoasystemtrayicon.mm \
    $$PWD/qcocoaintrospection.mm \
    $$PWD/qcocoakeymapper.mm \
    $$PWD/qcocoamimetypes.mm

SOURCES += $$PWD/messages.cpp

HEADERS += $$PWD/qcocoaintegration.h \
    $$PWD/qcocoatheme.h \
    $$PWD/qcocoabackingstore.h \
    $$PWD/qcocoawindow.h \
    $$PWD/qnsview.h \
    $$PWD/qnswindowdelegate.h \
    $$PWD/qcocoanativeinterface.h \
    $$PWD/qcocoaeventdispatcher.h \
    $$PWD/qcocoaapplicationdelegate.h \
    $$PWD/qcocoaapplication.h \
    $$PWD/qcocoamenu.h \
    $$PWD/qcocoamenuitem.h \
    $$PWD/qcocoamenubar.h \
    $$PWD/qcocoamenuloader.h \
    $$PWD/qcocoahelpers.h \
    $$PWD/qmultitouch_mac_p.h \
    $$PWD/qcocoaaccessibilityelement.h \
    $$PWD/qcocoaaccessibility.h \
    $$PWD/qcocoacolordialoghelper.h \
    $$PWD/qcocoafiledialoghelper.h \
    $$PWD/qcocoafontdialoghelper.h \
    $$PWD/qcocoacursor.h \
    $$PWD/qcocoaclipboard.h \
    $$PWD/qcocoadrag.h \
    $$PWD/qmacclipboard.h \
    $$PWD/qcocoasystemsettings.h \
    $$PWD/qcocoainputcontext.h \
    $$PWD/qcocoaservices.h \
    $$PWD/qcocoasystemtrayicon.h \
    $$PWD/qcocoaintrospection.h \
    $$PWD/qcocoakeymapper.h \
    $$PWD/messages.h \
    $$PWD/qcocoamimetypes.h

contains(QT_CONFIG, opengl.*) {
    OBJECTIVE_SOURCES += $$PWD/qcocoaglcontext.mm

    HEADERS += $$PWD/qcocoaglcontext.h
}

RESOURCES += $$PWD/qcocoaresources.qrc

LIBS += -framework Cocoa -framework Carbon -framework IOKit -lcups

QT += core-private gui-private platformsupport-private

# For shared builds the original `qtHaveModule(widgets)` guard works,
# and for `qt_static` builds the symbols are resolved only when the
# consumer app statically links all archives together,
# hence no need for `qtHaveModule` guard.
#
qt_static|qtHaveModule(widgets) {
    OBJECTIVE_SOURCES += \
        $$PWD/qpaintengine_mac.mm \
        $$PWD/qprintengine_mac.mm \
        $$PWD/qcocoaprintersupport.mm \
        $$PWD/qcocoaprintdevice.mm \

    HEADERS += \
        $$PWD/qpaintengine_mac_p.h \
        $$PWD/qprintengine_mac_p.h \
        $$PWD/qcocoaprintersupport.h \
        $$PWD/qcocoaprintdevice.h \

    QT += widgets-private printsupport-private
}

OTHER_FILES += $$PWD/cocoa.json

# Acccessibility debug support
# DEFINES += QT_COCOA_ENABLE_ACCESSIBILITY_INSPECTOR
# include ($$PWD/../../../../util/accessibilityinspector/accessibilityinspector.pri)

# Window debug support
#DEFINES += QT_COCOA_ENABLE_WINDOW_DEBUG

!qt_static {
    PLUGIN_TYPE = platforms
    PLUGIN_CLASS_NAME = QCocoaIntegrationPlugin
    !equals(TARGET, $$QT_DEFAULT_QPA_PLUGIN): PLUGIN_EXTENDS = -
    load(qt_plugin)
}

HEADERS += $$PWD/main-cocoa-platform.h
