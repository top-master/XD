
# XD 5 framework

This repository contains XD (e**X**tra **D**imension),
the cross-platform framework used by both `GUI` and `CLI` applications.

Based on `qtbase` [5.6.x](https://github.com/qt/qtbase),
which's a bundle of Qt libraries that are enough to replace both `boost` and `wxWidgets`,\
in other words, the other Qt libraries are nice to have,\
but they are really just fancy additions, unused by most projects.

However, if needed `XD` provides an all in one bundle which includes this bundle as a git-sub-module, at:\
https://github.com/top-master/qt5

**WARNING:** use the word `Qt` instead of `XD` in your search for solutions, like in StackOverflow.com,
and our header files are intentionally compatible with Qt headers to make that possible.

> **Naming:** Considering we see only a three-dimensional reality, and most people only see original `Qt5`, then our additions are the extra dimension.

For more information read other `README.md` or `README` files related to each directory.

---

## Download / Clone
We make use of `git-sub-module` feature,
and you will need to `clone` with below command to include all sub-modules:

>git clone --recurse-submodules https://github.org/top-master/qtbase.git --depth=1 --branch main

But if you already cloned without `--recurse-submodules`,
then you will need to run below command to fetch any sub-module:

>git submodule update --init --recursive --depth=1

---

## Build
### Pre-requisite:
1. **[Qt Creator](https://www.qt.io/offline-installers)** 5.x or later ([github](https://github.com/qt-creator/qt-creator)).
    - **Note** that  CDB-debugging plugin of Qt-Creator version 4 works for version 5 as well.
    - Also, version 5.x is last that supports Windows 7.

2. **[MSVC 2015 (Microsoft Visual Studio 2015)](https://www.google.com/search?q=download%20Microsoft%20Visual%20Studio%202015%20Community)**
    - Just used as compiler, not as IDE, hence installing should be enough (no need to purchase).
    - Microsoft's C++ Build tools 2015 may work as well (we only tested with MSVC).

### Steps:
1. Install both '`Qt-Creator`' and '`MSVC 2015`'
(then `Qt-Creator` should automatically detect `MSVC 2015` as compiler when both are installed).

2. Download `XD`-framework, and place it at '`C:/XD`', like:
    - First open the `terminal` and `cd` into your '`C:/`' drive (or for advanced users, wherever you want).
    - Then `clone` this repository with above mentioned command.
    - Wait until download is finished successfully.
    - At last ensure the '`C:/XD/bin`' directory is created and contains a prebuilt `qmake` named executable in it (but if `qmake` does not exist there, download one from `Qt-5`)

3. Open `Qt-Creator`, then [Add Qt-version and configure it](./doc/qt-creator/project-qmake.md) like:
    - Put in "`qmake location`" field the `XD` framework's qmake-executable address (e.g. "`C:/XD/bin/qmake.exe`" without quotes).
    - Also set the "`Version name`" field to "`MyXD`" (without quotes).

4. Still in `Qt-Creator` [Create a build tool-kit and configure it](./doc/qt-creator/targets.md) like:
    - In "`Name`" Text-box type "`MyToolKit`" without quotes.
    - In "`Qt Version`" Drop-down menu select "`MyXD`" (which you added in previous step)
    - In `"Compiler"` Drop-down menu select the auto-detected one with `x86` architecture (or even [configure compiler](./doc/qt-creator/tool-chains.md) manually)

5. In `Qt-Creator` open as project our main build-script file which is "`XD-mini.pro`" file, or for advanced users "`XD.pro`", then configure it to use the build tool-kit (which you added in previous step), like:
    - The "`Configure Project`" wizard should appear right after open.
    - **WARNING:** if said wizard does **not** appear, then do either of these:
        - Close Qt-Creator, and remove the "`C:/XD/XD-mini.pro.user`" file or if advanced `XD.pro.user` file, finally re-open said main build-script file in Qt-Creator.
        - Or [configure project](./doc/qt-creator/configuring-projects.md) manually.
    - In said wizard, tick the "`MyToolKit`" checkbox, and un-tick any other kit.
    - Expand the "`Details`" of said checkbox.
    - In both "`Debug`" and "`Release`" text-boxes type "C:/build/XD-tmp" without quotes (any path is allowed except "`C:/XD`" folder or its sub-folders).

6. In `Qt-Creator` click "Build All" from "Build" menu
   (note that both `debug` and `release` libraies get built, unlike normal projects,
   hence no need for switching to `release` mode, using `Ctrl+T` menu).

7. Optionally, create the `XD_ROOT` named environment-variable, and point it to `C:/XD` (directory which second step creates).


All done, `XD` framework is ready to use!
