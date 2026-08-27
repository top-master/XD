TEMPLATE = subdirs

# XD test-support bearer engine (pairs with the bearer-dummy daemon + TestBearer): a controllable
# set of fake, activatable network configurations so the headless bearer tests have configurations
# and QNetworkSession can actually open/close one. Publishes nothing unless $QT_BEARER_DUMMY_PORT
# points at a running bearer-dummy, so it is inert in normal use.
SUBDIRS += dummy

!android:linux*:qtHaveModule(dbus) {
    SUBDIRS += generic
    SUBDIRS += connman networkmanager
}

#win32:SUBDIRS += nla
win32:SUBDIRS += generic
blackberry:SUBDIRS += blackberry
win32:!wince:!winrt: SUBDIRS += nativewifi
mac:contains(QT_CONFIG, corewlan):SUBDIRS += corewlan
mac:SUBDIRS += generic
android:!android-no-sdk:SUBDIRS += android

isEmpty(SUBDIRS):SUBDIRS = generic
