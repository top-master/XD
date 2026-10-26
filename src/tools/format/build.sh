#!/bin/sh
# Copyright (C) 2015 The XD Company Ltd.
# SPDX-License-Identifier: Apache-2.0

# Builds the format tool alone, in `qt_static` mode (see `bh_template_app`);
# hence the tool runs without the shared Qt libraries. `--no-static` builds
# it shared instead, in a build folder of its own.
#
# XD's build that the template runs first builds only the parts of XD the tool
# needs (BH_XD_BUILD_FILTER): QtCore, the tools that build it (`bootstrap`,
# `moc`, and `rcc`), and the static plugins the tool compiles in.
# `--no-xd-build` leaves XD as it is.

# Gets the absolute path of this script's directory.
cd "${0%[/\\]*}" > /dev/null 2>&1
ROOT=$(pwd)
XD_DIR=$(cd ../../.. && pwd)

BH_ROOT=$ROOT
BH_SCRIPT_NAME=build.sh
. "$XD_DIR/tools/build-handler.sh"

BH_STATIC='^.*$'
BH_XD_BUILD_FILTER='^src/(3rdparty/pcre/pcre|tools/(bootstrap/bootstrap|moc/moc|rcc/rcc)|corelib/corelib|plugins-static/[a-z-]+/[a-z-]+)\.pro$'

# Builds in XD's own build folder, where XD's builds put the tool (see
# bh_xd_module_build_dir), and shows XD's folder as the build folder.
bh_hook args-parsed <<'}'
    BUILD_DIR=$(bh_xd_module_build_dir) || exit 1
}
bh_hook_add display-build-dir bh_xd_build_dir

bh_template_app "$ROOT/format.pro" "$@"
