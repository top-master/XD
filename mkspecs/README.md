
This does NOT attempt to be comprehensive and/or replace Qt documentation,
just few notes.

## Naming: file extensions
* The `.pro` file extension is reserved for user projects.
* `.pri` is reserved for scripts said `.pro` files may `include`.
* `.prf` is reserved for scripts in `mkspecs` folder, like features,
  for example, a `.pro` file can enable `QRemote` features like:
    ```
    CONFIG += remote
    ```

## CONFIG and/or conditions

* prefix_build : if set, under other means that Qt binaries
  (like `.exe`, `.dll` and `.so` files) should NOT be moved to source-root's `bin` folder.
  and that another custom-folder should prefix and/or contain `bin` and `lib` folders.

* debug_and_release : is set by default, and if reamins set after `.pro` file, then
  means to create `Makefile`s for both `debug` and `release` build-types in that same QMake run,
  for which to work, QMake auto-sets `build_pass`, and passes through entire script from begining,
  and said pass happens two times, once for each build-type, first for IDE's active build-type.
  See also `QMAKE_DIR_REPLACE_SANE` and `QMAKE_DIR_REPLACE`.

* build_pass : don't set manually, see `debug_and_release` for details,
  but this itself means that:
    - If set, do NOT execute things which if done once are enough for all build-types, example:
        ```
        !build_pass: warning(Failed to find MySQL SDK, at: \"$$MY_PATH\" - defaulting to SQLite)
        ```
    - If set, execute things that change depending on build-type, example:
        ```
        load(exclusive_builds_post) # Fix paths now (load works only in XD).

        build_pass {
            CONFIG(debug, debug|release) {
                copy_file(myDebugFile.txt, $$DESTDIR/myFile.txt)
            } else {
                copy_file(myReleaseFile.txt, $$DESTDIR/myFile.txt)
            }
        }
        ```

* compat_verbose : is set by default, XD warns about Qt-to-XD migration issues, **if** detected.

* fix_output_dirs : enables `QMAKE_DIR_FIX` even without `debug_and_release` being set.

## Variables

* CONFIG : a list of enabled features, which is specially treated by QMake,
    for example, each entry is checkable as condition, like:
    ```
    # Assume in some other file:
    CONFIG += my_feature

    if (my_feature) { # In your project.
        myFunc()
    }
    ```
    > **Note** that above causes QMake to load `mkspecs/features/my_feature.prf` file (if it exists).

* QMAKE_DIR_FIX : is ignored unless `debug_and_release` or `fix_output_dirs` is set, otherwise,
  is a list of variable-names, where each named variable's value should be a folder path,
  said path will be changed based on build-type,
  so that each built-type has an exclusive/ separate sub-folder
  (i.e. QMake may affix `debug` or `release` sub-folder to path, depending on build-type).

* QMAKE_DIR_REPLACE and QMAKE_DIR_REPLACE_SANE : both deprecated now,
  but were similar to `QMAKE_DIR_FIX` in older Qt projects,
  where "affix" was little different (and *could* have spaces).

-----------------------------------------------------------------------

## Qt Modules

Below statements only apply to projects that do `load(qt_module)`.

### Variables

* MODULE_PRIVATE_INCLUDES : Same as adding to `INCLUDEPATH`, but allows projects using the
  `QT += network_private` config to find internally used headers, like OpenSSL's.
