@echo off

@rem WARNING: this overwrites existing file.
@rem
@rem Note that `%1` should already be quoted, like by QMake-generated Makefile.

@call "%~dp0upx-%2.exe" -qq --best %1
