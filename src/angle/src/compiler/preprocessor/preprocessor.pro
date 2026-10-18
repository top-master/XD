CONFIG += static
contains(QT_CONFIG, dynamicgl): CONFIG += not_installed
include(../../config.pri)
include(../../generator.pri)

INCLUDEPATH = \
    $$ANGLE_DIR/src/compiler/preprocessor \
    $$ANGLE_DIR/src/compiler \
    # End INCLUDEPATH.

DEFINES += _SECURE_SCL=0


HEADERS += \
    $$ANGLE_DIR/src/compiler/preprocessor/DiagnosticsBase.h \
    $$ANGLE_DIR/src/compiler/preprocessor/DirectiveHandlerBase.h \
    $$ANGLE_DIR/src/compiler/preprocessor/DirectiveParser.h \
    $$ANGLE_DIR/src/compiler/preprocessor/ExpressionParser.h \
    $$ANGLE_DIR/src/compiler/preprocessor/Input.h \
    $$ANGLE_DIR/src/compiler/preprocessor/length_limits.h \
    $$ANGLE_DIR/src/compiler/preprocessor/Lexer.h \
    $$ANGLE_DIR/src/compiler/preprocessor/Macro.h \
    $$ANGLE_DIR/src/compiler/preprocessor/MacroExpander.h \
    $$ANGLE_DIR/src/compiler/preprocessor/numeric_lex.h \
    $$ANGLE_DIR/src/compiler/preprocessor/pp_utils.h \
    $$ANGLE_DIR/src/compiler/preprocessor/Preprocessor.h \
    $$ANGLE_DIR/src/compiler/preprocessor/SourceLocation.h \
    $$ANGLE_DIR/src/compiler/preprocessor/Token.h \
    $$ANGLE_DIR/src/compiler/preprocessor/Tokenizer.h

SOURCES += \
    $$ANGLE_DIR/src/compiler/preprocessor/DiagnosticsBase.cpp \
    $$ANGLE_DIR/src/compiler/preprocessor/DirectiveHandlerBase.cpp \
    $$ANGLE_DIR/src/compiler/preprocessor/DirectiveParser.cpp \
    $$ANGLE_DIR/src/compiler/preprocessor/ExpressionParser.cpp \
    $$ANGLE_DIR/src/compiler/preprocessor/Input.cpp \
    $$ANGLE_DIR/src/compiler/preprocessor/Lexer.cpp \
    $$ANGLE_DIR/src/compiler/preprocessor/Macro.cpp \
    $$ANGLE_DIR/src/compiler/preprocessor/MacroExpander.cpp \
    $$ANGLE_DIR/src/compiler/preprocessor/Preprocessor.cpp \
    $$ANGLE_DIR/src/compiler/preprocessor/Token.cpp \
    $$ANGLE_DIR/src/compiler/preprocessor/Tokenizer.cpp

# Regenerates the scanner and the parser above, when flex and bison run here.
OTHER_FILES += \
    $$ANGLE_DIR/src/compiler/preprocessor/ExpressionParser.y \
    $$ANGLE_DIR/src/compiler/preprocessor/Tokenizer.l
angleGenerate(flex, $$ANGLE_DIR/src/compiler/preprocessor/Tokenizer.l, \
    $$ANGLE_DIR/src/compiler/preprocessor/Tokenizer.cpp)
angleGenerate(bison, $$ANGLE_DIR/src/compiler/preprocessor/ExpressionParser.y, \
    $$ANGLE_DIR/src/compiler/preprocessor/ExpressionParser.cpp)
