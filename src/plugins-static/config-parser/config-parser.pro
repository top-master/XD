# Copyright (C) 2015 The XD Company Ltd.
# SPDX-License-Identifier: Apache-2.0

# Describes the static plugin in mkspecs/plugins-static, for the projects
# that name it in `STATIC_PLUGINS`.
TEMPLATE = aux
CONFIG -= qt

include($$PWD/config-parser.pri)
load(static_plugins)
staticPluginDescribe(config-parser)
