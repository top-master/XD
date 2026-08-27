CONFIG += testcase
testcase.timeout = 600 # this test is slow
CONFIG -= app_bundle debug_and_release_target
SOURCES  += ../tst_qnetworkreply.cpp
TARGET = ../tst_qnetworkreply

QT = core-private network-private testlib
RESOURCES += ../qnetworkreply.qrc

TESTDATA += ../empty ../rfc3252.txt ../resource ../bigfile ../*.jpg ../certs \
            ../index.html ../smb-file.txt

# Copy the fixture data NEXT TO THE BINARY ($$OUT_PWD/.. is where TARGET=../tst_qnetworkreply lands,
# i.e. QCoreApplication::applicationDirPath()). QFINDTESTDATA() prefers the binary dir over the source
# tree, so with the data present here the bundled server's --folder resolves to this WRITABLE build
# dir. That matters because the server now mutates its config at run time -- test-driven edits plus
# the IniWatcher's lastPull ack -- and those writes must never touch the checked-in server-config.ini.
serverdata.files = $$PWD/../rfc3252.txt $$PWD/../bigfile $$PWD/../index.html \
                   $$PWD/../smb-file.txt $$PWD/../empty $$PWD/../resource \
                   $$files($$PWD/../*.jpg)
serverdata.path = $$OUT_PWD/..
COPIES += serverdata
# A COPIES entry may not mix files and directories, so the certs/ dir is its own entry.
serverdatacerts.files = $$PWD/../certs
serverdatacerts.path = $$OUT_PWD/..
COPIES += serverdatacerts
# server-config.ini is rewritten at run time (edits + lastPull), so COPIES -- which skips once the
# destination is newer than the source -- would leave a stale copy after the first run. copyFileLater
# (xd_functions.prf) copies post-link on every build, so each run starts from the pristine committed
# config; it also spares us the hand-rolled $$QMAKE_COPY / shell_path / escape_expand incantation.
copyFileLater($$PWD/../server-config.ini, $$OUT_PWD/..)

contains(QT_CONFIG,xcb): CONFIG+=insignificant_test  # unstable, QTBUG-21102
win32:CONFIG += insignificant_test # QTBUG-24226

!winrt: TEST_HELPER_INSTALLS = ../echo/echo
