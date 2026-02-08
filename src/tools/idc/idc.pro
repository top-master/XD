#
# what this is:
#
# This idc TOOL (Interface Description Compiler) is used by qmake
# for ActiveQt based projects to generate "*.idl" files.
#
# Then those will be converted to "*.tlb" files automaticly by qmake using microsofts "midl.exe" tool
# found at "%ProgramFiles%\Microsoft SDKs\Windows\v7.0A\bin"
#
# MSDN: The MIDL compiler compiles scripts that are written in the Object Description Language (ODL).
# Microsoft has expanded the Interface Definition Language (IDL) to contain the complete ODL syntax.
# (i.e. "*.idl" files are newer then "*.odl" files)
#

option(host_build)
CONFIG += force_bootstrap

SOURCES = main.cpp

load(qt_tool)
