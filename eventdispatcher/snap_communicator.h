// Event Dispatcher
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



namespace snap
{




class snap_communicator_message
{
public:
    typedef QMap<QString, QString>                  parameters_t;
    typedef std::vector<snap_communicator_message>  vector_t;

    bool                    from_message(QString const & message);
    QString                 to_message() const;

    QString const &         get_sent_from_server() const;
    void                    set_sent_from_server(QString const & server);
    QString const &         get_sent_from_service() const;
    void                    set_sent_from_service(QString const & service);
    QString const &         get_server() const;
    void                    set_server(QString const & server);
    QString const &         get_service() const;
    void                    set_service(QString const & service);
    void                    reply_to(snap_communicator_message const & message);
    QString const &         get_command() const;
    void                    set_command(QString const & command);
    void                    add_parameter(QString const & name, char const * value);
    void                    add_parameter(QString const & name, std::string const & value);
    void                    add_parameter(QString const & name, QString const & value);
    void                    add_parameter(QString const & name, int32_t value);
    void                    add_parameter(QString const & name, uint32_t value);
    void                    add_parameter(QString const & name, int64_t value);
    void                    add_parameter(QString const & name, uint64_t value);
    bool                    has_parameter(QString const & name) const;
    QString const           get_parameter(QString const & name) const;
    int64_t                 get_integer_parameter(QString const & name) const;
    parameters_t const &    get_all_parameters() const;

    static void             verify_name(QString const & name, bool can_be_empty = false, bool can_be_lowercase = true);

private:
    QString                 f_sent_from_server = QString();
    QString                 f_sent_from_service = QString();
    QString                 f_server = QString();
    QString                 f_service = QString();
    QString                 f_command = QString();
    parameters_t            f_parameters = parameters_t();
    mutable QString         f_cached_message = QString();
};



class dispatcher_base
{
public:
    typedef std::shared_ptr<dispatcher_base>    pointer_t;
    typedef std::weak_ptr<dispatcher_base>      weak_t;

    virtual                 ~dispatcher_base();

    virtual bool            get_commands(snap_string_list & commands) = 0;
    virtual bool            dispatch(snap::snap_communicator_message & msg) = 0;

private:
};







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

    // this version defines the protocol version, it should really rarely
    // change if ever
    static int const                                VERSION = 1;

    typedef int                                     priority_t;

    static priority_t const                         EVENT_MAX_PRIORITY = 255;

    class snap_dispatcher_support
    {
    public:
        virtual                     ~snap_dispatcher_support();

        void                        set_dispatcher(dispatcher_base::pointer_t d);
        dispatcher_base::pointer_t  get_dispatcher() const;
        bool                        dispatch_message(snap::snap_communicator_message & msg);

        // new callback
        virtual void                process_message(snap_communicator_message const & message);

    private:
        dispatcher_base::weak_t     f_dispatcher = dispatcher_base::weak_t();
    };

    class snap_connection
        : public std::enable_shared_from_this<snap_connection>
    {
    public:
        typedef std::shared_ptr<snap_connection>    pointer_t;
        typedef std::vector<pointer_t>              vector_t;

                                    snap_connection();

        // prevent copies
                                    snap_connection(snap_connection const & connection) = delete;
        snap_connection &           operator = (snap_connection const & connection) = delete;

        // virtual classes must have a virtual destructor
        virtual                     ~snap_connection();

        void                        remove_from_communicator();

        QString const &             get_name() const;
        void                        set_name(QString const & name);

        virtual bool                is_listener() const;
        virtual bool                is_signal() const;
        virtual bool                is_reader() const;
        virtual bool                is_writer() const;
        virtual int                 get_socket() const = 0;
        virtual bool                valid_socket() const;

        bool                        is_enabled() const;
        virtual void                set_enable(bool enabled);

        int                         get_priority() const;
        void                        set_priority(priority_t priority);
        static bool                 compare(pointer_t const & lhs, pointer_t const & rhs);

        uint16_t                    get_event_limit() const;
        void                        set_event_limit(uint16_t event_limit);
        uint16_t                    get_processing_time_limit() const;
        void                        set_processing_time_limit(int32_t processing_time_limit);

        int64_t                     get_timeout_delay() const;
        void                        set_timeout_delay(int64_t timeout_us);
        void                        calculate_next_tick();
        int64_t                     get_timeout_date() const;
        void                        set_timeout_date(int64_t date_us);
        int64_t                     get_timeout_timestamp() const;

        void                        non_blocking() const;
        void                        keep_alive() const;

        bool                        is_done() const;
        void                        mark_done();
        void                        mark_not_done();

        // callbacks
        virtual void                process_timeout();
        virtual void                process_signal();
        virtual void                process_read();
        virtual void                process_write();
        virtual void                process_empty_buffer();
        virtual void                process_accept();
        virtual void                process_error();
        virtual void                process_hup();
        virtual void                process_invalid();
        virtual void                connection_added();
        virtual void                connection_removed();

    private:
        friend snap_communicator;

        int64_t                     save_timeout_timestamp();
        int64_t                     get_saved_timeout_timestamp() const;

        QString                     f_name = QString();
        bool                        f_enabled = true;
        bool                        f_done = false;
        uint16_t                    f_event_limit = 5;                  // limit before giving other events a chance
        priority_t                  f_priority = 100;
        int64_t                     f_timeout_delay = -1;               // in microseconds
        int64_t                     f_timeout_next_date = -1;           // in microseconds, when we use the f_timeout_delay
        int64_t                     f_timeout_date = -1;                // in microseconds
        int64_t                     f_saved_timeout_stamp = -1;         // in microseconds
        int32_t                     f_processing_time_limit = 500000;   // in microseconds
        int                         f_fds_position = -1;
    };

