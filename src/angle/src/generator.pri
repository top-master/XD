# This file regenerates the GLSL scanners and parsers that the compiler
# projects build from the source tree, out of their flex and bison files.

# The flex and bison that `angleGenerate()` runs; a qmake command line may name
# other ones, such as `ANGLE_FLEX=win_flex ANGLE_BISON=win_bison`.
isEmpty(ANGLE_FLEX): ANGLE_FLEX = flex
isEmpty(ANGLE_BISON): ANGLE_BISON = bison

# angleGenerate(tool, input, outputs)
#
# Regenerates the tracked `outputs` of the GLSL scanner or parser `input`
# with `tool` (`flex` or `bison`), when that tool runs here; otherwise the
# build compiles the tracked copies as they are.
# * Regenerates only when `input` changed (see `angleInputChanged()`), or an
#   output is missing; hence another version of a tool leaves the tracked
#   outputs as they are.
# * Runs in the first pass of each qmake run only; hence the debug and the
#   release builds never write the same file at once.
# * Works in a scratch folder of its own, then replaces each output whose
#   content changed; hence an unchanged output keeps its modified-time.
# * Makes the Makefile depend on `input`; hence an edit of it runs qmake,
#   and this function, again.
defineTest(angleGenerate) {
    tool = $$1
    input = $$2
    outputs = $$3
    QMAKE_INTERNAL_INCLUDED_FILES *= $$input
    export(QMAKE_INTERNAL_INCLUDED_FILES)
    build_pass: return(true)
    changed = false
    angleInputChanged($$input): changed = true
    for(output, outputs) {
        !exists($$output): changed = true
    }
    !equals(changed, true): return(true)

    scratch = $$angleScratchPath()
    mkpath($$scratch)
    equals(tool, flex) {
        # Runs from the input's folder; hence the `#line` that flex writes for
        # the `%top` block names the bare file.
        outfile = --outfile=$$system_path($$scratch/$$basename(outputs))
        command = $$angleChangeDir($$dirname(input)) && \
            $$ANGLE_FLEX --noline --nounistd $$system_quote($$outfile) \
            $$system_quote($$basename(input))
    } else {
        # Runs from the scratch folder; hence the outputs, and the header guard
        # that bison names after its header, carry bare file names.
        options =
        for(output, outputs) {
            name = $$basename(output)
            contains(name, ".*\\.h"): options += --defines=$$name
            else: options += --output=$$name
        }
        command = $$angleChangeDir($$scratch) && \
            $$ANGLE_BISON --no-lines --skeleton=yacc.c $$options \
            $$system_quote($$system_path($$input))
    }

    system($$command $$angleNullSink()) {
        for(output, outputs) {
            angleReplaceChanged($$scratch/$$basename(output), $$output)
        }
    } else {
        message("Kept the tracked output of $$basename(input):" \
                "$$tool did not run.")
    }
    angleRemoveDir($$scratch)
    return(true)
}

# angleInputChanged(input)
#
# Tells whether `input` differs from what git's `HEAD` holds: an edit, staged
# or not, or a file that git does not track. Without git, or outside a git
# checkout, the input counts as changed.
defineTest(angleInputChanged) {
    input = $$1
    name = $$system_quote($$basename(input))
    null = $$angleNullSink()
    command = $$angleChangeDir($$dirname(input)) && \
        git ls-files --error-unmatch -- $$name $$null && \
        git diff --quiet HEAD -- $$name $$null
    system($$command): return(false)
    return(true)
}

# angleScratchPath()
#
# Names a build folder that only this qmake run uses.
defineReplace(angleScratchPath) {
    equals(QMAKE_HOST.os, Windows) {
        id = $$system(echo %RANDOM%%RANDOM%)
    } else {
        id = $$system(echo $PPID)
    }
    return($$OUT_PWD/angle-generate-$$id)
}

# angleChangeDir(dir)
#
# Builds the command that enters `dir` in the shell of system().
defineReplace(angleChangeDir) {
    dir = $$system_quote($$system_path($$1))
    equals(QMAKE_HOST.os, Windows): return(cd /d $$dir)
    return(cd $$dir)
}

# angleNullSink()
#
# Builds the redirection that drops a command's output and its errors.
defineReplace(angleNullSink) {
    equals(QMAKE_HOST.os, Windows): return(>NUL 2>&1)
    return(>/dev/null 2>&1)
}

# angleReplaceChanged(source, target)
#
# Copies the file `source` over `target`, when their contents differ.
defineTest(angleReplaceChanged) {
    source = $$system_quote($$system_path($$1))
    target = $$system_quote($$system_path($$2))
    null = $$angleNullSink()
    equals(QMAKE_HOST.os, Windows) {
        system(fc /B $$source $$target $$null): return(true)
        system(copy /Y $$source $$target $$null): return(true)
    } else {
        system(cmp -s $$source $$target): return(true)
        system(cp -f $$source $$target): return(true)
    }
    return(false)
}

# angleRemoveDir(dir)
#
# Removes the folder `dir` and everything in it.
defineTest(angleRemoveDir) {
    dir = $$system_quote($$system_path($$1))
    null = $$angleNullSink()
    equals(QMAKE_HOST.os, Windows): system(rmdir /S /Q $$dir $$null)
    else: system(rm -rf $$dir)
    return(true)
}
