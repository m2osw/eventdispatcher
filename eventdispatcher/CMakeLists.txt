# Copyright (c) 2012-2024  Made to Order Software Corp.  All Rights Reserved
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

AtomicNames(names.an)

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
        dispatcher.cpp
        dispatcher_match.cpp
        dispatcher_support.cpp
        message.cpp
        message_definition.cpp

        # connections
        connection.cpp
        connection_with_send_message.cpp

        fd_connection.cpp
            fd_buffer_connection.cpp
                cui_connection.cpp

        file_changed.cpp
        process_changed.cpp

        inter_thread_message_connection.cpp
        thread_done_signal.cpp

        local_dgram_server_connection.cpp
            local_dgram_server_message_connection.cpp

        local_stream_server_connection.cpp

        local_stream_server_client_connection.cpp
            local_stream_server_client_buffer_connection.cpp
                local_stream_server_client_message_connection.cpp

        local_stream_client_connection.cpp
            local_stream_client_buffer_connection.cpp
                local_stream_client_message_connection.cpp
                    local_stream_blocking_client_message_connection.cpp
                local_stream_client_permanent_message_connection.cpp

        pipe_connection.cpp
            pipe_buffer_connection.cpp
                pipe_message_connection.cpp

        signal.cpp
            signal_child.cpp
            signal_profiler.cpp
        signal_handler.cpp

        socket_events.cpp

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

    # Unix datagram
    local_dgram_base.cpp
    local_dgram_client.cpp
    local_dgram_server.cpp

    # tcp
    tcp_base.cpp
    tcp_private.cpp
    tcp_bio_options.cpp
    tcp_bio_client.cpp
    tcp_bio_server.cpp
    tcp_client.cpp      # basic class, not used with the communicator
    tcp_server.cpp      # basic class, not used with the communicator

    # udp
    udp_base.cpp
    udp_client.cpp
    udp_server.cpp

    # various
    ${CMAKE_CURRENT_BINARY_DIR}/names.cpp
    utils.cpp
    version.cpp
)

# the linux header includes a &= with an enumb which the compiler doesn't like
set_source_files_properties(process_changed.cpp PROPERTIES COMPILE_FLAGS -fpermissive)

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
    ncurses
    readline
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
        broadcast_message.h
        communicator.h
        connection.h
        connection_with_send_message.h
        cui_connection.h
        dispatcher.h
        dispatcher_match.h
        dispatcher_support.h
        exception.h
        fd_buffer_connection.h
        fd_connection.h
        file_changed.h
        inter_thread_message_connection.h
        local_dgram_base.h
        local_dgram_client.h
        local_dgram_server_connection.h
        local_dgram_server.h
        local_dgram_server_message_connection.h
        local_stream_blocking_client_message_connection.h
        local_stream_client_buffer_connection.h
        local_stream_client_connection.h
        local_stream_client_message_connection.h
        local_stream_client_permanent_message_connection.h
        local_stream_server_client_buffer_connection.h
        local_stream_server_client_connection.h
        local_stream_server_client_message_connection.h
        local_stream_server_connection.h
        logrotate_udp_messenger.h
        message.h
        message_definition.h
        ${CMAKE_CURRENT_BINARY_DIR}/names.h
        pipe_buffer_connection.h
        pipe_connection.h
        pipe_message_connection.h
        process_changed.h
        signal.h
        signal_child.h
        signal_handler.h
        socket_events.h
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


##
## Generate the reporter library
##
add_subdirectory(reporter)


##
## Install the message definitions
##
add_subdirectory(message-definitions)


# vim: ts=4 sw=4 et
