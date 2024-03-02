# Copyright (c) 2013-2024  Made to Order Software Corp.  All Rights Reserved
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
# 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA.

##
## cppprocess library
##
project(cppprocess)

# Put the version in the header file
configure_file(
    ${CMAKE_CURRENT_SOURCE_DIR}/version.h.in
    ${CMAKE_CURRENT_BINARY_DIR}/version.h
)

add_library(${PROJECT_NAME} SHARED
    io.cpp
    io_capture_pipe.cpp
    io_data_pipe.cpp
    io_error_file.cpp
    io_file.cpp
    io_input_file.cpp
    io_output_file.cpp
    io_pipe.cpp

    process.cpp
    process_info.cpp
    process_list.cpp

    version.cpp
)

target_include_directories(${PROJECT_NAME}
    PUBLIC
        ${LIBEXCEPT_INCLUDE_DIRS}
        ${SNAPDEV_INCLUDE_DIRS}
)

target_link_libraries(${PROJECT_NAME}
    ${LIBEXCEPT_LIBRARIES}
    eventdispatcher
    pthread
    dl
)

set_target_properties(${PROJECT_NAME} PROPERTIES
    VERSION
        ${EVENTDISPATCHER_VERSION_MAJOR}.${EVENTDISPATCHER_VERSION_MINOR}

    SOVERSION
        ${EVENTDISPATCHER_VERSION_MAJOR}
)

install(
    TARGETS
        ${PROJECT_NAME}

    RUNTIME DESTINATION
        bin

    LIBRARY DESTINATION
        lib

    ARCHIVE DESTINATION
        lib
)

install(
    FILES
        buffer.h
        exception.h

        io.h
        io_capture_pipe.h
        io_data_pipe.h
        io_error_file.h
        io_file.h
        io_input_file.h
        io_output_file.h
        io_pipe.h

        process.h
        process_info.h
        process_list.h

        ${CMAKE_CURRENT_BINARY_DIR}/version.h

    DESTINATION
        include/cppprocess
)


# vim: ts=4 sw=4 et
