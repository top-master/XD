# Copyright (C) 2015 The XD Company Ltd.
# SPDX-License-Identifier: Apache-2.0

# Lists the console helpers (a logger for a tool's output and warnings, and
# the prompts that ask the person at the terminal) for `STATIC_PLUGINS`.
PLUGIN_HEADERS = console.h logger.h
PLUGIN_SOURCES = console.cpp logger.cpp
PLUGIN_QT = core
PLUGIN_DEPENDS =
