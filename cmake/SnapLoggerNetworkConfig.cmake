# - Find Snap! Logger Network extension
#
# SNAPLOGGER_NETWORK_FOUND        - System has the Snap! Logger Network extension
# SNAPLOGGER_NETWORK_INCLUDE_DIRS - The Snap! Logger Network include directories
# SNAPLOGGER_NETWORK_LIBRARIES    - The libraries needed to use Snap! Logger Network
# SNAPLOGGER_NETWORK_DEFINITIONS  - Compiler switches required for using Snap! Logger Network
#
# License:
#
# Copyright (c) 2011-2024  Made to Order Software Corp.  All Rights Reserved
#
# https://snapwebsites.org/project/eventdispatcher
# contact@m2osw.com
#
# This program is free software: you can redistribute it and/or modify
# it under the terms of the GNU General Public License as published by
# the Free Software Foundation, either version 3 of the License, or
# (at your option) any later version.
#
# This program is distributed in the hope that it will be useful,
# but WITHOUT ANY WARRANTY; without even the implied warranty of
# MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
# GNU General Public License for more details.
#
# You should have received a copy of the GNU General Public License
# along with this program.  If not, see <https://www.gnu.org/licenses/>.

find_path(
    SNAPLOGGER_NETWORK_INCLUDE_DIR
        snaplogger/network/version.h

    PATHS
        ENV SNAPLOGGER_NETWORK_INCLUDE_DIR
)

find_library(
    SNAPLOGGER_NETWORK_LIBRARY
        snaplogger-network

    PATHS
        ${SNAPLOGGER_NETWORK_LIBRARY_DIR}
        ENV SNAPLOGGER_NETWORK_LIBRARY
)

mark_as_advanced(
    SNAPLOGGER_NETWORK_INCLUDE_DIR
    SNAPLOGGER_NETWORK_LIBRARY
)

set(SNAPLOGGER_NETWORK_INCLUDE_DIRS ${SNAPLOGGER_NETWORK_INCLUDE_DIR})
set(SNAPLOGGER_NETWORK_LIBRARIES    ${SNAPLOGGER_NETWORK_LIBRARY})

include(FindPackageHandleStandardArgs)
find_package_handle_standard_args(
    EventDispatcher
    REQUIRED_VARS
        SNAPLOGGER_NETWORK_INCLUDE_DIR
        SNAPLOGGER_NETWORK_LIBRARY
)

# vim: ts=4 sw=4 et
