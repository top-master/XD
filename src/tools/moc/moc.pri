
INCLUDEPATH += $$PWD

HEADERS =  $$PWD/moc.h \
           $$PWD/moc-config.h \
           $$PWD/preprocessor.h \
           $$PWD/parser.h \
           $$PWD/symbols.h \
           $$PWD/token.h \
           $$PWD/utils.h \
           $$PWD/generator.h \
           $$PWD/outputrevision.h \
           $$PWD/parser/function-parser.h \
           $$PWD/parser/function-mapper.h

SOURCES =  $$PWD/moc.cpp \
           $$PWD/preprocessor.cpp \
           $$PWD/generator.cpp \
           $$PWD/parser.cpp \
           $$PWD/token.cpp \
           $$PWD/utils.cpp \
           $$PWD/parser/function-mapper.cpp