    class connection_with_send_message
    {
    public:
        virtual                     ~connection_with_send_message();

        // new callback
        virtual bool                send_message(snap_communicator_message const & message, bool cache = false) = 0;

        virtual void                msg_help(snap_communicator_message & message);
        virtual void                msg_alive(snap_communicator_message & message);
        virtual void                msg_log(snap_communicator_message & message);
        virtual void                msg_quitting(snap_communicator_message & message);
        virtual void                msg_ready(snap_communicator_message & message);
        virtual void                msg_stop(snap_communicator_message & message);
        virtual void                msg_log_unknown(snap_communicator_message & message);
        virtual void                msg_reply_with_unknown(snap_communicator_message & message);

        virtual void                help(snap_string_list & commands);
        virtual void                ready(snap_communicator_message & message);
        virtual void                stop(bool quitting);
    };

    class snap_timer
        : public snap_connection
    {
    public:
        // timer is implemented using the timeout value on poll().
        // we could have another implementation that makes use of
        // the timerfd_create() function (in which case we'd be
        // limited to a date timeout, although an interval would
        // work too but require a little bit of work.)
        //
        typedef std::shared_ptr<snap_timer>    pointer_t;

                                    snap_timer(int64_t timeout_us);

        // snap_connection implementation
        virtual int                 get_socket() const override;
        virtual bool                valid_socket() const override;
    };

    class snap_signal
        : public snap_connection
    {
    public:
        typedef std::shared_ptr<snap_signal>    pointer_t;

                                    snap_signal(int posix_signal);
        virtual                     ~snap_signal() override;

        void                        close();
        void                        unblock_signal_on_destruction();

        // snap_connection implementation
        virtual bool                is_signal() const override;
        virtual int                 get_socket() const override;

        pid_t                       get_child_pid() const;

    private:
        friend snap_communicator;

        void                        process();

        int                         f_signal = 0;   // i.e. SIGHUP, SIGTERM...
        int                         f_socket = -1;  // output of signalfd()
        struct signalfd_siginfo     f_signal_info = signalfd_siginfo();
        bool                        f_unblock = false;
    };

    class snap_thread_done_signal
        : public snap_connection
    {
    public:
        typedef std::shared_ptr<snap_thread_done_signal>    pointer_t;

                                    snap_thread_done_signal();
        virtual                     ~snap_thread_done_signal() override;

        // snap_connection implementation
        virtual bool                is_reader() const override;
        virtual int                 get_socket() const override;
        virtual void                process_read() override;

        void                        thread_done();

    private:
        int                         f_pipe[2] = { -1, -1 };      // pipes
    };

    class snap_inter_thread_message_connection
        : public snap_connection
        , public connection_with_send_message
    {
    public:
        typedef std::shared_ptr<snap_inter_thread_message_connection>    pointer_t;

                                    snap_inter_thread_message_connection();
        virtual                     ~snap_inter_thread_message_connection() override;

        void                        close();

        // the child cannot have its own snap_communicator object, so...
        int                         poll(int timeout);

        // snap_connection implementation
        virtual bool                is_reader() const override;
        //virtual bool                is_writer() const override;
        virtual int                 get_socket() const override;
        virtual void                process_read() override;

        // connection_with_send_message
        virtual bool                send_message(snap_communicator_message const & message, bool cache = false) override;

        // new callback
        virtual void                process_message_a(snap_communicator_message const & message) = 0;
        virtual void                process_message_b(snap_communicator_message const & message) = 0;

    private:
        pid_t                       f_creator_id = -1;

        std::shared_ptr<int>        f_thread_a = std::shared_ptr<int>();
        snap::snap_thread::snap_fifo<snap_communicator_message>
                                    f_message_a = snap::snap_thread::snap_fifo<snap_communicator_message>();

        std::shared_ptr<int>        f_thread_b = std::shared_ptr<int>();
        snap::snap_thread::snap_fifo<snap_communicator_message>
                                    f_message_b = snap::snap_thread::snap_fifo<snap_communicator_message>();
    };

