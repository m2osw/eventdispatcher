# - Try to find EventDispatcherQt
#
# Once done this will define
#
# EVENTDISPATCHER_QT_FOUND        - System has EventDispatcherQt
# EVENTDISPATCHER_QT_INCLUDE_DIRS - The EventDispatcherQt include directories
# EVENTDISPATCHER_QT_LIBRARIES    - The libraries needed to use EventDispatcherQt (none)
# EVENTDISPATCHER_QT_DEFINITIONS  - Compiler switches required for using EventDispatcherQt (none)
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
# You should have received a copy of the GNU General Public License
# along with this program; if not, write to the Free Software
# Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA  02110-1301  USA

find_path(
    EVENTDISPATCHER_QT_INCLUDE_DIR
        eventdispatcher/connection.h

    PATHS
        $ENV{EVENTDISPATCHER_INCLUDE_DIR}
)

find_library(
    EVENTDISPATCHER_QT_LIBRARY
        eventdispatcher_qt

    PATHS
        $ENV{EVENTDISPATCHER_LIBRARY}
)

mark_as_advanced(
    EVENTDISPATCHER_QT_INCLUDE_DIR
    EVENTDISPATCHER_QT_LIBRARY
)

set(EVENTDISPATCHER_QT_INCLUDE_DIRS ${EVENTDISPATCHER_QT_INCLUDE_DIR})
set(EVENTDISPATCHER_QT_LIBRARIES    ${EVENTDISPATCHER_QT_LIBRARY})

include(FindPackageHandleStandardArgs)

# handle the QUIETLY and REQUIRED arguments and set EVENTDISPATHCER_QT_FOUND to
# TRUE if all listed variables are TRUE
find_package_handle_standard_args(
    EventDispatcherQt
    DEFAULT_MSG
    EVENTDISPATCHER_QT_INCLUDE_DIR
    EVENTDISPATCHER_QT_LIBRARY
)

# vim: ts=4 sw=4 et
