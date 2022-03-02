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
// You should have received a copy of the GNU General Public License
// along with this program; if not, write to the Free Software
// Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA  02110-1301  USA

/** \file
 * \brief Event dispatch class.
 *
 * Class used to handle events.
 */


// self
//
#include    "eventdispatcher/tcp_client.h"

#include    "eventdispatcher/exception.h"
#include    "eventdispatcher/utils.h"


// snaplogger
//
#include    <snaplogger/message.h>


// C
//
#include    <netdb.h>
#include    <arpa/inet.h>
#include    <string.h>


// last include
//
#include    <snapdev/poison.h>



namespace ed
{



/** \class tcp_client
 * \brief Create a client socket and connect to a server.
 *
 * This class is a client socket implementation used to connect to a server.
 * The server is expected to be running at the time the client is created
 * otherwise it fails connecting.
 *
 * This class is not appropriate to connect to a server that may come and go
 * over time.
 */

/** \brief Construct a tcp_client object.
 *
 * The tcp_client constructor initializes a TCP client object by connecting
 * to the specified server. The server is defined by the \p address object.
 *
 * \exception event_dispatcher_invalid_parameter
 * This exception is raised if the \p port parameter is out of range or the
 * IP address is an empty string or otherwise an invalid address.
 *
 * \exception event_dispatcher_runtime_error
 * This exception is raised if the client cannot create the socket or it
 * cannot connect to the server.
 *
 * \param[in] address  The address of the server to connect to. It must be valid.
 */
tcp_client::tcp_client(addr::addr const & address)
    : f_address(address)
{
    if(f_address.is_default())
    {
        throw event_dispatcher_invalid_parameter("the default address is not valid for a client socket");
    }
    if(f_address.get_protocol() != IPPROTO_TCP)
    {
        throw event_dispatcher_invalid_parameter("the address presents a protocol other than the expected TCP");
    }

    f_socket.reset(address.create_socket(0));
    if(f_socket < 0)
    {
        int const e(errno);
        SNAP_LOG_FATAL
            << "socket() failed to create a socket descriptor (errno: "
            << e
            << " -- "
            << strerror(e)
            << ")"
            << SNAP_LOG_SEND;
        throw event_dispatcher_runtime_error("could not create socket for client");
    }

    if(f_address.connect(f_socket.get()) != 0)
    {
        int const e(errno);
        std::stringstream ss;
        ss << "tcp_client() -- failed to connect() socket (errno: "
           << e
           << " -- "
           << strerror(e)
           << ")";
        SNAP_LOG_FATAL
            << ss
            << SNAP_LOG_SEND;
        throw event_dispatcher_runtime_error(ss.str());
    }
}

/** \brief Clean up the TCP client object.
 *
 * This function cleans up the TCP client object by closing the attached socket.
 *
 * \note
 * DO NOT use the shutdown() call since we may end up forking and using
 * that connection in the child.
 */
tcp_client::~tcp_client()
{
}

/** \brief Get the socket descriptor.
 *
 * This function returns the TCP client socket descriptor. This can be
 * used to change the descriptor behavior (i.e. make it non-blocking for
 * example.)
 *
 * \return The socket descriptor.
 */
int tcp_client::get_socket() const
{
    return f_socket.get();
}

/** \brief Get the TCP client port.
 *
 * This function returns the port used when creating the TCP client.
 * Note that this is the port the server is listening to and not the port
 * the TCP client is currently connected to.
 *
 * \return The TCP client port.
 */
int tcp_client::get_port() const
{
    return f_address.get_port();
}

/** \brief Get the TCP server address.
 *
 * This function returns the address used when creating the TCP address.
 * Note that this is the address of the server where the client is connected
 * and not the address where the client is running (although it may be the
 * same.)
 *
 * Use the get_client_addr() function to retrieve the client's TCP address.
 *
 * \return The TCP server address.
 */
std::string tcp_client::get_addr() const
{
    return f_address.to_ipv4or6_string(addr::addr::string_ip_t::STRING_IP_ONLY);
}

/** \brief Get the TCP server address.
 *
 * This function returns a copy of the address as specified in the contructor.
 *
 * \return The TCP server address/
 */
addr::addr tcp_client::get_address() const
{
    return f_address;
}

/** \brief Get the TCP client port.
 *
 * This function retrieve the port of the client (used on your computer).
 * This is retrieved from the socket using the getsockname() function.
 *
 * \return The port or -1 if it cannot be determined.
 *
 * \sa get_client_addr()
 * \sa get_client_address()
 */
int tcp_client::get_client_port() const
{
    struct sockaddr_in6 addr;
    struct sockaddr *a(reinterpret_cast<struct sockaddr *>(&addr));
    socklen_t len(sizeof(addr));
    int const r(getsockname(f_socket.get(), a, &len));
    if(r != 0)
    {
        return -1;
    }

    // Note: I know the port is at the exact same location in both
    //       structures in Linux but it could change on other Unices
    //
    if(a->sa_family == AF_INET
    && len >= sizeof(sockaddr_in))
    {
        // IPv4
        return reinterpret_cast<sockaddr_in const *>(a)->sin_port;
    }

    if(a->sa_family == AF_INET6
    && len >= sizeof(sockaddr_in6))
    {
        // IPv6
        return addr.sin6_port;
    }

    return -1;
}

/** \brief Get the TCP client address.
 *
 * This function retrieve the IP address of the client (your computer).
 * This is retrieved from the socket using the getsockname() function.
 *
 * \exception event_dispatcher_runtime_error
 * The function raises this exception if the address we retrieve doesn't
 * match its type properly;
 *
 * \return The IP address as a string.
 *
 * \sa get_client_port()
 * \sa get_client_address()
 */
std::string tcp_client::get_client_addr() const
{
    sockaddr_in6 addr;
    sockaddr *a(reinterpret_cast<sockaddr *>(&addr));
    socklen_t len(sizeof(addr));
    int const r(getsockname(f_socket.get(), a, &len));
    if(r != 0)
    {
        throw event_dispatcher_runtime_error("address not available");
    }

    char buf[BUFSIZ];
    switch(a->sa_family)
    {
    case AF_INET:
        if(len < sizeof(sockaddr_in))
        {
            throw event_dispatcher_runtime_error("address size incompatible (AF_INET)");
        }
        inet_ntop(
              AF_INET
            , &reinterpret_cast<sockaddr_in *>(a)->sin_addr
            , buf
            , sizeof(buf));
        break;

    case AF_INET6:
        if(len < sizeof(sockaddr_in6))
        {
            throw event_dispatcher_runtime_error("address size incompatible (AF_INET6)");
        }
        inet_ntop(AF_INET6, &addr.sin6_addr, buf, sizeof(buf));
        break;

    default:
        throw event_dispatcher_runtime_error("unknown address family");

    }

    return buf;
}

/** \brief Get the TCP client address.
 *
 * This function retrieve the IP address of the client (your computer).
 * This is retrieved from the socket using the getsockname() function.
 *
 * \exception event_dispatcher_runtime_error
 * The function raises this exception if the address we retrieve doesn't
 * match its type properly;
 *
 * \return The IP address as a string.
 *
 * \sa get_client_port()
 * \sa get_client_address()
 */
addr::addr tcp_client::get_client_address() const
{
    addr::addr a;
    a.set_from_socket(f_socket.get(), true);
    return a;
}

/** \brief Read data from the socket.
 *
 * A TCP socket is a stream type of socket and one can read data from it
 * as if it were a regular file. This function reads \p size bytes and
 * returns. The function returns early if the server closes the connection.
 *
 * If your socket is blocking, \p size should be exactly what you are
 * expecting or this function will block forever or until the server
 * closes the connection.
 *
 * The function returns -1 if an error occurs. The error is available in
 * errno as expected in the POSIX interface.
 *
 * \param[in,out] buf  The buffer where the data is read.
 * \param[in] size  The size of the buffer.
 *
 * \return The number of bytes read from the socket, or -1 on errors.
 */
int tcp_client::read(char *buf, size_t size)
{
    return static_cast<int>(::read(f_socket.get(), buf, size));
}


/** \brief Read one line.
 *
 * This function reads one line from the current location up to the next
 * '\\n' character. We do not have any special handling of the '\\r'
 * character.
 *
 * The function may return 0 in which case the server closed the connection.
 *
 * \param[out] line  The resulting line read from the server.
 *
 * \return The number of bytes read from the socket, or -1 on errors.
 *         If the function returns 0 or more, then the \p line parameter
 *         represents the characters read on the network.
 */
int tcp_client::read_line(std::string& line)
{
    line.clear();
    int len(0);
    for(;;)
    {
        char c;
        int r(read(&c, sizeof(c)));
        if(r <= 0)
        {
            return len == 0 && r < 0 ? -1 : len;
        }
        if(c == '\n')
        {
            return len;
        }
        ++len;
        line += c;
    }
}


/** \brief Write data to the socket.
 *
 * A TCP socket is a stream type of socket and one can write data to it
 * as if it were a regular file. This function writes \p size bytes to
 * the socket and then returns. This function returns early if the server
 * closes the connection.
 *
 * If your socket is not blocking, less than \p size bytes may be written
 * to the socket. In that case you are responsible for calling the function
 * again to write the remainder of the buffer until the function returns
 * a number of bytes written equal to \p size.
 *
 * The function returns -1 if an error occurs. The error is available in
 * errno as expected in the POSIX interface.
 *
 * \param[in] buf  The buffer with the data to send over the socket.
 * \param[in] size  The number of bytes in buffer to send over the socket.
 *
 * \return The number of bytes that were actually accepted by the socket
 * or -1 if an error occurs.
 */
int tcp_client::write(const char *buf, size_t size)
{
    return static_cast<int>(::write(f_socket.get(), buf, size));
}



} // namespace ed
// vim: ts=4 sw=4 et
