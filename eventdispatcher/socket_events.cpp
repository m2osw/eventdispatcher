// Copyright (c) 2012-2021  Made to Order Software Corp.  All Rights Reserved
//
// https://snapwebsites.org/project/eventdispatcher
// contact@m2osw.com
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

/** \file
 * \brief Implementation of the Snap Communicator class.
 *
 * This class wraps the C poll() interface in a C++ object with many types
 * of objects:
 *
 * \li Server Connections; for software that want to offer a port to
 *     which clients can connect to; the server will call accept()
 *     once a new client connection is ready; this results in a
 *     Server/Client connection object
 * \li Client Connections; for software that want to connect to
 *     a server; these expect the IP address and port to connect to
 * \li Server/Client Connections; for the server when it accepts a new
 *     connection; in this case the server gets a socket from accept()
 *     and creates one of these objects to handle the connection
 *
 * Using the poll() function is the easiest and allows us to listen
 * on pretty much any number of sockets (on my server it is limited
 * at 16,768 and frankly over 1,000 we probably will start to have
 * real slowness issues on small VPN servers.)
 */


// self
//
#include    "eventdispatcher/socket_events.h"

#include    "eventdispatcher/communicator.h"
#include    "eventdispatcher/exception.h"
#include    "eventdispatcher/timer.h"

// #include    "eventdispatcher/tcp_bio_client.h"
// #include    "eventdispatcher/tcp_server_client_message_connection.h"
// #include    "eventdispatcher/thread_done_signal.h"


// snaplogger lib
//
#include    <snaplogger/message.h>


// snapdev lib
//
#include    <snapdev/raii_generic_deleter.h>


// cppthread lib
//
// #include    <cppthread/exception.h>
#include    <cppthread/guard.h>
#include    <cppthread/mutex.h>
// #include    <cppthread/thread.h>


// libaddr lib
//
#include    <libaddr/addr_parser.h>


// C++ lib
//
#include    <algorithm>
#include    <deque>


// C lib
//
// #include    <sys/socket.h>
#include    <linux/inet_diag.h>
#include    <linux/netlink.h>
#include    <linux/sock_diag.h>


// last include
//
#include    <snapdev/poison.h>



namespace ed
{



namespace
{



#pragma GCC diagnostic push
//#pragma GCC diagnostic ignored "-Weff-c++"
struct socket_evt
{
    typedef std::shared_ptr<socket_evt>     pointer_t;
    typedef std::deque<pointer_t>           deque_t;

