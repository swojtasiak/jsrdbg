#.rst:
# FindJSRDBG
# ----------
#
#
#
# Locate jsrdbg library, a remote debugger for SpiderMonkey.
#
#     https://github.com/swojtasiak/jsrdbg.git
#
# Expects jsrdbg to be used as follows::
#
#     #include <jsrdbg/jsrdbg.h>
#
# This module defines
#
# ::
#
#   JSRDBG_FOUND          - if false, do not try to link to jsrdbg
#   JSRDBG_LIBRARIES      - where to find libjsrdbg.so
#   JSRDBG_INCLUDE_DIR    - where to find jsrdbg.h header

#=============================================================================
# Copyright 2016 otris software AG


find_library(JSRDBG_LIBRARIES
    NAME jsrdbg
    PATHS /usr/local)

find_path(JSRDBG_INCLUDE_DIR
    NAME jsrdbg/jsrdbg.h
    PATH_SUFFIXES include)

#message(STATUS "${JSRDBG_LIBRARIES}, ${JSRDBG_INCLUDE_DIR}")

include(FindPackageHandleStandardArgs)
# Handle the QUIETLY and REQUIRED arguments and set JSRDBG_FOUND to TRUE if all
# listed variables are TRUE
find_package_handle_standard_args(JSRDBG
    REQUIRED_VARS JSRDBG_LIBRARIES JSRDBG_INCLUDE_DIR)

# vim:et sw=4 ts=4
