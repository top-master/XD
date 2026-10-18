# Copyright (C) 2015 The XD Company Ltd.
# SPDX-License-Identifier: Apache-2.0

# Lists the ignore-file parser (git's `.gitignore` rules, and the names a
# tool appends, such as `.formatignore`) for `STATIC_PLUGINS`.
PLUGIN_HEADERS = ignore-parser.h ignore-rule.h
PLUGIN_SOURCES = ignore-parser.cpp
PLUGIN_QT = core
PLUGIN_DEPENDS = config-parser
