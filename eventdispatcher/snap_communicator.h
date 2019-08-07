// Copyright (c) 2012-2019  Made to Order Software Corp.  All Rights Reserved
//
// This program is free software; you can redistribute it and/or modify
// it under the terms of the GNU General Public License as published by
// the Free Software Foundation; either version 2 of the License, or
// (at your option) any later version.
//
// This program is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU General Public License for more details.
//
// You should have received a copy of the GNU General Public License
// along with this program; if not, write to the Free Software
// Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA  02110-1301  USA
#pragma once

// self
//
#include "eventdispatcher/tcp_client_server.h"
#include "eventdispatcher/udp_client_server.h"
#include "eventdispatcher/utils.h"


// cppthread lib
//
#include "cppthread/thread.h"


// snapdev lib
//
#include "snapdev/not_used.h"


// C lib
//
#include <signal.h>
#include <sys/signalfd.h>



namespace ed
{
// forward class declaration
class snap_tcp_client_permanent_message_connection_impl;













// WARNING: a snap_communicator object must be allocated and held in a shared pointer (see pointer_t)
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Weffc++"
#pragma GCC diagnostic ignored "-Wnon-virtual-dtor"
class snap_communicator
    : public std::enable_shared_from_this<snap_communicator>
{
public:
    typedef std::shared_ptr<snap_communicator>      pointer_t;




















    class snap_tcp_server_client_connection
        : public snap_connection
        //, public tcp_client_server::tcp_client -- this will not work without some serious re-engineering of the tcp_client class
    {
    public:
        typedef std::shared_ptr<snap_tcp_server_client_connection>    pointer_t;

                                    snap_tcp_server_client_connection(tcp_client_server::bio_client::pointer_t client);
        virtual                     ~snap_tcp_server_client_connection() override;

        void                        close();
        size_t                      get_client_address(struct sockaddr_storage & address) const;
        std::string                 get_client_addr() const;
        int                         get_client_port() const;
        std::string                 get_client_addr_port() const;

        // snap_connection implementation
        virtual bool                is_reader() const override;
        virtual int                 get_socket() const override;

        // new callbacks
        virtual ssize_t             read(void * buf, size_t count);
        virtual ssize_t             write(void const * buf, size_t count);

    private:
        bool                        define_address();

        tcp_client_server::bio_client::pointer_t
                                    f_client = tcp_client_server::bio_client::pointer_t();
        struct sockaddr_storage     f_address = sockaddr_storage();
        socklen_t                   f_length = 0;
    };

    class snap_tcp_server_client_buffer_connection
        : public snap_tcp_server_client_connection
    {
    public:
        typedef std::shared_ptr<snap_tcp_server_client_buffer_connection>    pointer_t;

                                    snap_tcp_server_client_buffer_connection(tcp_client_server::bio_client::pointer_t client);

        bool                        has_input() const;
        bool                        has_output() const;

        // snap::snap_communicator::snap_connection
        virtual bool                is_writer() const override;

        // snap::snap_communicator::snap_tcp_server_client_connection implementation
        virtual ssize_t             write(void const * data, size_t const length) override;
        virtual void                process_read() override;
        virtual void                process_write() override;
        virtual void                process_hup() override;

        // new callback
        virtual void                process_line(QString const & line) = 0;

    private:
        std::string                 f_line = std::string(); // input -- do NOT use QString because UTF-8 would break often... (since we may only receive part of messages)
        std::vector<char>           f_output = std::vector<char>();
        size_t                      f_position = 0;
    };

    class snap_tcp_server_client_message_connection
        : public snap_tcp_server_client_buffer_connection
        , public snap_dispatcher_support
        , public connection_with_send_message
    {
    public:
        typedef std::shared_ptr<snap_tcp_server_client_message_connection>    pointer_t;

                                    snap_tcp_server_client_message_connection(tcp_client_server::bio_client::pointer_t client);

        QString const &             get_remote_address() const;

        // connection_with_send_message
        virtual bool                send_message(snap_communicator_message const & message, bool cache = false) override;

        // snap_tcp_server_client_buffer_connection implementation
        virtual void                process_line(QString const & line) override;

    private:
        QString                     f_remote_address = QString();
    };

    class snap_tcp_client_buffer_connection
        : public snap_tcp_client_connection
    {
    public:
        typedef std::shared_ptr<snap_tcp_client_buffer_connection>    pointer_t;

                                    snap_tcp_client_buffer_connection(std::string const & addr, int port, mode_t const mode = mode_t::MODE_PLAIN, bool const blocking = false);

        bool                        has_input() const;
        bool                        has_output() const;

        // snap::snap_communicator::snap_tcp_client_connection implementation
        virtual ssize_t             write(void const * data, size_t length) override;
        virtual bool                is_writer() const override;
        virtual void                process_read() override;
        virtual void                process_write() override;
        virtual void                process_hup() override;

        // new callback
        virtual void                process_line(QString const & line) = 0;

    private:
        std::string                 f_line = std::string(); // input -- do NOT use QString because UTF-8 would break often... (since we may only receive part of messages)
        std::vector<char>           f_output = std::vector<char>();
        size_t                      f_position = 0;
    };

    class snap_tcp_client_message_connection
        : public snap_tcp_client_buffer_connection
        , public snap_dispatcher_support
        , public connection_with_send_message
    {
    public:
        typedef std::shared_ptr<snap_tcp_client_message_connection>    pointer_t;

                                    snap_tcp_client_message_connection(std::string const & addr, int port, mode_t const mode = mode_t::MODE_PLAIN, bool const blocking = false);

        // connection_with_send_message
        virtual bool                send_message(snap_communicator_message const & message, bool cache = false) override;

        // snap_tcp_client_reader_connection implementation
        virtual void                process_line(QString const & line) override;
    };

    class snap_tcp_client_permanent_message_connection
        : public snap_timer
        , public snap_dispatcher_support
        , public connection_with_send_message
    {
    public:
        typedef std::shared_ptr<snap_tcp_client_permanent_message_connection>    pointer_t;

        static int64_t const        DEFAULT_PAUSE_BEFORE_RECONNECTING = 60LL * 1000000LL;  // 1 minute

                                    snap_tcp_client_permanent_message_connection(std::string const & address, int port, tcp_client_server::bio_client::mode_t mode = tcp_client_server::bio_client::mode_t::MODE_PLAIN, int64_t const pause = DEFAULT_PAUSE_BEFORE_RECONNECTING, bool const use_thread = true);
        virtual                     ~snap_tcp_client_permanent_message_connection() override;

        bool                        is_connected() const;
        void                        disconnect();
        void                        mark_done();
        void                        mark_done(bool messenger);
        size_t                      get_client_address(struct sockaddr_storage & address) const;
        std::string                 get_client_addr() const;

        // connection_with_send_message
        virtual bool                send_message(snap_communicator_message const & message, bool cache = false) override;

        // snap_connection implementation
        virtual void                process_timeout() override;
        virtual void                process_error() override;
        virtual void                process_hup() override;
        virtual void                process_invalid() override;
        virtual void                connection_removed() override;

        // new callbacks
        virtual void                process_connection_failed(std::string const & error_message);
        virtual void                process_connected();

    private:
        std::shared_ptr<snap_tcp_client_permanent_message_connection_impl>
                                    f_impl = std::shared_ptr<snap_tcp_client_permanent_message_connection_impl>();
        int64_t                     f_pause = 0;
        bool const                  f_use_thread = true;
    };

    class snap_udp_server_connection
        : public snap_connection
        , public udp_client_server::udp_server
    {
    public:
        typedef std::shared_ptr<snap_udp_server_connection>    pointer_t;

                                    snap_udp_server_connection(std::string const & addr, int port);

        // snap_connection implementation
        virtual bool                is_reader() const override;
        virtual int                 get_socket() const override;

        void                        set_secret_code(std::string const & secret_code);
        std::string const &         get_secret_code() const;

    private:
        std::string                 f_secret_code = std::string();
    };

    class snap_udp_server_message_connection
        : public snap_udp_server_connection
        , public snap_dispatcher_support
    {
    public:
        typedef std::shared_ptr<snap_udp_server_message_connection>    pointer_t;

        static size_t const         DATAGRAM_MAX_SIZE = 1024;

                                    snap_udp_server_message_connection(std::string const & addr, int port);

        static bool                 send_message(std::string const & addr
                                               , int port
                                               , snap_communicator_message const & message
                                               , std::string const & secret_code = std::string());

        // snap_connection implementation
        virtual void                process_read() override;

    private:
    };

    class snap_tcp_blocking_client_message_connection
        : public snap_tcp_client_message_connection
    {
    public:
                                    snap_tcp_blocking_client_message_connection(std::string const & addr, int port, mode_t mode = mode_t::MODE_PLAIN);

        void                        run();
        void                        peek();

        // connection_with_send_message
        virtual bool                send_message(snap_communicator_message const & message, bool cache = false) override;

        // snap_connection callback
        virtual void                process_error() override;

    private:
        std::string                 f_line = std::string();
    };

    static pointer_t                    instance();

    // prevent copies
                                        snap_communicator(snap_communicator const & communicator) = delete;
    snap_communicator &                 operator = (snap_communicator const & communicator) = delete;

    snap_connection::vector_t const &   get_connections() const;
    bool                                add_connection(snap_connection::pointer_t connection);
    bool                                remove_connection(snap_connection::pointer_t connection);
    virtual bool                        run();

private:
                                        snap_communicator();

    snap_connection::vector_t           f_connections = snap_connection::vector_t();
    bool                                f_force_sort = true;
};
#pragma GCC diagnostic pop


} // namespace ed
// vim: ts=4 sw=4 et
