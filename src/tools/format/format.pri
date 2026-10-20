# Copyright (C) 2015 The XD Company Ltd.
# SPDX-License-Identifier: Apache-2.0

# Builds the formatter's logic on top of moc's tokenizer and the ignore-file
# parser, for the `format` tool and for the tests that link the formatter's
# rules directly.
include($$PWD/../moc/moc.pri)
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
    $$PWD/format-settings.h \
    $$PWD/format-spacing.h \
    $$PWD/format-thread-runner.h \
    $$PWD/formatter.h \
    $$PWD/lang/class-members.h \
    $$PWD/lang/comment-line.h \
    $$PWD/lang/member-scope.h \
    $$PWD/parser/class-helpers.h \
    $$PWD/parser/code-helpers.h \
    $$PWD/parser/comment-helpers.h \
    $$PWD/parser/expression-layout.h \
    $$PWD/parser/layout-helpers.h \
    $$PWD/parser/scope-conditions.h \
    $$PWD/rules/args-wrap.h \
    $$PWD/rules/as-with.h \
    $$PWD/rules/code-backticks.h \
    $$PWD/rules/code-shape.h \
    $$PWD/rules/comment-case.h \
    $$PWD/rules/comment-period.h \
    $$PWD/rules/comment-width.h \
    $$PWD/rules/comment.h \
    $$PWD/rules/condition-close.h \
    $$PWD/rules/condition-wrap.h \
    $$PWD/rules/connect-layout.h \
    $$PWD/rules/const-overload.h \
    $$PWD/rules/continuation-indent.h \
    $$PWD/rules/control-braces.h \
    $$PWD/rules/doc-placement.h \
    $$PWD/rules/excludes.h \
    $$PWD/rules/global-static.h \
    $$PWD/rules/init-list.h \
    $$PWD/rules/inline-keyword.h \
    $$PWD/rules/mark-banner.h \
    $$PWD/rules/member-init.h \
    $$PWD/rules/moc-run.h \
    $$PWD/rules/no-lambda.h \
    $$PWD/rules/not-space.h \
    $$PWD/rules/one-statement.h \
    $$PWD/rules/operator-first.h \
    $$PWD/rules/operator-wrap.h \
    $$PWD/rules/outline-this.h \
    $$PWD/rules/param-wrap.h \
    $$PWD/rules/pattern-report.h \
    $$PWD/rules/private-access.h \
    $$PWD/rules/prose.h \
    $$PWD/rules/qll.h \
    $$PWD/rules/qpointer-getter.h \
    $$PWD/rules/reference-space.h \
    $$PWD/rules/relative-include.h \
    $$PWD/rules/stub-line.h \
    $$PWD/rules/super-typedef.h \
    $$PWD/rules/template-close.h \
    $$PWD/rules/ternary-wrap.h \
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
    $$PWD/format-settings.cpp \
    $$PWD/format-thread-runner.cpp \
    $$PWD/formatter.cpp \
    $$PWD/parser/class-helpers.cpp \
    $$PWD/parser/code-helpers.cpp \
    $$PWD/parser/comment-helpers.cpp \
    $$PWD/parser/expression-layout.cpp \
    $$PWD/parser/layout-helpers.cpp \
    $$PWD/parser/scope-conditions.cpp \
    $$PWD/rules/args-wrap.cpp \
    $$PWD/rules/as-with.cpp \
    $$PWD/rules/code-backticks.cpp \
    $$PWD/rules/code-shape.cpp \
    $$PWD/rules/comment-case.cpp \
    $$PWD/rules/comment-period.cpp \
    $$PWD/rules/comment-width.cpp \
    $$PWD/rules/comment.cpp \
    $$PWD/rules/condition-close.cpp \
    $$PWD/rules/condition-wrap.cpp \
    $$PWD/rules/connect-layout.cpp \
    $$PWD/rules/const-overload.cpp \
    $$PWD/rules/continuation-indent.cpp \
    $$PWD/rules/control-braces.cpp \
    $$PWD/rules/doc-placement.cpp \
    $$PWD/rules/excludes.cpp \
    $$PWD/rules/global-static.cpp \
    $$PWD/rules/init-list.cpp \
    $$PWD/rules/inline-keyword.cpp \
    $$PWD/rules/mark-banner.cpp \
    $$PWD/rules/member-init.cpp \
    $$PWD/rules/moc-run.cpp \
    $$PWD/rules/no-lambda.cpp \
    $$PWD/rules/not-space.cpp \
    $$PWD/rules/one-statement.cpp \
    $$PWD/rules/operator-first.cpp \
    $$PWD/rules/operator-wrap.cpp \
    $$PWD/rules/outline-this.cpp \
    $$PWD/rules/param-wrap.cpp \
    $$PWD/rules/pattern-report.cpp \
    $$PWD/rules/private-access.cpp \
    $$PWD/rules/prose.cpp \
    $$PWD/rules/qll.cpp \
    $$PWD/rules/qpointer-getter.cpp \
    $$PWD/rules/reference-space.cpp \
    $$PWD/rules/relative-include.cpp \
    $$PWD/rules/stub-line.cpp \
    $$PWD/rules/super-typedef.cpp \
    $$PWD/rules/template-close.cpp \
    $$PWD/rules/ternary-wrap.cpp \
    $$PWD/rules/text-report.cpp \
    $$PWD/rules/trailing-comment.cpp \
    $$PWD/rules/us-spelling.cpp

# Carries the prompt templates of agent mode (see FormatAgentGuides).
RESOURCES += $$PWD/assets/assets.qrc
