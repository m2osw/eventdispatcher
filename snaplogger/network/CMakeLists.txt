# Copyright (c) 2021-2025  Made to Order Software Corp.  All Rights Reserved
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
## snaplogger daemon appenders (i.e. network plugin extension)
##
project(snaplogger_network) # use underscores (_) for plugins

configure_file(
    ${CMAKE_CURRENT_SOURCE_DIR}/version.h.in
    ${CMAKE_CURRENT_BINARY_DIR}/version.h
)

add_library(${PROJECT_NAME} SHARED
    snaplogger_network.cpp

    alert_appender.cpp
    base_network_appender.cpp
    tcp_appender.cpp
    udp_appender.cpp
    version.cpp
)

target_include_directories(${PROJECT_NAME}
    PUBLIC
        ${ADVGETOPT_INCLUDE_DIRS}
        ${SNAPLOGGER_INCLUDE_DIRS}
        ${LIBADDR_INCLUDE_DIRS}
)

target_link_libraries(${PROJECT_NAME}
    eventdispatcher
    ${SNAPLOGGER_LIBRARIES}
)

install(
    TARGETS
        ${PROJECT_NAME}

    LIBRARY DESTINATION
        lib/snaplogger/plugins
)


install(
    FILES
        base_network_appender.h
        tcp_appender.h
        udp_appender.h
        ${CMAKE_CURRENT_BINARY_DIR}/version.h

    DESTINATION
        # Should it be along snaplogger files or eventdispatcher?
        # Since these are appenders, I think snaplogger makes more sense?
        #
        include/snaplogger/network
)


add_subdirectory(message-definitions)


# vim: ts=4 sw=4 et