    class snap_pipe_connection
        : public snap_connection
    {
    public:
        typedef std::shared_ptr<snap_pipe_connection>    pointer_t;

                                    snap_pipe_connection();
        virtual                     ~snap_pipe_connection() override;

        void                        close();

        // snap_connection implementation
        virtual bool                is_reader() const override;
        virtual int                 get_socket() const override;

        // new callbacks
        virtual ssize_t             read(void * buf, size_t count);
        virtual ssize_t             write(void const * buf, size_t count);

    private:
        pid_t                       f_parent = -1;  // the process that created these pipes (read/write to 0 if getpid() == f_parent, read/write to 1 if getpid() != f_parent)
        int                         f_socket[2] = { -1, -1 };    // socket pair
    };

    class snap_pipe_buffer_connection
        : public snap_pipe_connection
    {
    public:
        typedef std::shared_ptr<snap_pipe_buffer_connection>    pointer_t;

        // snap::snap_communicator::snap_connection
        virtual bool                is_writer() const override;

        // snap::snap_communicator::snap_pipe_connection implementation
        virtual ssize_t             write(void const * data, size_t length) override;
        virtual void                process_read() override;
        virtual void                process_write() override;
        virtual void                process_hup() override;

        // new callback
        virtual void                process_line(QString const & line) = 0;

    private:
        std::string                 f_line = std::string(); // do NOT use QString because UTF-8 would break often... (since we may only receive part of messages)
        std::vector<char>           f_output = std::vector<char>();
        size_t                      f_position = 0;
    };

    class snap_pipe_message_connection
        : public snap_pipe_buffer_connection
        , public snap_dispatcher_support
        , public connection_with_send_message
    {
    public:
        typedef std::shared_ptr<snap_pipe_message_connection>    pointer_t;

        // connection_with_send_message
        virtual bool                send_message(snap_communicator_message const & message, bool cache = false) override;

        // snap_tcp_server_client_buffer_connection implementation
        virtual void                process_line(QString const & line) override;
    };

    class snap_file_changed
        : public snap_connection
    {
    public:
        typedef std::shared_ptr<snap_file_changed>      pointer_t;
        typedef uint32_t                                event_mask_t;

        static event_mask_t const   SNAP_FILE_CHANGED_EVENT_NO_EVENTS  = 0x0000;

        // bits added to watch_...() functions
        //
        static event_mask_t const   SNAP_FILE_CHANGED_EVENT_ATTRIBUTES = 0x0001;   // chmod, chown (timestamp, link count, user/group, etc.)
        static event_mask_t const   SNAP_FILE_CHANGED_EVENT_READ       = 0x0002;   // read, execve
        static event_mask_t const   SNAP_FILE_CHANGED_EVENT_WRITE      = 0x0004;   // write, truncate
        static event_mask_t const   SNAP_FILE_CHANGED_EVENT_CREATED    = 0x0008;   // open & O_CREAT, rename, mkdir, link, symlink, bind
        static event_mask_t const   SNAP_FILE_CHANGED_EVENT_DELETED    = 0x0010;   // unlink, rename
        static event_mask_t const   SNAP_FILE_CHANGED_EVENT_ACCESS     = 0x0020;   // open, close

        static event_mask_t const   SNAP_FILE_CHANGED_EVENT_IO         = SNAP_FILE_CHANGED_EVENT_READ
                                                                       | SNAP_FILE_CHANGED_EVENT_WRITE;

        static event_mask_t const   SNAP_FILE_CHANGED_EVENT_ALL        = SNAP_FILE_CHANGED_EVENT_ATTRIBUTES
                                                                       | SNAP_FILE_CHANGED_EVENT_IO
                                                                       | SNAP_FILE_CHANGED_EVENT_CREATED
                                                                       | SNAP_FILE_CHANGED_EVENT_DELETED
                                                                       | SNAP_FILE_CHANGED_EVENT_ACCESS;

        // flags added in event_t objects
        //
        static event_mask_t const   SNAP_FILE_CHANGED_EVENT_DIRECTORY  = 0x1000;   // object is a directory
        static event_mask_t const   SNAP_FILE_CHANGED_EVENT_GONE       = 0x2000;   // removed
        static event_mask_t const   SNAP_FILE_CHANGED_EVENT_UNMOUNTED  = 0x4000;   // unmounted

        class event_t
        {
        public:
                                    event_t(std::string const & watched_path
                                          , event_mask_t events
                                          , std::string const & filename);

            std::string const &     get_watched_path() const;
            event_mask_t            get_events() const;
            std::string const &     get_filename() const;

            bool                    operator < (event_t const & rhs) const;

        private:
            std::string             f_watched_path = std::string();
            event_mask_t            f_events       = 0;
            std::string             f_filename     = std::string();
        };

                                    snap_file_changed();
        virtual                     ~snap_file_changed() override;

        void                        watch_file(std::string const & watched_path, event_mask_t const events);
        void                        watch_symlink(std::string const & watched_path, event_mask_t const events);
        void                        watch_directory(std::string const & watched_path, event_mask_t const events);

        void                        stop_watch(std::string const & watched_path);

        // snap_connection implementation
        virtual bool                is_reader() const override;
        virtual int                 get_socket() const override;
        virtual void                set_enable(bool enabled);
        virtual void                process_read() override;

        // new callback
        virtual void                process_event(event_t const & watch_event) = 0;

    private:
        // TODO: RAII would be great with an impl and a counter...
        //       (i.e. we make copies at the moment.)
        struct watch_t
        {
            typedef std::map<int, watch_t>          map_t;

                                    watch_t();
                                    watch_t(std::string const & watched_path, event_mask_t events, uint32_t add_flags);

            void                    add_watch(int inotify);
            void                    merge_watch(int inotify, event_mask_t const events);
            void                    remove_watch(int inotify);

            std::string             f_watched_path = std::string();
            event_mask_t            f_events       = SNAP_FILE_CHANGED_EVENT_NO_EVENTS;
            uint32_t                f_mask         = 0;
            int                     f_watch        = -1;
        };

        bool                        merge_watch(std::string const & watched_path, event_mask_t const events);
        static uint32_t             events_to_mask(event_mask_t const events);
        static event_mask_t         mask_to_events(uint32_t const mask);

        int                         f_inotify = -1;
        watch_t::map_t              f_watches = watch_t::map_t();
    };

