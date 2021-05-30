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

##
## eventdispatcher library
##
project(eventdispatcher)

# Put the version in the header file
configure_file(
    ${CMAKE_CURRENT_SOURCE_DIR}/version.h.in
    ${CMAKE_CURRENT_BINARY_DIR}/version.h
)

add_library(${PROJECT_NAME} SHARED
    # communicator
    communicator.cpp

        # message handling
        dispatcher_base.cpp
        dispatcher_support.cpp
        message.cpp

        # connections
        connection.cpp
        connection_with_send_message.cpp

        fd_connection.cpp
            fd_buffer_connection.cpp

        file_changed.cpp

        inter_thread_message_connection.cpp
        thread_done_signal.cpp

        pipe_connection.cpp
            pipe_buffer_connection.cpp
                pipe_message_connection.cpp

        signal.cpp
        signal_handler.cpp
        signal_profiler.cpp

        timer.cpp

        tcp_server_connection.cpp

        tcp_server_client_connection.cpp
            tcp_server_client_buffer_connection.cpp
                tcp_server_client_message_connection.cpp

        tcp_client_connection.cpp
            tcp_client_buffer_connection.cpp
                tcp_client_message_connection.cpp
                    tcp_blocking_client_message_connection.cpp
                tcp_client_permanent_message_connection.cpp

        udp_server_connection.cpp
            udp_server_message_connection.cpp
                logrotate_udp_messenger.cpp

    # tcp
    tcp_base.cpp
    tcp_private.cpp
    tcp_bio_options.cpp
    tcp_bio_client.cpp
    tcp_bio_server.cpp
    tcp_client.cpp
    tcp_server.cpp

    # udp
    udp_base.cpp
    udp_client.cpp
    udp_server.cpp

    # various
    utils.cpp
    version.cpp
)

include_directories(
    ${ADVGETOPT_INCLUDE_DIRS}
    ${CPPTHREAD_INCLUDE_DIRS}
    ${LIBEXCEPT_INCLUDE_DIRS}
    ${LIBUTF8_INCLUDE_DIRS}
    ${SNAPLOGGER_INCLUDE_DIRS}
    ${OPENSSL_INCLUDE_DIR}
    ${LIBADDR_INCLUDE_DIRS}
)

target_link_libraries(${PROJECT_NAME}
    ${ADVGETOPT_LIBRARIES}
    ${CPPTHREAD_LIBRARIES}
    ${LIBEXCEPT_LIBRARIES}
    ${LIBUTF8_LIBRARIES}
    ${SNAPLOGGER_LIBRARIES}
    ${OPENSSL_LIBRARIES}
    ${LIBADDR_LIBRARIES}
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
        communicator.h
        connection.h
        connection_with_send_message.h
        dispatcher_base.h
        dispatcher.h
        dispatcher_support.h
        exception.h
        fd_buffer_connection.h
        fd_connection.h
        file_changed.h
        inter_thread_message_connection.h
        logrotate_udp_messenger.h
        message.h
        pipe_buffer_connection.h
        pipe_connection.h
        pipe_message_connection.h
        signal.h
        signal_handler.h
        tcp_base.h
        tcp_bio_client.h
        tcp_bio_options.h
        tcp_bio_server.h
        tcp_blocking_client_message_connection.h
        tcp_client_buffer_connection.h
        tcp_client_connection.h
        tcp_client.h
        tcp_client_message_connection.h
        tcp_client_permanent_message_connection.h
        tcp_server_client_buffer_connection.h
        tcp_server_client_connection.h
        tcp_server_client_message_connection.h
        tcp_server_connection.h
        tcp_server.h
        thread_done_signal.h
        timer.h
        udp_base.h
        udp_client.h
        udp_server_connection.h
        udp_server.h
        udp_server_message_connection.h
        utils.h
        ${CMAKE_CURRENT_BINARY_DIR}/version.h

    DESTINATION
        include/eventdispatcher
)


# vim: ts=4 sw=4 et
