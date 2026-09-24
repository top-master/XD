CONFIG += testcase
TARGET = tst_qftp
SOURCES  += tst_qftp.cpp

requires(contains(QT_CONFIG,private_tests))
QT = core-private network-private testlib

wince {
   addFiles.files = rfc3252.txt
   addFiles.path = .
   DEPLOYMENT += addFiles
}

