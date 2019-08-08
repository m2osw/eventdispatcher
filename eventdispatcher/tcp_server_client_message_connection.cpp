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
#include "eventdispatcher/tcp_server_client_message_connection.h"

#include "eventdispatcher/exception.h"


// snaplogger lib
//
#include "snaplogger/message.h"


// C lib
//
#include <arpa/inet.h>
#include <sys/socket.h>


// last include
//
#include <snapdev/poison.h>



namespace ed
{



/** \brief Initializes a client to read messages from a socket.
 *
 * This implementation creates a message in/out client.
 * This is the most useful client in our Snap! Communicator
 * as it directly sends and receives messages.
 *
 * \todo
 * Convert the socket address to string using libaddr.
 *
 * \param[in] client  The client representing the in/out socket.
 */
tcp_server_client_message_connection::tcp_server_client_message_connection(tcp_bio_client::pointer_t client)
    : tcp_server_client_buffer_connection(client)
{
    // TODO: somehow the port seems wrong (i.e. all connections return the same port)

    // make sure the socket is defined and well
    //
    int const socket(client->get_socket());
    if(socket < 0)
    {
        SNAP_LOG_ERROR
            << "called with a closed client connection.";
        throw std::runtime_error("tcp_server_client_message_connection() called with a closed client connection.");
    }

    struct sockaddr_storage address = sockaddr_storage();
    socklen_t length(sizeof(address));
    if(getpeername(socket, reinterpret_cast<struct sockaddr *>(&address), &length) != 0)
    {
        int const e(errno);
        SNAP_LOG_ERROR
            << "getpeername() failed retrieving IP address (errno: "
            << e
            << " -- "
            << strerror(e)
            << ").";
        throw std::runtime_error("getpeername() failed to retrieve IP address in tcp_server_client_message_connection()");
    }
    if(address.ss_family != AF_INET
    && address.ss_family != AF_INET6)
    {
        SNAP_LOG_ERROR
            << "address family ("
            << address.ss_family
            << ") returned by getpeername() is not understood, it is neither an IPv4 nor IPv6.";
        throw std::runtime_error("getpeername() returned an address which is not understood in tcp_server_client_message_connection()");
    }
    if(length < sizeof(address))
    {
        // reset the rest of the structure, just in case
        memset(reinterpret_cast<char *>(&address) + length, 0, sizeof(address) - length);
    }

    constexpr size_t max_length(std::max(INET_ADDRSTRLEN, INET6_ADDRSTRLEN) + 1);

// in release mode this should not be dynamic (although the syntax is so
// the warning would happen), but in debug it is likely an alloca()
//
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wvla"
    char buf[max_length];
#pragma GCC diagnostic pop

    char const * r(nullptr);

    if(address.ss_family == AF_INET)
    {
        r = inet_ntop(AF_INET, &reinterpret_cast<struct sockaddr_in const &>(address).sin_addr, buf, max_length);
    }
    else
    {
        r = inet_ntop(AF_INET6, &reinterpret_cast<struct sockaddr_in6 const &>(address).sin6_addr, buf, max_length);
    }

    if(r == nullptr)
    {
        int const e(errno);
        std::string err("inet_ntop() could not convert IP address (errno: ");
        err += std::to_string(e);
        err += " -- ";
        err += strerror(e);
        err += ").";
        SNAP_LOG_FATAL << err;
        throw event_dispatcher_runtime_error(err);
    }

    if(address.ss_family == AF_INET)
    {
        f_remote_address = buf;
        f_remote_address += ':';
        f_remote_address += std::to_string(static_cast<int>(ntohs(reinterpret_cast<sockaddr_in const &>(address).sin_port)));
    }
    else
    {
        f_remote_address = "[";
        f_remote_address += buf;
        f_remote_address += "]:";
        f_remote_address += std::to_string(static_cast<int>(ntohs(reinterpret_cast<sockaddr_in6 const &>(address).sin6_port)));
    }
}


/** \brief Process a line (string) just received.
 *
 * The function parses the line as a message (snap_communicator_message)
 * and then calls the process_message() function if the line was valid.
 *
 * \param[in] line  The line of text that was just read.
 */
void tcp_server_client_message_connection::process_line(std::string const & line)
{
    // empty lines should not occur, but just in case, just ignore
    if(line.empty())
    {
        return;
    }

    message msg;
    if(msg.from_message(line))
    {
        dispatch_message(msg);
    }
    else
    {
        // TODO: what to do here? This could because the version changed
        //       and the messages are not compatible anymore.
        //
        SNAP_LOG_ERROR
            << "process_line() was asked to process an invalid message ("
            << line
            << ")";
    }
}


/** \brief Send a message.
 *
 * This function sends a message to the client on the other side
 * of this connection.
 *
 * \exception event_dispatcher_runtime_error
 * This function throws this exception if the write() to the pipe
 * fails to write the entire message. This should only happen if
 * the pipe gets severed.
 *
 * \param[in] message  The message to be processed.
 * \param[in] cache  Whether to cache the message if there is no connection.
 *                   (Ignore because a client socket has to be there until
 *                   closed and then it can't be reopened by the server.)
 *
 * \return Always true, although if an error occurs the function throws.
 */
bool tcp_server_client_message_connection::send_message(message const & msg, bool cache)
{
    snap::NOTUSED(cache);

    // transform the message to a string and write to the socket
    // the writing is asynchronous so the message is saved in a cache
    // and transferred only later when the run() loop is hit again
    //
    std::string buf(msg.to_message());
    buf += '\n';
    return write(buf.c_str(), buf.length()) == static_cast<ssize_t>(buf.length());
}


/** \brief Retrieve the remote address information.
 *
 * This function can be used to retrieve the remove address and port
 * information as was specified on the constructor. These can be used
 * to find this specific connection at a later time or create another
 * connection.
 *
 * For example, you may get 192.168.2.17:4040.
 *
 * The function works even after the socket gets closed as we save
 * the remote address and port in a string just after the connection
 * was established.
 *
 * \warning
 * This function returns BOTH: the address and the port.
 *
 * \note
 * These parameters are the same as what was passed to the constructor,
 * only both will have been converted to numbers. So for example when
 * you used "localhost", here you get "::1" or "127.0.0.1" for the
 * address.
 *
 * \return The remote host address and connection port.
 */
std::string const & tcp_server_client_message_connection::get_remote_address() const
{
    return f_remote_address;
}



} // namespace ed
// vim: ts=4 sw=4 et
