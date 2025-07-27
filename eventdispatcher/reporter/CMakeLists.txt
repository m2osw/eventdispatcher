# Copyright (c) 2012-2025  Made to Order Software Corp.  All Rights Reserved
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


##
## reporter library
##
project(reporter)

add_library(${PROJECT_NAME} SHARED
    basic_instructions.cpp
    direct_tcp_server_client.cpp
    direct_tcp_server.cpp
    executor.cpp
    expression.cpp
    instruction.cpp
    instruction_factory.cpp
    lexer.cpp
    parser.cpp
    state.cpp
    statement.cpp
    messenger_tcp_server_client.cpp
    messenger_tcp_server.cpp
    token.cpp
    variable.cpp
    variable_address.cpp
    variable_array.cpp
    variable_floating_point.cpp
    variable_integer.cpp
    variable_list.cpp
    variable_regex.cpp
    variable_string.cpp
    variable_timestamp.cpp
    variable_void.cpp
)

target_include_directories(${PROJECT_NAME}
    PUBLIC
        ${ADVGETOPT_INCLUDE_DIRS}
        ${CPPTHREAD_INCLUDE_DIRS}
        ${LIBADDR_INCLUDE_DIRS}
        ${LIBEXCEPT_INCLUDE_DIRS}
        ${LIBUTF8_INCLUDE_DIRS}
        ${OPENSSL_INCLUDE_DIR}
        ${SNAPLOGGER_INCLUDE_DIRS}
)

target_link_libraries(${PROJECT_NAME}
    ${ADVGETOPT_LIBRARIES}
    ${CPPTHREAD_LIBRARIES}
    ${LIBADDR_LIBRARIES}
    ${LIBEXCEPT_LIBRARIES}
    ${LIBUTF8_LIBRARIES}
    ${OPENSSL_LIBRARIES}
    ${SNAPLOGGER_LIBRARIES}
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
    DIRECTORY
        ${CMAKE_CURRENT_SOURCE_DIR}/

    DESTINATION
        include/eventdispatcher/reporter

    FILES_MATCHING PATTERN
        "*.h"
)


# vim: ts=4 sw=4 et
