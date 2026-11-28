# Copyright (C) 2015 The XD Company Ltd.
# SPDX-License-Identifier: Apache-2.0

# Builds the formatter's logic with its self-contained lexer and the
# ignore-file parser, for the `format` tool and for the tests that link the
# formatter's rules directly.
STATIC_PLUGINS += config-parser ignore-parser project-parser console

INCLUDEPATH += $$PWD

HEADERS += \
    $$PWD/format-agent-guides.h \
    $$PWD/format-comment.h \
    $$PWD/format-config.h \
    $$PWD/format-document.h \
    $$PWD/format-file-result.h \
    $$PWD/format-growth.h \
    $$PWD/format-issue.h \
    $$PWD/format-line-spec.h \
    $$PWD/format-named-rule.h \
    $$PWD/format-options.h \
    $$PWD/format-process-runner.h \
    $$PWD/format-project-files.h \
    $$PWD/format-project-reader.h \
    $$PWD/format-result.h \
    $$PWD/format-rule.h \
    $$PWD/format-rules.h \
    $$PWD/format-runner.h \
    $$PWD/format-file-config.h \
    $$PWD/format-spacing.h \
    $$PWD/format-thread-runner.h \
    $$PWD/formatter.h \
    $$PWD/ast/ast-node.h \
    $$PWD/ast/ast-parser.h \
    $$PWD/ast/ast-printer.h \
    $$PWD/ast/ast-transforms.h \
    $$PWD/ast/comment-attacher.h \
    $$PWD/doc/doc-node.h \
    $$PWD/doc/doc-printer.h \
    $$PWD/lang/class-members.h \
    $$PWD/lang/comment-line.h \
    $$PWD/lang/member-scope.h \
    $$PWD/lexer/format-language.h \
    $$PWD/lexer/format-lexer.h \
    $$PWD/lexer/format-token.h \
    $$PWD/parser/class-helpers.h \
    $$PWD/parser/class-relations.h \
    $$PWD/parser/code-helpers.h \
    $$PWD/parser/condition-context.h \
    $$PWD/parser/condition-object.h \
    $$PWD/parser/count-object.h \
    $$PWD/parser/settings-object.h \
    $$PWD/parser/condition-layout.h \
    $$PWD/parser/comment-helpers.h \
    $$PWD/parser/comment-layout.h \
    $$PWD/parser/expression-layout.h \
    $$PWD/parser/layout-helpers.h \
    $$PWD/parser/type-mapping.h \
    $$PWD/parser/scope-conditions.h \
    $$PWD/rules/args-array.h \
    $$PWD/rules/template-object.h \
    $$PWD/rules/as-with.h \
    $$PWD/rules/code-backticks.h \
    $$PWD/rules/comment.h \
    $$PWD/rules/comment-case.h \
    $$PWD/rules/comment-period.h \
    $$PWD/rules/comment-width.h \
    $$PWD/rules/doc-placement.h \
    $$PWD/rules/doc-tag-sign.h \
    $$PWD/rules/mark-banner.h \
    $$PWD/rules/pattern-report.h \
    $$PWD/rules/prose.h \
    $$PWD/rules/stub-line.h \
    $$PWD/rules/text-report.h \
    $$PWD/rules/trailing-comment.h \
    $$PWD/rules/us-spelling.h

SOURCES += \
    $$PWD/format-agent-guides.cpp \
    $$PWD/format-document.cpp \
    $$PWD/format-line-spec.cpp \
    $$PWD/format-named-rule.cpp \
    $$PWD/format-process-runner.cpp \
    $$PWD/format-project-files.cpp \
    $$PWD/format-project-reader.cpp \
    $$PWD/format-rule.cpp \
    $$PWD/format-rules.cpp \
    $$PWD/format-runner.cpp \
    $$PWD/format-file-config.cpp \
    $$PWD/format-thread-runner.cpp \
    $$PWD/formatter.cpp \
    $$PWD/ast/ast-node.cpp \
    $$PWD/ast/ast-parser.cpp \
    $$PWD/ast/ast-printer.cpp \
    $$PWD/ast/ast-transforms.cpp \
    $$PWD/ast/comment-attacher.cpp \
    $$PWD/doc/doc-node.cpp \
    $$PWD/doc/doc-printer.cpp \
    $$PWD/lexer/format-language.cpp \
    $$PWD/lexer/format-lexer.cpp \
    $$PWD/parser/class-helpers.cpp \
    $$PWD/parser/class-relations.cpp \
    $$PWD/parser/code-helpers.cpp \
    $$PWD/parser/condition-context.cpp \
    $$PWD/parser/condition-object.cpp \
    $$PWD/parser/count-object.cpp \
    $$PWD/parser/settings-object.cpp \
    $$PWD/parser/condition-layout.cpp \
    $$PWD/parser/comment-helpers.cpp \
    $$PWD/parser/comment-layout.cpp \
    $$PWD/parser/expression-layout.cpp \
    $$PWD/parser/layout-helpers.cpp \
    $$PWD/parser/type-mapping.cpp \
    $$PWD/parser/scope-conditions.cpp \
    $$PWD/rules/args-array.cpp \
    $$PWD/rules/template-object.cpp \
    $$PWD/rules/as-with.cpp \
    $$PWD/rules/code-backticks.cpp \
    $$PWD/rules/comment.cpp \
    $$PWD/rules/comment-case.cpp \
    $$PWD/rules/comment-period.cpp \
    $$PWD/rules/comment-width.cpp \
    $$PWD/rules/doc-placement.cpp \
    $$PWD/rules/doc-tag-sign.cpp \
    $$PWD/rules/mark-banner.cpp \
    $$PWD/rules/pattern-report.cpp \
    $$PWD/rules/prose.cpp \
    $$PWD/rules/stub-line.cpp \
    $$PWD/rules/text-report.cpp \
    $$PWD/rules/trailing-comment.cpp \
    $$PWD/rules/us-spelling.cpp

# Carries the prompt templates of agent mode (see FormatAgentGuides).
RESOURCES += $$PWD/assets/assets.qrc
