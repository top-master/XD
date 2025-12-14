
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
