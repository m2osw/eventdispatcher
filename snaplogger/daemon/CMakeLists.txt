# Copyright (c) 2021  Made to Order Software Corp.  All Rights Reserved
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

##
## snaplogger daemon
##
project(snaploggerd)

add_executable(${PROJECT_NAME} SHARED
    main.cpp
    snaploggerd.cpp
    tcp_connection.cpp
    udp_connection.cpp
)

include_directories(
    eventdispatcher
    ${ADVGETOPT_INCLUDE_DIRS}
    ${LIBADDR_INCLUDE_DIRS}
    ${LIBEXCEPT_INCLUDE_DIRS}
    ${SNAPLOGGER_INCLUDE_DIRS}

    #${LIBUFTF8_INCLUDE_DIRS}
    #${OPENSSL_INCLUDE_DIR}
)

target_link_libraries(${PROJECT_NAME}
    eventdispatcher
    ${SNAPLOGGER_LIBRARIES}
)

install(
    TARGETS
        ${PROJECT_NAME}

    RUNTIME DESTINATION
        sbin
)




##
## snaplogger daemon
##
project(snaploggerd)

add_library(${PROJECT_NAME} SHARED
    tcp_appender.cpp
    udp_appender.cpp
)

include_directories(
    ${ADVGETOPT_INCLUDE_DIRS}
    #${LIBEXCEPT_INCLUDE_DIRS}
    #${LIBUFTF8_INCLUDE_DIRS}
    ${SNAPLOGGER_INCLUDE_DIRS}
    #${OPENSSL_INCLUDE_DIR}
    #${LIBADDR_INCLUDE_DIRS}
)

target_link_libraries(${PROJECT_NAME}
    ${SNAPLOGGER_LIBRARIES}
)

set_target_properties(${PROJECT_NAME} PROPERTIES
    VERSION
        ${EVENTDISPATCHER_VERSION_MAJOR}.${SNAPLOGGER_VERSION_MINOR}

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
        tcp_appender.h
        udp_appender.h
        ${CMAKE_CURRENT_BINARY_DIR}/version.h

    DESTINATION
        # Should it be along snaplogger files or eventdispatcher?
        include/snaplogger
)



# vim: ts=4 sw=4 et
