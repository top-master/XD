# Copyright (C) 2015 The XD Company Ltd.
# SPDX-License-Identifier: Apache-2.0

# Builds TestBearer into a test, with the fixture reader it shares with the
# dummy bearer engine.
SOURCES += \
    $$PWD/testbearer.cpp \
    $$PWD/../../../src/plugins/bearer/dummy/bearerconfig.cpp
