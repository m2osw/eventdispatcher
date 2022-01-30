# - Try to find cppprocess
#
# Once done this will define
#
# CPPPROCESS_FOUND        - System has cppprocess
# CPPPROCESS_INCLUDE_DIRS - The cppprocess include directories
# CPPPROCESS_LIBRARIES    - The libraries needed to use cppprocess (none)
# CPPPROCESS_DEFINITIONS  - Compiler switches required for using cppprocess (none)
#
# License:
#
# Copyright (c) 2021-2022  Made to Order Software Corp.  All Rights Reserved
#
# https://snapwebsites.org/project/eventdispatcher
# contact@m2osw.com
#
# This program is free software; you can redistribute it and/or modify
# it under the terms of the GNU General Public License as published by
# the Free Software Foundation; either version 2 of the License, or
# (at your option) any later version.
#
# This program is distributed in the hope that it will be useful,
# but WITHOUT ANY WARRANTY; without even the implied warranty of
# MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
# GNU General Public License for more details.
#
# You should have received a copy of the GNU General Public License along
# with this program; if not, write to the Free Software Foundation, Inc.,
# 51 Franklin St, Fifth Floor, Boston, MA  02110-1301  USA

find_path(
    CPPPROCESS_INCLUDE_DIR
        cppprocess/version.h

    PATHS
        $ENV{CPPPROCESS_INCLUDE_DIR}
)

find_library(
    CPPPROCESS_LIBRARY
        cppprocess

    PATHS
        $ENV{CPPPROCESS_LIBRARY}
)

mark_as_advanced(
    CPPPROCESS_INCLUDE_DIR
    CPPPROCESS_LIBRARY
)

set(CPPPROCESS_INCLUDE_DIRS ${CPPPROCESS_INCLUDE_DIR})
set(CPPPROCESS_LIBRARIES    ${CPPPROCESS_LIBRARY})

include(FindPackageHandleStandardArgs)

# handle the QUIETLY and REQUIRED arguments and set CPPPROCESS_FOUND to
# TRUE if all listed variables are TRUE
find_package_handle_standard_args(
    CppProcess
    DEFAULT_MSG
    CPPPROCESS_INCLUDE_DIR
    CPPPROCESS_LIBRARY
)

# vim: ts=4 sw=4 et
