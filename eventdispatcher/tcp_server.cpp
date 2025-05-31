// Copyright (c) 2012-2025  Made to Order Software Corp.  All Rights Reserved
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
// You should have received a copy of the GNU General Public License along
// with this program; if not, write to the Free Software Foundation, Inc.,
// 51 Franklin St, Fifth Floor, Boston, MA  02110-1301  USA

/** \file
 * \brief TCP server definition.
 *
 * Class used to handle events.
 */

// to get the POLLRDHUP definition
#ifndef _GNU_SOURCE
#define _GNU_SOURCE
#endif


// self
//
#include    "eventdispatcher/tcp_server.h"

#include    "eventdispatcher/exception.h"


// snaplogger
//
#include    <snaplogger/message.h>


// snapdev
//
#include    <snapdev/not_reached.h>


// C++
//
#include    <cstring>


// C
//
#include    <poll.h>


// last include
//
#include    <snapdev/poison.h>



namespace ed
{



/** \brief Initialize the server and start listening for connections.
 *
 * The server constructor creates a socket, binds it, and then listen to it.
 *
 * By default the server accepts a maximum of \p max_connections (set to
 * 0 or less to get the default tcp_server::MAX_CONNECTIONS) in its waiting
 * queue. If you use the server and expect a low connection rate, you may
 * want to reduce the count to 5. On the other hand, busy servers use larger
 * numbers. This value gets clamped to a minimum of 5 and a maximum of 1,000.
 *
 * Note that the maximum number of connections is actually limited to
 * /proc/sys/net/core/somaxconn connections. This number was around 128 
 * in 2016. In 2024, I see that it is now 4096. So the high limit of 1,000
 * prevents you to use the OS maximum.
 *
 * The address is made non-reusable (which is the default for TCP sockets).
 * It is possible to mark the server address as immediately reusable by
 * setting the \p reuse_addr to true.
 *
 * By default the server is marked as "keepalive". You can turn it off
 * using the keepalive() function with false.
 *
 * \exception runtime_error
 * This exception is raised if the socket cannot be created, bound to
 * the specified IP address and port, or listen() fails on the socket.
 *
 * \param[in] address  The address to listen on. It may be set to "0.0.0.0".
 * \param[in] max_connections  The number of connections to keep in the listen queue.
 * \param[in] reuse_addr  Whether to mark the socket with the SO_REUSEADDR flag.
 * \param[in] auto_close  Automatically close the client socket in accept and the destructor.
 */
tcp_server::tcp_server(
          addr::addr const & address
        , int max_connections
        , bool reuse_addr
        , bool auto_close)
    : f_address(address)
    , f_auto_close(auto_close)
{
    f_max_connections = std::clamp(max_connections <= 0 ? MAX_CONNECTIONS : max_connections, 5, 1000);

    f_socket = f_address.create_socket(
                  (reuse_addr
                        ? addr::addr::SOCKET_FLAG_REUSE
                        : 0));
    if(f_socket < 0)
    {
        int const e(errno);
        std::stringstream ss;
        ss << "addr::create_socket() failed to create a socket descriptor (errno: "
           << std::to_string(e)
           << " -- "
           << strerror(e)
           << ')';
        SNAP_LOG_ERROR
            << ss
            << SNAP_LOG_SEND;
        throw runtime_error(ss.str());
    }

    if(f_address.bind(f_socket) != 0)
    {
        close(f_socket);
        std::stringstream ss;
        ss << "could not bind the socket to \""
           << f_address.to_ipv4or6_string(addr::STRING_IP_ADDRESS | addr::STRING_IP_PORT)
           << '"';
        SNAP_LOG_ERROR
            << ss
            << SNAP_LOG_SEND;
        throw runtime_error(ss.str());
    }

    // start listening, we expect the caller to then call accept() to
    // acquire connections
    //
    if(listen(f_socket, f_max_connections) < 0)
    {
        close(f_socket);
        throw runtime_error(
                  "could not listen to the socket bound to \""
                + f_address.to_ipv4or6_string(addr::STRING_IP_ADDRESS | addr::STRING_IP_PORT)
                + "\"");
    }
}


/** \brief Clean up the server sockets.
 *
 * This function ensures that the server sockets get cleaned up.
 *
 * If the \p auto_close parameter was set to true in the constructor, then
 * the last accepted socket gets closed by this function.
 *
 * \note
 * DO NOT use the shutdown() call since we may end up forking and using
 * that connection in the child.
 */
tcp_server::~tcp_server()
{
    close(f_socket);
    if(f_auto_close && f_accepted_socket != -1)
    {
        close(f_accepted_socket);
    }
}


/** \brief Retrieve the socket descriptor.
 *
 * This function returns the socket descriptor. It can be used to
 * tweak things on the socket such as making it non-blocking or
 * directly accessing the data.
 *
 * \return The socket descriptor.
 */
int tcp_server::get_socket() const
{
    return f_socket;
}


/** \brief Retrieve the maximum number of connections.
 *
 * This function returns the maximum number of connections that can
 * be accepted by the socket. This was set by the constructor and
 * it cannot be changed later.
 *
 * \return The maximum number of incoming connections.
 */
int tcp_server::get_max_connections() const
{
    return f_max_connections;
}


/** \brief Retrieve the server IP address and port.
 *
 * This function returns the IP address used to bind the socket. This
 * is the address clients have to use to connect to the server unless
 * the address was set to all zeroes (0.0.0.0) in which case any user
 * can connect.
 *
 * The IP address cannot be changed.
 *
 * \return The server IP address.
 */
addr::addr tcp_server::get_address() const
{
    return f_address;
}


/** \brief Return the current status of the keepalive flag.
 *
 * This function returns the current status of the keepalive flag. This
 * flag is set to true by default (in the constructor.) It can be
 * changed with the set_keepalive() function.
 *
 * The flag is used to mark new connections with the SO_KEEPALIVE flag.
 * This is used whenever a service may take a little to long to answer
 * and avoid losing the TCP connection before the answer is sent to
 * the client.
 *
 * \return The current status of the keepalive flag.
 */
bool tcp_server::get_keepalive() const
{
    return f_keepalive;
}


/** \brief Set the keepalive flag.
 *
 * This function sets the keepalive flag to either true (i.e. mark connection
 * sockets with the SO_KEEPALIVE flag) or false. The default is true (as set
 * in the constructor,) because in most cases this is a feature people want.
 *
 * \param[in] yes  Whether to keep new connections alive even when no traffic
 * goes through.
 */
void tcp_server::set_keepalive(bool yes)
{
    f_keepalive = yes;
}


/** \brief Return the current status of the close_on_exec flag.
 *
 * This function returns the current status of the close_on_exec flag. This
 * flag is set to false by default (in the constructor.) It can be
 * changed with the set_close_on_exec() function.
 *
 * The flag is used to atomically mark new connections with the FD_CLOEXEC
 * flag. This prevents child processes from inheriting the socket (i.e. if
 * you use the system() function, for example, that process would inherit
 * your socket).
 *
 * \return The current status of the close_on_exec flag.
 */
bool tcp_server::get_close_on_exec() const
{
    return f_close_on_exec;
}


/** \brief Set the close_on_exec flag.
 *
 * This function sets the close_on_exec flag to either true (i.e. mark connection
 * sockets with the FD_CLOEXEC flag) or false. The default is false (as set
 * in the constructor,) because in our legacy code, the flag is not expected
 * to be set.
 *
 * \param[in] yes  Whether to close on exec() or not.
 */
void tcp_server::set_close_on_exec(bool yes)
{
    f_close_on_exec = yes;
}


/** \brief Accept a connection.
 *
 * A TCP server accepts incoming connections. This call is a blocking call.
 * If no connections are available on the line, then the call blocks until
 * a connection becomes available.
 *
 * To prevent being blocked by this call you can either check the status of
 * the file descriptor (use the get_socket() function to retrieve the
 * descriptor and use an appropriate wait with 0 as a timeout,) or transform
 * the socket in a non-blocking socket (not tested, though.)
 *
 * This TCP socket implementation is expected to be used in one of two ways:
 *
 * (1) the main server accepts connections and then fork()'s to handle the
 * transaction with the client, in that case we want to set the \p auto_close
 * parameter of the constructor to true so the accept() function automatically
 * closes the last accepted socket.
 *
 * (2) the main server keeps a set of connections and handles them alongside
 * the main server connection. Although there are limits to what you can do
 * in this way, it is very efficient, but this also means the accept() call
 * cannot close the last accepted socket since the rest of the software may
 * still be working on it.
 *
 * The function returns a client/server socket. This is the socket one can
 * use to communicate with the client that just connected to the server. This
 * descriptor can be written to or read from.
 *
 * This function is the one that applies the keepalive flag to the
 * newly accepted socket.
 *
 * \note
 * If you prevent SIGCHLD from stopping your code, you may want to allow it
 * when calling this function (that is, if you're interested in getting that
 * information immediately, otherwise it is cleaner to always block those
 * signals).
 * \note
 * For SIGCHLD, you may want to consider using the signal_child class which
 * takes care of the `waitpid()` and calls your callback at the time your
 * child process dies.
 *
 * \note
 * DO NOT use the shutdown() call since we may end up forking and using
 * that connection in the child.
 *
 * \note
 * If you want to have the FD_CLOEXEC flag set, make sure to call the
 * set_close_on_exec() function before you call the accept() function.
 *
 * \param[in] max_wait_ms  The maximum number of milliseconds to wait for
 *            a message. If set to -1 (the default), accept() will block
 *            indefinitely.
 *
 * \return A client socket descriptor, -1 if an error occurred, or
 *         -2 if it times out and max_wait is set.
 */
int tcp_server::accept(int const max_wait_ms)
{
    // auto-close?
    if(f_auto_close
    && f_accepted_socket != -1)
    {
        // if the close is interrupted, make sure we try again otherwise
        // we could lose that stream until next restart (this could happen
        // if you have SIGCHLD)
        //
        if(close(f_accepted_socket) == -1)
        {
            if(errno == EINTR)
            {
                close(f_accepted_socket);
            }
        }
    }
    f_accepted_socket = -1;

    if( max_wait_ms > -1 )
    {
        pollfd fd;
        fd.events = POLLIN | POLLPRI | POLLRDHUP;
        fd.fd = f_socket;
        int const retval(poll(&fd, 1, max_wait_ms));

// on newer system each input of select() must be a distinct fd_set...
//        fd_set s;
//
//        FD_ZERO(&s);
//#pragma GCC diagnostic push
//#pragma GCC diagnostic ignored "-Wold-style-cast"
//        FD_SET(f_socket, &s);
//#pragma GCC diagnostic pop
//
//        struct timeval timeout;
//        timeout.tv_sec = max_wait_ms / 1000;
//        timeout.tv_usec = (max_wait_ms % 1000) * 1000;
//        int const retval = select(f_socket + 1, &s, nullptr, &s, &timeout);

        if( retval == -1 )
        {
            // error
            //
            return -1;
        }
        else if( retval == 0 )
        {
            // timeout
            //
            return -2;
        }
    }

    // accept the next connection
    //
    struct sockaddr_in accepted_addr = {};
    socklen_t addr_len(sizeof(accepted_addr));
    f_accepted_socket = ::accept4(
                  f_socket
                , reinterpret_cast<struct sockaddr *>(&accepted_addr)
                , &addr_len
                , f_close_on_exec ? SOCK_CLOEXEC : 0);

    // mark the new connection with the SO_KEEPALIVE flag
    //
    if(f_accepted_socket != -1 && f_keepalive)
    {
        // if this fails, we ignore the error, but still log the event
        //
        int optval(1);
        socklen_t const optlen(sizeof(optval));
        if(setsockopt(f_accepted_socket, SOL_SOCKET, SO_KEEPALIVE, &optval, optlen) != 0)
        {
            SNAP_LOG_WARNING
                << "tcp_server::accept(): an error occurred trying to mark"
                   " accepted socket with SO_KEEPALIVE."
                << SNAP_LOG_SEND;
        }
    }

    return f_accepted_socket;
}


/** \brief Retrieve the last accepted socket descriptor.
 *
 * This function returns the last accepted socket descriptor as retrieved by
 * accept(). If accept() was never called or failed, then this returns -1.
 *
 * Note that it is possible that the socket was closed in between in which
 * case this value is going to be an invalid socket.
 *
 * \return The last accepted socket descriptor.
 */
int tcp_server::get_last_accepted_socket() const
{
    return f_accepted_socket;
}



} // namespace ed
// vim: ts=4 sw=4 et
