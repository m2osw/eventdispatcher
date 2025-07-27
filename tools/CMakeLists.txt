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
## check-certificate
##
project(check-certificate)

add_executable(${PROJECT_NAME}
    check_certificate.cpp
)

target_include_directories(${PROJECT_NAME}
    PUBLIC
        ${ADVGETOPT_INCLUDE_DIRS}
)

target_link_libraries(${PROJECT_NAME}
    eventdispatcher
)

install(
    TARGETS
        ${PROJECT_NAME}

    RUNTIME DESTINATION
        bin
)


##
## ed-signal
##
project(ed-signal)

add_executable(${PROJECT_NAME}
    ed_signal.cpp
)

target_include_directories(${PROJECT_NAME}
    PUBLIC
        ${ADVGETOPT_INCLUDE_DIRS}
)

target_link_libraries(${PROJECT_NAME}
    eventdispatcher
)

install(
    TARGETS
        ${PROJECT_NAME}

    RUNTIME DESTINATION
        bin
)


##
## ed-stop
##
project(ed-stop)

add_executable(${PROJECT_NAME}
    ed_stop.cpp
)

target_include_directories(${PROJECT_NAME}
    PUBLIC
        ../cppprocess
        ${ADVGETOPT_INCLUDE_DIRS}
)

target_link_libraries(${PROJECT_NAME}
    cppprocess
    eventdispatcher
)

install(
    TARGETS
        ${PROJECT_NAME}

    RUNTIME DESTINATION
        bin
)


##
## verify-message-definitions
##
project(verify-message-definitions)

add_executable(${PROJECT_NAME}
    verify_message_definitions.cpp
)

target_include_directories(${PROJECT_NAME}
    PUBLIC
        ../cppprocess
        ${ADVGETOPT_INCLUDE_DIRS}
)

target_link_libraries(${PROJECT_NAME}
    eventdispatcher
)

install(
    TARGETS
        ${PROJECT_NAME}

    RUNTIME DESTINATION
        bin
)


##
## Verify the message definitions defined in the eventdispatcher
##
#
# Unfortunate, but the command used to verify the message definitions is
# created in this project and the CMake macro is added by this project so
# other projects can simply use a one liner which is not available to us
# here. It's not that bad, though.
#
project(verify-eventdispatcher-message-definitions)

set(VERIFIED_EVENTDISPATCHER_MESSAGE_DEFINITIONS ${PROJECT_BINARY_DIR}/verify-eventdispatcher-message-definitions)

add_custom_command(
    OUTPUT
        ${VERIFIED_EVENTDISPATCHER_MESSAGE_DEFINITIONS}

    COMMAND
        echo "--- verifying message definitions ---"

    COMMAND
        ${CMAKE_COMMAND} -E env "LD_LIBRARY_PATH=${CMAKE_BINARY_DIR}/eventdispatcher"
            "${CMAKE_BINARY_DIR}/tools/verify-message-definitions"
                "--path-to-message-definitions"
                    "${CMAKE_SOURCE_DIR}/snaplogger/network/message-definitions:${CMAKE_SOURCE_DIR}/eventdispatcher/message-definitions"
                "${CMAKE_SOURCE_DIR}/snaplogger/network/message-definitions/*.conf"
                "${CMAKE_SOURCE_DIR}/eventdispatcher/message-definitions/*.conf"

    COMMAND
        "touch"
            "${VERIFIED_EVENTDISPATCHER_MESSAGE_DEFINITIONS}"

    DEPENDS
        verify-message-definitions
        "${CMAKE_SOURCE_DIR}/snaplogger/network/message-definitions/*.conf"
        "${CMAKE_SOURCE_DIR}/eventdispatcher/message-definitions/*.conf"
)

add_custom_target(${PROJECT_NAME} ALL
    DEPENDS
        ${VERIFIED_EVENTDISPATCHER_MESSAGE_DEFINITIONS}
)


# vim: ts=4 sw=4 et
