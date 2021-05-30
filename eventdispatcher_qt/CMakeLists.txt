# Copyright (c) 2012-2021  Made to Order Software Corp.  All Rights Reserved
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

# TODO: add a protection in case Qt is not installed (i.e. this should be
#       optional) -- for now, edit the main CMakeLists.txt and command out
#       the following command:
#           add_subdirectory(eventdispatcher_qt)

##
## eventdispatcher_qt library
##
project(eventdispatcher_qt)

include_directories(
    ${SNAPDEV_INCLUDE_DIRS}
    ${Qt5Core_INCLUDE_DIRS}
    ${Qt5Widgets_INCLUDE_DIRS}
)

set(HEADERS
    qt_connection.h
)

add_library(${PROJECT_NAME} SHARED
    ${HEADERS}
    qt_connection.cpp                    # qt connection extension
)

target_link_libraries(${PROJECT_NAME}
    ${LIBEXCEPT_LIBRARIES}
    ${LIBUTF8_LIBRARIES}
    ${QT_LIBRARIES}
    Qt5::X11Extras
    eventdispatcher
)

set_target_properties( ${PROJECT_NAME} PROPERTIES
    VERSION
        ${EVENTDISPATCHER_VERSION_MAJOR}.${EVENTDISPATCHER_VERSION_MINOR}

    SOVERSION
        ${EVENTDISPATCHER_VERSION_MAJOR}
)

install(
    TARGETS
        ${PROJECT_NAME}

    LIBRARY DESTINATION
        lib
)

install(
    DIRECTORY
        ${CMAKE_CURRENT_SOURCE_DIR}/

    DESTINATION
        include/eventdispatcher

    FILES_MATCHING PATTERN
        "*.h"
)

# vim: ts=4 sw=4 et nocindent
