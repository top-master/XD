# Copyright (C) 2015 The XD Company Ltd.
# SPDX-License-Identifier: Apache-2.0

# Lists the settings parser (`.ini` files read level by level from a project's
# root, checked against a tool's schema) for `STATIC_PLUGINS`.
PLUGIN_HEADERS = \
    config-any-space.h \
    config-array-obj.h \
    config-choice.h \
    config-condition.h \
    config-escape.h \
    config-evaluator-context.h \
    config-key.h \
    config-name.h \
    config-numbers.h \
    config-object.h \
    config-parser.h \
    config-regexp.h \
    config-replacement.h \
    config-scanner.h \
    config-schema.h \
    config-scopes.h \
    config-section.h \
    config-settings.h \
    config-types.h \
    config-value-matcher.h \
    config-word.h
PLUGIN_SOURCES = \
    config-any-space.cpp \
    config-array-obj.cpp \
    config-choice.cpp \
    config-condition.cpp \
    config-escape.cpp \
    config-evaluator-context.cpp \
    config-name.cpp \
    config-numbers.cpp \
    config-object.cpp \
    config-parser.cpp \
    config-regexp.cpp \
    config-replacement.cpp \
    config-scanner.cpp \
    config-schema.cpp \
    config-scopes.cpp \
    config-section.cpp \
    config-types.cpp \
    config-value-matcher.cpp \
    config-word.cpp
PLUGIN_QT = core
PLUGIN_DEPENDS =
