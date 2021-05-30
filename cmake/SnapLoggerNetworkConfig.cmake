# - Try to find EventDispatcher
#
# Once done this will define
#
# SNAPLOGGER_NETWORK_FOUND        - System has the Snap! Logger Network extension
# SNAPLOGGER_NETWORK_INCLUDE_DIRS - The Snap! Logger Network include directories
# SNAPLOGGER_NETWORK_LIBRARIES    - The libraries needed to use Snap! Logger Network (none)
# SNAPLOGGER_NETWORK_DEFINITIONS  - Compiler switches required for using Snap! Logger Network (none)
#
# License:
#
# Copyright (c) 2013-2021  Made to Order Software Corp.  All Rights Reserved
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
    SNAPLOGGER_NETWORK_INCLUDE_DIR
        snaplogger/network/version.h

    PATHS
        $ENV{SNAPLOGGER_NETWORK_INCLUDE_DIR}
)

find_library(
    SNAPLOGGER_NETWORK_LIBRARY
        snaplogger-network

    PATHS
        $ENV{SNAPLOGGER_NETWORK_LIBRARY}
)

mark_as_advanced(
    SNAPLOGGER_NETWORK_INCLUDE_DIR
    SNAPLOGGER_NETWORK_LIBRARY
)

set(SNAPLOGGER_NETWORK_INCLUDE_DIRS ${SNAPLOGGER_NETWORK_INCLUDE_DIR})
set(SNAPLOGGER_NETWORK_LIBRARIES    ${SNAPLOGGER_NETWORK_LIBRARY})

include(FindPackageHandleStandardArgs)

# handle the QUIETLY and REQUIRED arguments and set SNAPLOGGER_NETWORK_FOUND
# to TRUE if all listed variables are TRUE
find_package_handle_standard_args(
    EventDispatcher
    DEFAULT_MSG
    SNAPLOGGER_NETWORK_INCLUDE_DIR
    SNAPLOGGER_NETWORK_LIBRARY
)

# vim: ts=4 sw=4 et