    bool                        f_listening = false;
    socket_events *             f_socket_events = nullptr;
};
#pragma GCC diagnostic pop



class socket_listener
    : public timer
{
public:
    typedef std::shared_ptr<socket_listener>        pointer_t;

    static constexpr std::size_t const  RECEIVE_BUFFER_SIZE = 1'000 * (sizeof(nlmsghdr) + sizeof(inet_diag_msg));
    static constexpr int                TCP_LISTEN_STATE = 10;

                                socket_listener(cppthread::mutex & socket_mutex);
    virtual                     ~socket_listener();

    static pointer_t            instance();

    void                        add_socket_events(socket_events * evts);
    void                        lost_connection(socket_events * evts);
    void                        remove_socket_events(socket_events * evts);

    // connection implementation
    virtual bool                is_reader() const override;
    virtual bool                is_writer() const override;
    virtual int                 get_socket() const override;
    virtual void                process_timeout() override;
    virtual void                process_read() override;
    virtual void                process_write() override;
    virtual void                process_error() override;
    virtual void                process_hup() override;
    virtual void                process_invalid() override;

private:
    cppthread::mutex &          f_socket_mutex;
    snap::raii_fd_t             f_netlink_socket = snap::raii_fd_t();
    socket_evt::deque_t         f_socket_events = socket_evt::deque_t();
};


socket_listener::pointer_t      g_socket_listener = socket_listener::pointer_t();


socket_listener::socket_listener(cppthread::mutex & socket_mutex)
    : timer(1'000'000)
    , f_socket_mutex(socket_mutex)
    , f_netlink_socket(socket(
                  AF_NETLINK
                , SOCK_RAW | SOCK_CLOEXEC | SOCK_NONBLOCK
                , NETLINK_SOCK_DIAG))
{
    if(f_netlink_socket < 0)
    {
        throw event_dispatcher_runtime_error("opening SOCK_RAW failed in socket_listener.");
    }

    // increase our changes to avoid memory issues
    //
    int const sndbuf(32 * 1'024);
	if(setsockopt(
              f_netlink_socket.get()
            , SOL_SOCKET
            , SO_SNDBUF
            , &sndbuf
            , sizeof(sndbuf)) != 0)
    {
        SNAP_LOG_WARNING
            << "the SO_SNDBUF failed against the NETLINK socket."
            << SNAP_LOG_SEND;
	}

    // enough space to support up to about 1,000 messages max.
    //
    int const rcvbuf(RECEIVE_BUFFER_SIZE);
	if(setsockopt(
              f_netlink_socket.get()
            , SOL_SOCKET
            , SO_RCVBUF
            , &rcvbuf
            , sizeof(rcvbuf)) != 0)
    {
        SNAP_LOG_WARNING
            << "the SO_RCVBUF failed against the NETLINK socket."
            << SNAP_LOG_SEND;
	}

#if 0
    struct sockaddr_nl addr = {};
    addr.nl_family = AF_NETLINK;
    addr.nl_pid = getpid();

    // You can find the "groups" flags in Linux source:
    // (change the version as required with your current version)
    //
    //     "/usr/src/linux-headers-4.15.0-147/include/net/tcp_states.h
    //
    addr.nl_groups = TCPF_LISTEN;

    if(bind(d, reinterpret_cast<struct sockaddr *>(&addr), sizeof(addr)) != 0)
    {
        throw event_dispatcher_runtime_error("could not bind() the SOCK_RAW of socket_listener.");
    }
#endif
}


socket_listener::~socket_listener()
{
}


socket_listener::pointer_t socket_listener::instance()
{
    static cppthread::mutex g_mutex;

    cppthread::guard g(g_mutex);

    if(g_socket_listener == nullptr)
    {
        g_socket_listener.reset(new socket_listener(g_mutex));

        communicator::instance()->add_connection(g_socket_listener);
    }

    return g_socket_listener;
}


void socket_listener::add_socket_events(socket_events * evts)
{
    if(!evts->get_addr().is_ipv4())
    {
        throw event_dispatcher_invalid_parameter("at this time, the socket listener is limited to IPv4 addresses.");
    }

    cppthread::guard g(f_socket_mutex);

    socket_evt::pointer_t evt(std::make_shared<socket_evt>());
    evt->f_socket_events = evts;

    f_socket_events.push_back(evt);

    set_enable(true);
}


void socket_listener::lost_connection(socket_events * evts)
{
    cppthread::guard g(f_socket_mutex);

    auto it(std::find_if(
              f_socket_events.begin()
            , f_socket_events.end()
            , [&evts](socket_evt::pointer_t evt)
            {
                return evt->f_socket_events == evts;
            }));
    if(it != f_socket_events.end())
    {
        // if we lost the connection we assume that the other end is not
        // listening
        //
        (*it)->f_listening = false;
    }

    set_enable(true);
}


void socket_listener::remove_socket_events(socket_events * evts)
{
    cppthread::guard g(f_socket_mutex);

    auto it(std::find_if(
              f_socket_events.begin()
            , f_socket_events.end()
            , [&evts](socket_evt::pointer_t evt)
            {
                return evt->f_socket_events == evts;
            }));
    if(it != f_socket_events.end())
    {
        f_socket_events.erase(it);

        if(f_socket_events.empty())
        {
            communicator::instance()->remove_connection(g_socket_listener);

            g_socket_listener.reset();
        }
    }
}


bool socket_listener::is_reader() const
{
    cppthread::guard g(f_socket_mutex);

    return true;
}


bool socket_listener::is_writer() const
{
    cppthread::guard g(f_socket_mutex);

    for(auto it : f_socket_events)
    {
        if(!it->f_listening)
        {
            return true;
        }
    }

    return false;
}


int socket_listener::get_socket() const
{
    return f_netlink_socket.get();
}


void socket_listener::process_timeout()
{
    cppthread::guard g(f_socket_mutex);

    for(auto it : f_socket_events)
    {
        if(!it->f_listening)
        {
            return;
        }
    }

    // nothing to check, go to sleep
    //
    set_enable(false);
}


void socket_listener::process_read()
{
    sockaddr_nl nladdr = {};

    nladdr.nl_family = AF_NETLINK;

    char buf[RECEIVE_BUFFER_SIZE * 2];
    struct iovec vec = {};
    vec.iov_base = buf;
    vec.iov_len = sizeof(buf);

    for(;;)
    {
        struct msghdr msg = {};

        msg.msg_name = &nladdr;
        msg.msg_namelen = sizeof(nladdr);
        msg.msg_iov = &vec;
        msg.msg_iovlen = 1;

        ssize_t size(recvmsg(f_netlink_socket.get(), &msg, 0));
        if(size < 0)
        {
            int const e(errno);
            SNAP_LOG_ERROR
                << "recvmsg() returned with an error: "
                << e
                << " ("
                << strerror(e)
                << ")."
                << SNAP_LOG_SEND;
            return;
        }

        if(size == 0)
        {
            // found end of message stream for now
            //
            return;
        }

        for(nlmsghdr * h(reinterpret_cast<nlmsghdr *>(buf));
            NLMSG_OK(h, size);
            h = NLMSG_NEXT(h, size))
        {
            switch(h->nlmsg_type)
            {
            case NLMSG_DONE:
                return;

            case NLMSG_ERROR:
                if(h->nlmsg_len < NLMSG_LENGTH(sizeof(nlmsgerr)))
                {
                    SNAP_LOG_ERROR
                        << "unknown NLMSG_ERROR received (data buffer too small)."
                        << SNAP_LOG_SEND;
                }
                else
                {
                    nlmsgerr const * err(reinterpret_cast<nlmsgerr const *>(NLMSG_DATA(h)));
                    int const e(-err->error);
                    if(e != ENOENT)
                    {
                        SNAP_LOG_ERROR
                            << "NETLINK error: "
                            << e
                            << " ("
                            << strerror(e)
                            << ")."
                            << SNAP_LOG_SEND;
                    }
                }
                break;

            case SOCK_DIAG_BY_FAMILY:
                if(h->nlmsg_len < NLMSG_LENGTH(sizeof(inet_diag_msg)))
                {
                    SNAP_LOG_WARNING
                        << "NETLINK length (h->nlmsg_len = "
                        << h->nlmsg_len
                        << ", expected at least "
                        << sizeof(inet_diag_msg)
                        << ") too small for a SOCK_DIAG_BY_FAMILY object."
                        << SNAP_LOG_SEND;
                    return;
                }
                else
                {
                    inet_diag_msg const * diag(reinterpret_cast<inet_diag_msg const *>(NLMSG_DATA(h)));
                    if(diag->idiag_state == TCP_LISTEN_STATE)
                    {
                        // got a listen(), look for which connection this is
                        // and mark it as valid (open/listening)
                        //
                        for(auto it : f_socket_events)
                        {
                            if(!it->f_listening)
                            {
                                addr::addr a(it->f_socket_events->get_addr());
                                if(a.get_port() == diag->id.idiag_sport)
                                {
                                    sockaddr_in in = {};
                                    a.get_ipv4(in);
                                    if(in.sin_addr.s_addr == diag->id.idiag_src[0])
                                    {
                                        // got it!
                                        //
                                        it->f_socket_events->process_listening();
                                        it->f_listening = true;

                                        // TBD: if we add two connections with the same IP:port combo,
                                        //      we get two separate socket_events but I do not know
                                        //      whether we'll get one or two replies... so at this
                                        //      time do not break this loop
                                        //break;
                                    }
                                }
                            }
                        }
                    }
                }
                break;

            default:
                SNAP_LOG_WARNING
                    << "unexpected message type (h->nlmsg_type) "
                    << h->nlmsg_type
                    << SNAP_LOG_SEND;
                break;

            }
        }
    }
}


void socket_listener::process_write()
{
    cppthread::guard g(f_socket_mutex);

    // count the number of requests we have to send
    //
    int const count(std::count_if(
              f_socket_events.begin()
            , f_socket_events.end()
            , [](auto const & evt)
            {
                return !evt->f_listening;
            }));

    struct nl_request
    {
        struct nlmsghdr         f_nlh;
        struct inet_diag_req_v2 f_inet;
    };

    // preallocation means that the pointers won't change
    // which is important here
    //
    std::vector<nl_request> req(count);
    std::vector<iovec> vec(count);

    int idx(0);
    for(auto it : f_socket_events)
    {
        if(!it->f_listening)
        {
            addr::addr const & a(it->f_socket_events->get_addr());

            sockaddr_in in = {};
            a.get_ipv4(in);

            req[idx].f_nlh.nlmsg_len = sizeof(nl_request);
            req[idx].f_nlh.nlmsg_type = SOCK_DIAG_BY_FAMILY;
            req[idx].f_nlh.nlmsg_flags = NLM_F_REQUEST;
            req[idx].f_inet.sdiag_family = AF_INET;
            req[idx].f_inet.sdiag_protocol = IPPROTO_TCP;
            req[idx].f_inet.idiag_ext = 0;
            req[idx].f_inet.pad = 0;
            req[idx].f_inet.idiag_states = 0;
            req[idx].f_inet.id.idiag_sport = in.sin_port;
            req[idx].f_inet.id.idiag_dport = 0;
            req[idx].f_inet.id.idiag_src[0] = in.sin_addr.s_addr;
            req[idx].f_inet.id.idiag_dst[0] = 0;
            req[idx].f_inet.id.idiag_if = 0;
            req[idx].f_inet.id.idiag_cookie[0] = INET_DIAG_NOCOOKIE;
            req[idx].f_inet.id.idiag_cookie[1] = INET_DIAG_NOCOOKIE;

            vec[idx].iov_base = &req[idx];
            vec[idx].iov_len = sizeof(nl_request);

            ++idx;
        }
    }
    if(idx != count)
    {
        throw event_dispatcher_implementation_error(
                  "somehow the number of requests counted ("
                + std::to_string(count)
                + ") did not match the number of request created ("
                + std::to_string(idx)
                + ").");
    }

    sockaddr_nl nladdr = {};

    nladdr.nl_family = AF_NETLINK;

    msghdr msg = {};

    msg.msg_name = &nladdr;
    msg.msg_namelen = sizeof(nladdr);
    msg.msg_iov = vec.data();
    msg.msg_iovlen = count;

    int const r(sendmsg(f_netlink_socket.get(), &msg, 0));
    if(r < 0)
    {
        process_error();
        return;
    }
}


void socket_listener::process_error()
{
    cppthread::guard g(f_socket_mutex);

    socket_evt::deque_t evts(f_socket_events);
    for(auto it : evts)
    {
        if(!it->f_listening)
        {
            it->f_socket_events->process_error();
        }
    }
}


void socket_listener::process_hup()
{
    cppthread::guard g(f_socket_mutex);

    for(auto it : f_socket_events)
    {
        if(!it->f_listening)
        {
            it->f_socket_events->process_hup();
        }
    }
}


void socket_listener::process_invalid()
{
    cppthread::guard g(f_socket_mutex);

    for(auto it : f_socket_events)
    {
        if(!it->f_listening)
        {
            it->f_socket_events->process_invalid();
        }
    }
}






}
// no name detail



/** \brief Initializes this socket event object.
 *
 * This function initializes the socket_events with the specified address.
 * The address will be used to listen for a `listen()` call from any process
 * on this system.
 *
 * \warning
 * This only works for local services. Services that run on a remote computer
 * must attempt to connect and fail on the connect until the remote service
 * is available.
 *
 * \param[in] a  The address and port to poll for a `listen()`.
 */
socket_events::socket_events(addr::addr const & a)
    : f_addr(a)
{
    socket_listener::instance()->add_socket_events(this);
}


/** \brief Initializes this socket events object.
 *
 * Initialize a socket_events object to listen to the specified address
 * and port for a new connection (i.e. for a process to call `listen()`
 * on that address:port combo).
 *
 * If this is the first socket_events created, then a new socket listener
 * is also created.
 *
 * Then this socket_events object gets added to that socket listener.
 *
 * \param[in] address  The address of the port to wait on.
 * \param[in] port  The port to poll for a `listen()`.
 */
socket_events::socket_events(
            std::string const & address
          , int port)
    : f_addr(addr::string_to_addr(
          address
        , "127.0.0.1"
        , port
        , "tcp"))     // we really only support TCP at the moment
{
    socket_listener::instance()->add_socket_events(this);
}


/** \brief Destroy instance.
 *
 * This function cleans up this socket event instance. This means the socket
 * address and port are removed from the actual socket listener and if that
 * was the last socket_events object, the socket listener is also destroyed.
 */
socket_events::~socket_events()
{
    socket_listener::instance()->remove_socket_events(this);
}


/** \brief This higher level connection has no socket.
 *
 * This function always returns -1 as there is no socket in this connection.
 *
 * \note
 * The one socket is found in the socket_listener which gets created with
 * the first socket_events (and destroyed with the last deleted
 * socket_events).
 *
 * \return Always -1.
 */
int socket_events::get_socket() const
{
    return -1;
}


/** \brief This function gives you access to the address of this connection.
 *
 * The loops used to create the NETLINK_SOCK_DIAG requests makes use of this
 * function to filter on this specific IP address and port.
 *
 * \return The address we are listening on.
 */
addr::addr const & socket_events::get_addr() const
{
    return f_addr;
}


/** \brief This function is called whenever you lose a connection.
 *
 * In most cases, you lose a connection because the service breaks (crashes
 * or was restarted) so you need to poll for a `listen()` again. This
 * function lets the socket_listener internal object know that you expect
 * a call to the process_listening() once the service is available again.
 */
void socket_events::lost_connection()
{
    socket_listener::instance()->lost_connection(this);
}



} // namespace ed
// vim: ts=4 sw=4 et
