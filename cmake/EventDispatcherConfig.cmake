# - Try to find EventDispatcher
#
# Once done this will define
#
# EVENTDISPATCHER_FOUND        - System has EventDispatcher
# EVENTDISPATCHER_INCLUDE_DIRS - The EventDispatcher include directories
# EVENTDISPATCHER_LIBRARIES    - The libraries needed to use EventDispatcher (none)
# EVENTDISPATCHER_DEFINITIONS  - Compiler switches required for using EventDispatcher (none)
#
# License:
#
# Copyright (c) 2013-2022  Made to Order Software Corp.  All Rights Reserved
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
# You should have received a copy of the GNU General Public License
# along with this program; if not, write to the Free Software
# Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA  02110-1301  USA

find_path(
    EVENTDISPATCHER_INCLUDE_DIR
        eventdispatcher/version.h

    PATHS
        $ENV{EVENTDISPATCHER_INCLUDE_DIR}
)

find_library(
    EVENTDISPATCHER_LIBRARY
        eventdispatcher

    PATHS
        $ENV{EVENTDISPATCHER_LIBRARY}
)

mark_as_advanced(
    EVENTDISPATCHER_INCLUDE_DIR
    EVENTDISPATCHER_LIBRARY
)

set(EVENTDISPATCHER_INCLUDE_DIRS ${EVENTDISPATCHER_INCLUDE_DIR})
set(EVENTDISPATCHER_LIBRARIES    ${EVENTDISPATCHER_LIBRARY})

include(FindPackageHandleStandardArgs)

# handle the QUIETLY and REQUIRED arguments and set EVENTDISPATHCER_FOUND to
# TRUE if all listed variables are TRUE
find_package_handle_standard_args(
    EventDispatcher
    DEFAULT_MSG
    EVENTDISPATCHER_INCLUDE_DIR
    EVENTDISPATCHER_LIBRARY
)

# vim: ts=4 sw=4 et
