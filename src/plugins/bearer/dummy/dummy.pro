TARGET = qbearerdummy

QT = core-private network-private

SOURCES += dummyengine.cpp
OTHER_FILES += dummy.json

PLUGIN_TYPE = bearer
PLUGIN_CLASS_NAME = DummyBearerPlugin
load(qt_plugin)
