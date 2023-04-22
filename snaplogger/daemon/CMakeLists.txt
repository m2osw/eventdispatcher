# Copyright (c) 2021-2023  Made to Order Software Corp.  All Rights Reserved
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
# 51 Franklin St, Fifth Floor, Boston, MA  02110-1301  USA

##
## snaplogger daemon
##
project(snaploggerd)

configure_file(
    ${CMAKE_CURRENT_SOURCE_DIR}/version.h.in
    ${CMAKE_CURRENT_BINARY_DIR}/version.h
)

add_executable(${PROJECT_NAME}
    main.cpp
    network_component.cpp
    snaploggerd.cpp
    tcp_logger_connection.cpp
    tcp_logger_server.cpp
    udp_logger_server.cpp
    utils.cpp
    version.cpp
)

target_include_directories(${PROJECT_NAME}
    PUBLIC
        ${ADVGETOPT_INCLUDE_DIRS}
        ${LIBADDR_INCLUDE_DIRS}
        ${LIBEXCEPT_INCLUDE_DIRS}
        ${SNAPLOGGER_INCLUDE_DIRS}
)

target_link_libraries(${PROJECT_NAME}
    eventdispatcher
    ${ADVGETOPT_LIBRARIES}
    ${LIBADDR_LIBRARIES}
    ${LIBEXCEPT_LIBRARIES}
    ${SNAPLOGGER_LIBRARIES}
)

install(
    TARGETS
        ${PROJECT_NAME}

    RUNTIME DESTINATION
        sbin
)



# vim: ts=4 sw=4 et
