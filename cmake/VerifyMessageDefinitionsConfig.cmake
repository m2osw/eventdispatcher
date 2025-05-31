# Copyright (c) 2024-2025  Made to Order Software Corp.  All Rights Reserved
#
# http://snapwebsites.org/project/eventdispatcher
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
#
################################################################################
#
# File:         VerifyMessageDefinitionsConfig.cmake
# Object:       Provide function to verify message definitions at compile time.
#
include(CMakeParseArguments)

# First try to find the one we just compiled (developer environment)
get_filename_component(CMAKE_BINARY_PARENT_DIR ${CMAKE_BINARY_DIR} DIRECTORY)
get_filename_component(CMAKE_BINARY_PARENT_DIR_NAME ${CMAKE_BINARY_PARENT_DIR} NAME)
if(${CMAKE_BINARY_PARENT_DIR_NAME} STREQUAL "coverage")
    # we have a sub-sub-directory when building coverage
    get_filename_component(CMAKE_BINARY_PARENT_DIR ${CMAKE_BINARY_PARENT_DIR} DIRECTORY)
    get_filename_component(CMAKE_BINARY_PARENT_DIR ${CMAKE_BINARY_PARENT_DIR} DIRECTORY)
endif()
find_program(
    VERIFY_MESSAGE_DEFINITIONS_PROGRAM
        verify-message-definitions

    HINTS
        ${CMAKE_BINARY_PARENT_DIR}/eventdispatcher/tools
        ${CMAKE_BINARY_PARENT_DIR}/contrib/eventdispatcher/tools

    NO_DEFAULT_PATH
)

# Second, if the first find_program() came up empty handed, try again to
# find an installed version (i.e. generally under /usr/bin)
# This one is marked as REQUIRED.
find_program(
    VERIFY_MESSAGE_DEFINITIONS_PROGRAM
        verify-message-definitions

    REQUIRED
)

if(${VERIFY_MESSAGE_DEFINITIONS_PROGRAM} STREQUAL "VERIFY_MESSAGE_DEFINITIONS_PROGRAM-NOTFOUND")
    message(FATAL_ERROR "verify-message-definitions tool not found")
endif()

# This function verifies that message definitions are valid
#
# \param[in] PATH_TO_MESSAGE_DEFINITIONS  A path to a set of message definitions.
#
function(VerifyMessageDefinitions PATH_TO_MESSAGE_DEFINITIONS)
    cmake_parse_arguments(PARSE_ARGV 1 "" "" "" "")

    file(RELATIVE_PATH RELATIVE_SOURCE_DIR ${CMAKE_SOURCE_DIR} ${CMAKE_CURRENT_SOURCE_DIR})
    string(REPLACE "/" "_" INTRODUCER ${RELATIVE_SOURCE_DIR})

    project(${INTRODUCER}_VerifyMessageDefinitions)

    set(VERIFIED_EVENTDISPATCHER_MESSAGE_DEFINITIONS ${CMAKE_CURRENT_BINARY_DIR}/verify-eventdispatcher-message-definitions)

    add_custom_command(
        OUTPUT
            ${VERIFIED_EVENTDISPATCHER_MESSAGE_DEFINITIONS}

        COMMAND
            echo "--- verifying message definitions ---"

        COMMAND
            "${VERIFY_MESSAGE_DEFINITIONS_PROGRAM}"
                "--path-to-message-definitions"
                    "${PATH_TO_MESSAGE_DEFINITIONS}"
                "${PATH_TO_MESSAGE_DEFINITIONS}/*.conf"

        COMMAND
            "touch"
                "${VERIFIED_EVENTDISPATCHER_MESSAGE_DEFINITIONS}"

        DEPENDS
            "${PATH_TO_MESSAGE_DEFINITIONS}/*.conf"
    )

    add_custom_target(${PROJECT_NAME} ALL
        DEPENDS
            ${VERIFIED_EVENTDISPATCHER_MESSAGE_DEFINITIONS}
    )
endfunction()

# vim: ts=4 sw=4 et
