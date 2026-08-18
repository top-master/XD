
#
# Handles specific tasks (if passed to qmake CLI).
#

# Using Auxiliary-method (aux).
TEMPLATE = aux
build_pass: return()

# Validate CLI arguments.
for(ARG, QMAKE_ARGS) {
    contains(ARG, ^\\s*TASK\\s*=.*) {
        eval($$ARG)
    } else {
        contains(ARG, ^\\s*ARG\\d\\s*=.*) {
            eval($$ARG)
        }
    }
}
isEmpty(TASK): error(qmake has no TASK command line)

# MARK: Execute requested task.

equals(TASK, "touch") {
    exists($$OUT_PWD/Makefile): \
        touch($$ARG1, $$OUT_PWD/Makefile)
    else {
        tmp = $$basename(ARG1)
        tmp = $$replace(tmp, \\.[^.]*$, )
        tmp = Makefile.$$tmp
        exists($$OUT_PWD/$$tmp): \
            touch($$ARG1, $$OUT_PWD/$$tmp)
        else: \
            warning(Failed to find Makefile or $$tmp at: \"$$OUT_PWD\" for file: \"$$ARG1\")
    }
}

# removeObjects(folders)
#
# Removes the object files of each folder in `folders` (relative to the build
# folder, the one make runs the task in), several per command. cmd.exe refuses
# a command line past 8191 characters; hence a batch stays at 25 paths.
defineTest(removeObjects) {
    equals(QMAKE_HOST.os, Windows): remove = del /f /q
    else: remove = rm -f
    for(folder, ARGS) {
        folder = $$absolute_path($$folder, $$OUT_PWD)
        objects = $$files($$folder/*$$QMAKE_EXT_OBJ)
        batch =
        for(object, objects) {
            batch += $$system_quote($$system_path($$object))
            count(batch, 25) {
                system($$remove $$batch)
                batch =
            }
        }
        !isEmpty(batch): system($$remove $$batch)
    }
    return(true)
}

equals(TASK, "remove-objects") {
    removeObjects($$ARG1 $$ARG2 $$ARG3 $$ARG4 $$ARG5)
}
