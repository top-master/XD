#
# Minimal build-script for Windows to speed-up building, which
# also ensures `README.md` file is kept simple, without need to add:
# ```
# In `Qt-Creator` ensure side-bar's "`Edit`" tab is open, then
# in "Projects" side-panel find the `src` sub-folder, and right-click on said folder, then
# finally from the shown menu click "Build ..." option
# ```
#
# However, if you're advanced you should use `XD.pro` file instead, which
# allows you to run tests and benchmarks as well.
#

CONFIG += xd_mini_mode
include($$PWD/XD.pro)
