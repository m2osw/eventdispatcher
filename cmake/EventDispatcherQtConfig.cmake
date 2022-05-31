# - Find EventDispatcherQt
#
# EVENTDISPATCHER_QT_FOUND        - System has EventDispatcherQt
# EVENTDISPATCHER_QT_INCLUDE_DIRS - The EventDispatcherQt include directories
# EVENTDISPATCHER_QT_LIBRARIES    - The libraries needed to use EventDispatcherQt
# EVENTDISPATCHER_QT_DEFINITIONS  - Compiler switches required for using EventDispatcherQt
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
    EVENTDISPATCHER_QT_INCLUDE_DIR
        eventdispatcher/qt_connection.h

    PATHS
        ENV EVENTDISPATCHER_QT_INCLUDE_DIR
        ENV EVENTDISPATCHER_INCLUDE_DIR
)

find_library(
    EVENTDISPATCHER_QT_LIBRARY
        eventdispatcher_qt

    PATHS
        ${EVENTDISPATCHER_QT_LIBRARY_DIR}
        ${EVENTDISPATCHER_LIBRARY_DIR}
        ENV EVENTDISPATCHER_QT_LIBRARY
        ENV EVENTDISPATCHER_LIBRARY
)

mark_as_advanced(
    EVENTDISPATCHER_QT_INCLUDE_DIR
    EVENTDISPATCHER_QT_LIBRARY
)

set(EVENTDISPATCHER_QT_INCLUDE_DIRS ${EVENTDISPATCHER_QT_INCLUDE_DIR})
set(EVENTDISPATCHER_QT_LIBRARIES    ${EVENTDISPATCHER_QT_LIBRARY})

include(FindPackageHandleStandardArgs)
find_package_handle_standard_args(
    EventDispatcherQt
    REQUIRED_VARS
        EVENTDISPATCHER_QT_INCLUDE_DIR
        EVENTDISPATCHER_QT_LIBRARY
)

# vim: ts=4 sw=4 et
