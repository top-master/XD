TEMPLATE = subdirs
SUBDIRS = \
    test/qthreadstorage_tst.pro

!winrt: SUBDIRS += crashonexit

CONFIG += ordered parallel_test
