// Copyright (c) 2012-2019  Made to Order Software Corp.  All Rights Reserved
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


// snaplogger lib
//
#include    <snaplogger/message.h>


//// C lib
////
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

/** \brief Contruct a tcp_client object.
 *
 * The tcp_client constructor initializes a TCP client object by connecting
 * to the specified server. The server is defined with the \p addr and
 * \p port specified as parameters.
 *
 * \exception tcp_client_server_parameter_error
 * This exception is raised if the \p port parameter is out of range or the
 * IP address is an empty string or otherwise an invalid address.
 *
 * \exception tcp_client_server_runtime_error
 * This exception is raised if the client cannot create the socket or it
 * cannot connect to the server.
 *
 * \param[in] addr  The address of the server to connect to. It must be valid.
 * \param[in] port  The port the server is listening on.
 */
tcp_client::tcp_client(std::string const & addr, int port)
    : f_socket(-1)
    , f_port(port)
    , f_addr(addr)
{
    if(f_port < 0 || f_port >= 65536)
    {
        throw event_dispatcher_invalid_parameter("invalid port for a client socket");
    }
    if(f_addr.empty())
    {
        throw event_dispatcher_invalid_parameter("an empty address is not valid for a client socket");
    }

    addrinfo hints;
    memset(&hints, 0, sizeof(hints));
    hints.ai_family = AF_UNSPEC;
    hints.ai_socktype = SOCK_STREAM;
    hints.ai_protocol = IPPROTO_TCP;
    std::string const port_str(std::to_string(f_port));
    addrinfo * addrinfo(nullptr);
    int const r(getaddrinfo(addr.c_str(), port_str.c_str(), &hints, &addrinfo));
    raii_addrinfo_t addr_info(addrinfo);
    if(r != 0
    || addrinfo == nullptr)
    {
        int const e(errno);
        std::string err("getaddrinfo() failed to parse the address or port \"");
        err += addr;
        err += ":";
        err += port_str;
        err += "\" strings (errno: ";
        err += std::to_string(e);
        err += " -- ";
        err += strerror(e);
        err += ")";
        SNAP_LOG_FATAL << err;
        throw event_dispatcher_runtime_error(err);
    }

    f_socket = socket(addr_info.get()->ai_family, SOCK_STREAM, IPPROTO_TCP);
    if(f_socket < 0)
    {
        int const e(errno);
        SNAP_LOG_FATAL
            << "socket() failed to create a socket descriptor (errno: "
            << e
            << " -- "
            << strerror(e)
            << ")";
        throw event_dispatcher_runtime_error("could not create socket for client");
    }

    if(connect(f_socket, addr_info.get()->ai_addr, addr_info.get()->ai_addrlen) < 0)
    {
        int const e(errno);
        SNAP_LOG_FATAL
            << "connect() failed to connect a socket (errno: "
            << e
            << " -- "
            << strerror(e)
            << ")";
        close(f_socket);
        throw event_dispatcher_runtime_error("could not connect client socket to \"" + f_addr + "\"");
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
    close(f_socket);
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
    return f_socket;
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
    return f_port;
}

/** \brief Get the TCP server address.
 *
 * This function returns the address used when creating the TCP address as is.
 * Note that this is the address of the server where the client is connected
 * and not the address where the client is running (although it may be the
 * same.)
 *
 * Use the get_client_addr() function to retrieve the client's TCP address.
 *
 * \return The TCP client address.
 */
std::string tcp_client::get_addr() const
{
    return f_addr;
}

/** \brief Get the TCP client port.
 *
 * This function retrieve the port of the client (used on your computer).
 * This is retrieved from the socket using the getsockname() function.
 *
 * \return The port or -1 if it cannot be determined.
 */
int tcp_client::get_client_port() const
{
    struct sockaddr addr;
    socklen_t len(sizeof(addr));
    int r(getsockname(f_socket, &addr, &len));
    if(r != 0)
    {
        return -1;
    }
    // Note: I know the port is at the exact same location in both
    //       structures in Linux but it could change on other Unices
    if(addr.sa_family == AF_INET)
    {
        // IPv4
        return reinterpret_cast<sockaddr_in *>(&addr)->sin_port;
    }
    if(addr.sa_family == AF_INET6)
    {
        // IPv6
        return reinterpret_cast<sockaddr_in6 *>(&addr)->sin6_port;
    }
    return -1;
}

/** \brief Get the TCP client address.
 *
 * This function retrieve the IP address of the client (your computer).
 * This is retrieved from the socket using the getsockname() function.
 *
 * \return The IP address as a string.
 */
std::string tcp_client::get_client_addr() const
{
    struct sockaddr addr;
    socklen_t len(sizeof(addr));
    int const r(getsockname(f_socket, &addr, &len));
    if(r != 0)
    {
        throw event_dispatcher_runtime_error("address not available");
    }
    char buf[BUFSIZ];
    switch(addr.sa_family)
    {
    case AF_INET:
        if(len < sizeof(struct sockaddr_in))
        {
            throw event_dispatcher_runtime_error("address size incompatible (AF_INET)");
        }
        inet_ntop(AF_INET, &reinterpret_cast<struct sockaddr_in *>(&addr)->sin_addr, buf, sizeof(buf));
        break;

    case AF_INET6:
        if(len < sizeof(struct sockaddr_in6))
        {
            throw event_dispatcher_runtime_error("address size incompatible (AF_INET6)");
        }
        inet_ntop(AF_INET6, &reinterpret_cast<struct sockaddr_in6 *>(&addr)->sin6_addr, buf, sizeof(buf));
        break;

    default:
        throw event_dispatcher_runtime_error("unknown address family");

    }
    return buf;
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
    return static_cast<int>(::read(f_socket, buf, size));
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
    return static_cast<int>(::write(f_socket, buf, size));
}



} // namespace tcp_client_server
// vim: ts=4 sw=4 et
