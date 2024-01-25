TEMPLATE = app

# TRACE/qmake/tests: never rely on mkspecs' default `RCC_DIR` value #3.
RCC_DIR = rcc
QMAKE_DIR_FIX *= RCC_DIR
#CONFIG -= debug_and_release

SOURCES = main.cpp

pro_file.files = resources.pro
pro_file.prefix = /prefix

subdir.files = subdir/file.txt
subdir.base = subdir

RESOURCES = test.qrc main.cpp pro_file subdir
