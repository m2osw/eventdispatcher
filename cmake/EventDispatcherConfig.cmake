# - Find EventDispatcher
#
# EVENTDISPATCHER_FOUND        - System has EventDispatcher
# EVENTDISPATCHER_INCLUDE_DIRS - The EventDispatcher include directories
# EVENTDISPATCHER_LIBRARIES    - The libraries needed to use EventDispatcher
# EVENTDISPATCHER_DEFINITIONS  - Compiler switches required for using EventDispatcher
#
# License:
#
# Copyright (c) 2011-2022  Made to Order Software Corp.  All Rights Reserved
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
    EVENTDISPATCHER_INCLUDE_DIR
        eventdispatcher/version.h

    PATHS
        ENV EVENTDISPATCHER_INCLUDE_DIR
)

find_library(
    EVENTDISPATCHER_LIBRARY
        eventdispatcher

    PATHS
        ${EVENTDISPATCHER_LIBRARY_DIR}
        ENV EVENTDISPATCHER_LIBRARY
)

mark_as_advanced(
    EVENTDISPATCHER_INCLUDE_DIR
    EVENTDISPATCHER_LIBRARY
)

set(EVENTDISPATCHER_INCLUDE_DIRS ${EVENTDISPATCHER_INCLUDE_DIR})
set(EVENTDISPATCHER_LIBRARIES    ${EVENTDISPATCHER_LIBRARY})

include(FindPackageHandleStandardArgs)
find_package_handle_standard_args(
    EventDispatcher
    REQUIRED_VARS
        EVENTDISPATCHER_INCLUDE_DIR
        EVENTDISPATCHER_LIBRARY
)

# vim: ts=4 sw=4 et
