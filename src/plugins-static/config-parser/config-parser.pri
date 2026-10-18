# Copyright (C) 2015 The XD Company Ltd.
# SPDX-License-Identifier: Apache-2.0

# Lists the settings parser (`.ini` files read level by level from a project's
# root, checked against a tool's schema) for `STATIC_PLUGINS`.
PLUGIN_HEADERS = \
    config-any-space.h \
    config-condition.h \
    config-escape.h \
    config-key.h \
    config-numbers.h \
    config-parser.h \
    config-regexp.h \
    config-replacement.h \
    config-scanner.h \
    config-schema.h \
    config-section.h \
    config-value-matcher.h \
    config-word.h
PLUGIN_SOURCES = \
    config-any-space.cpp \
    config-condition.cpp \
    config-escape.cpp \
    config-numbers.cpp \
    config-parser.cpp \
    config-regexp.cpp \
    config-replacement.cpp \
    config-scanner.cpp \
    config-schema.cpp \
    config-section.cpp \
    config-value-matcher.cpp \
    config-word.cpp
PLUGIN_QT = core
PLUGIN_DEPENDS =
