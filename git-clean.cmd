@echo off

@rem Clean but keep already built.

git clean -d -x -f ^
    -e /todo/** ^
    -e /bin/** ^
    -e /lib/** ^
    -e /mkspecs/modules/** ^
    -e /plugins/**

pause
