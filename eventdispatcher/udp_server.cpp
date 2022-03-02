// Copyright (c) 2012-2022  Made to Order Software Corp.  All Rights Reserved
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
 * \brief UDP server class implementation.
 *
 * This file is the implementation of the UDP server.
 *
 * A UDP server accepts UDP packets from any number of clients (contrary to
 * a TCP connection which is one on one). Without the proper implementation,
 * a UDP _connection_ is generally considered insecure. Also unless you
 * handle a form of acknowledgement, there is no way to know whether a
 * packet made it to the other end.
 */

// to get the POLLRDHUP definition
#ifndef _GNU_SOURCE
#define _GNU_SOURCE
#endif


// self
//
#include    "eventdispatcher/udp_server.h"

#include    "eventdispatcher/exception.h"


// snaplogger
//
#include    <snaplogger/message.h>


// C
//
#include    <arpa/inet.h>
#include    <poll.h>
#include    <string.h>


// last include
//
#include    <snapdev/poison.h>




namespace ed
{



/** \brief Initialize a UDP server object.
 *
 * This function initializes one UDP server object making it ready to
 * receive messages.
 *
 * The server address and port are specified in the constructor
 * as a libaddr `addr` object. It can represent an IPv4 or IPv6
 * address.
 *
 * Note that this function calls bind() to listen to the socket
 * at the specified address. To accept data on different UDP addresses
 * and ports, multiple UDP servers must be created.
 *
 * \note
 * The socket is open in this process. If you fork() or exec() then the
 * socket will be closed by the operating system.
 *
 * \warning
 * Remember that the multicast feature under Linux is shared by all
 * processes running on that server. Any one process can listen for
 * any and all multicast messages from any other process. Our
 * implementation limits the multicast from a specific IP. However.
 * other processes can also receive your packets and there is nothing
 * you can do to prevent that. Multicast is only supported with IPv4
 * addresses. We currently do not allow the default address as the
 * multicast address.
 *
 * \exception event_dispatcher_runtime_error
 * The event_dispatcher_runtime_error exception is raised when the socket
 * cannot be opened.
 *
 * \param[in] address  The address we receive on.
 * \param[in] multicast_address  A multicast address.
 */
udp_server::udp_server(
          addr::addr const & address
        , addr::addr const & multicast_address)
    : udp_base(address)
{
    int r(0);

    if(!multicast_address.is_default())
    {
        // in multicast we have to bind to the multicast IP (or IN_ANYADDR
        // which right now we do not support)
        //
        if(!multicast_address.is_ipv4()
        || !f_address.is_ipv4())
        {
            std::stringstream ss;
            ss << "the UDP multicast implementation only supports IPv4 at the moment; multicast: \""
               << multicast_address.to_ipv4or6_string(addr::addr::string_ip_t::STRING_IP_PORT)
               << "\", address: \""
               << f_address.to_ipv4or6_string(addr::addr::string_ip_t::STRING_IP_PORT)
               << "\".";
            SNAP_LOG_FATAL
                << ss
                << SNAP_LOG_SEND;
            throw event_dispatcher_runtime_error(ss.str());
        }

        r = multicast_address.bind(get_socket());
        if(r != 0)
        {
            int const e(errno);
            std::stringstream ss;
            ss << "the multicast address bind() function failed with errno: "
               << e
               << " ("
               << strerror(e)
               << "); address "
               << multicast_address.to_ipv4or6_string(addr::addr::string_ip_t::STRING_IP_PORT);
            SNAP_LOG_FATAL
                << ss
                << SNAP_LOG_SEND;
            throw event_dispatcher_runtime_error(ss.str());
        }
    }
    else
    {
        r = f_address.bind(get_socket());
        if(r != 0)
        {
            int const e(errno);

            std::stringstream ss;
            ss << "the bind() function failed with errno: "
               << e
               << " ("
               << strerror(e)
               << "); address "
               << f_address.to_ipv4or6_string(addr::addr::string_ip_t::STRING_IP_PORT);
            SNAP_LOG_FATAL
                << ss
                << SNAP_LOG_SEND;
            throw event_dispatcher_runtime_error(ss.str());
        }
    }

    // are we creating a server to listen to multicast packets?
    //
    if(!multicast_address.is_default())
    {
        sockaddr_in m = {};
        sockaddr_in a = {};

        multicast_address.get_ipv4(m);
        f_address.get_ipv4(a);

#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wpedantic"
        ip_mreqn mreq = {
            .imr_multiaddr = m.sin_addr,
            .imr_address = a.sin_addr,
            .imr_ifindex = 0,                   // no specific interface
        };
#pragma GCC diagnostic pop

        r = setsockopt(f_socket.get(), IPPROTO_IP, IP_ADD_MEMBERSHIP, &mreq, sizeof(mreq));
        if(r < 0)
        {
            int const e(errno);
            throw event_dispatcher_runtime_error(
                      "IP_ADD_MEMBERSHIP failed for: \""
                    + f_address.to_ipv4or6_string(addr::addr::string_ip_t::STRING_IP_PORT)
                    + "\" or \""
                    + multicast_address.to_ipv4or6_string(addr::addr::string_ip_t::STRING_IP_PORT)
                    + "\", errno: "
                    + std::to_string(e) + ", " + strerror(e));
        }

        // setup the multicast to 0 so we don't receive other's
        // messages; apparently the default would be 1
        //
        int multicast_all(0);
        r = setsockopt(f_socket.get(), IPPROTO_IP, IP_MULTICAST_ALL, &multicast_all, sizeof(multicast_all));
        if(r < 0)
        {
            // things should still work if the IP_MULTICAST_ALL is not
            // set as we want it
            //
            int const e(errno);
            SNAP_LOG_WARNING
                    << "could not set IP_MULTICAST_ALL to zero, e = "
                    << e
                    << " -- "
                    << strerror(e)
                    << SNAP_LOG_SEND;
        }
    }
}


/** \brief Clean up the UDP server.
 *
 * This function frees the address info structures and close the socket.
 */
udp_server::~udp_server()
{
}


/** \brief Wait on a message.
 *
 * This function waits until a message is received on this UDP server.
 * There are no means to return from this function except by receiving
 * a message. Remember that UDP does not have a connect state so whether
 * another process quits does not change the status of this UDP server
 * and thus it continues to wait forever.
 *
 * Note that you may change the type of socket by making it non-blocking
 * (use the get_socket() to retrieve the socket identifier) in which
 * case this function will not block if no message is available. Instead
 * it returns immediately.
 *
 * \param[in] msg  The buffer where the message is saved.
 * \param[in] max_size  The maximum size the message (i.e. size of the \p msg buffer.)
 *
 * \return The number of bytes read or -1 if an error occurs.
 */
int udp_server::recv(char * msg, size_t max_size)
{
    return static_cast<int>(::recv(f_socket.get(), msg, max_size, 0));
}


/** \brief Wait for data to come in.
 *
 * This function waits for a given amount of time for data to come in. If
 * no data comes in after max_wait_ms, the function returns with -1 and
 * errno set to EAGAIN.
 *
 * The socket is expected to be a blocking socket (the default,) although
 * it is possible to setup the socket as non-blocking if necessary for
 * some other reason.
 *
 * This function blocks for a maximum amount of time as defined by
 * max_wait_ms. It may return sooner with an error or a message.
 *
 * \param[in] msg  The buffer where the message will be saved.
 * \param[in] max_size  The size of the \p msg buffer in bytes.
 * \param[in] max_wait_ms  The maximum number of milliseconds to wait for a message.
 *
 * \return -1 if an error occurs or the function timed out, the number of bytes received otherwise.
 */
int udp_server::timed_recv(char * msg, size_t const max_size, int const max_wait_ms)
{
    pollfd fd;
    fd.events = POLLIN | POLLPRI | POLLRDHUP;
    fd.fd = f_socket.get();
    int const retval(poll(&fd, 1, max_wait_ms));

//    fd_set s;
//    FD_ZERO(&s);
//#pragma GCC diagnostic push
//#pragma GCC diagnostic ignored "-Wold-style-cast"
//    FD_SET(f_socket.get(), &s);
//#pragma GCC diagnostic pop
//    struct timeval timeout;
//    timeout.tv_sec = max_wait_ms / 1000;
//    timeout.tv_usec = (max_wait_ms % 1000) * 1000;
//    int const retval(select(f_socket.get() + 1, &s, nullptr, &s, &timeout));
    if(retval == -1)
    {
        // poll() sets errno accordingly
        return -1;
    }
    if(retval > 0)
    {
        // our socket has data
        return static_cast<int>(::recv(f_socket.get(), msg, max_size, 0));
    }

    // our socket has no data
    errno = EAGAIN;
    return -1;
}


/** \brief Wait for data to come in, but return a std::string.
 *
 * This function waits for a given amount of time for data to come in. If
 * no data comes in after max_wait_ms, the function returns with -1 and
 * errno set to EAGAIN.
 *
 * The socket is expected to be a blocking socket (the default,) although
 * it is possible to setup the socket as non-blocking if necessary for
 * some other reason.
 *
 * This function blocks for a maximum amount of time as defined by
 * max_wait_ms. It may return sooner with an error or a message.
 *
 * \param[in] bufsize  The maximum size of the returned string in bytes.
 * \param[in] max_wait_ms  The maximum number of milliseconds to wait for a message.
 *
 * \return The received string or an empty string if not data received or error.
 *
 * \sa timed_recv()
 */
std::string udp_server::timed_recv(int const bufsize, int const max_wait_ms)
{
    std::vector<char> buf;
    buf.resize(bufsize + 1, '\0'); // +1 for ending \0
    int const r(timed_recv(&buf[0], bufsize, max_wait_ms));
    if(r <= -1)
    {
        // Timed out, so return empty string.
        // TBD: could std::string() smash errno?
        //
        return std::string();
    }

    // Resize the buffer, then convert to std string
    //
    buf.resize(r + 1, '\0');

    std::string word;
    word.resize(r);
    std::copy(buf.begin(), buf.end(), word.begin());

    return word;
}




} // namespace ed
// vim: ts=4 sw=4 et