    class snap_fd_connection
        : public snap_connection
    {
    public:
        typedef std::shared_ptr<snap_fd_connection>         pointer_t;

        enum class mode_t
        {
            FD_MODE_READ,
            FD_MODE_WRITE,
            FD_MODE_RW
        };

                                    snap_fd_connection(int fd, mode_t mode);

        void                        close();
        void                        mark_closed();

        // snap_connection implementation
        virtual bool                is_reader() const override;
        virtual bool                is_writer() const override;
        virtual int                 get_socket() const override;

        // new callbacks
        virtual ssize_t             read(void * buf, size_t count);
        virtual ssize_t             write(void const * buf, size_t count);

    private:
        int                         f_fd = -1;
        mode_t                      f_mode = mode_t::FD_MODE_RW;
    };

    class snap_fd_buffer_connection
        : public snap_fd_connection
    {
    public:
        typedef std::shared_ptr<snap_fd_buffer_connection>    pointer_t;

                                    snap_fd_buffer_connection(int fd, mode_t mode);

        bool                        has_input() const;
        bool                        has_output() const;
        virtual bool                is_writer() const override;

        // snap_fd_connection implementation
        virtual ssize_t             write(void const * data, size_t const length) override;

        // snap_connection implementation
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

    class snap_tcp_client_connection
        : public snap_connection
        , public tcp_client_server::bio_client
    {
    public:
        typedef std::shared_ptr<snap_tcp_client_connection>    pointer_t;

                                    snap_tcp_client_connection(std::string const & addr, int port, mode_t mode = mode_t::MODE_PLAIN);

        QString const &             get_remote_address() const;

        // snap_connection implementation
        virtual bool                is_reader() const override;
        virtual int                 get_socket() const override;

        // new callbacks
        virtual ssize_t             read(void * buf, size_t count);
        virtual ssize_t             write(void const * buf, size_t count);

    private:
        QString const               f_remote_address = QString();
    };

    class snap_tcp_server_connection
        : public snap_connection
        , public tcp_client_server::bio_server
    {
    public:
        typedef std::shared_ptr<snap_tcp_server_connection>    pointer_t;

                                    snap_tcp_server_connection(std::string const & addr, int port, std::string const & certificate, std::string const & private_key, mode_t mode = mode_t::MODE_PLAIN, int max_connections = -1, bool reuse_addr = false);

        // snap_connection implementation
        virtual bool                is_listener() const override;
        virtual int                 get_socket() const override;
    };

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


} // namespace snap
// vim: ts=4 sw=4 et
