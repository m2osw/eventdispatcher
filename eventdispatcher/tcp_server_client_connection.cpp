// Copyright (c) 2012-2020  Made to Order Software Corp.  All Rights Reserved
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
#include    "eventdispatcher/tcp_server_client_connection.h"

#include    "eventdispatcher/exception.h"


// snaplogger lib
//
#include    <snaplogger/message.h>


// C lib
//
#include    <arpa/inet.h>
#include    <netdb.h>


// last include
//
#include    <snapdev/poison.h>



namespace ed
{



/** \brief Create a client connection created from an accept().
 *
 * This constructor initializes a client connection from a socket
 * that we received from an accept() call.
 *
 * The destructor will automatically close that socket on destruction.
 *
 * \param[in] client  The client that acecpt() returned.
 */
tcp_server_client_connection::tcp_server_client_connection(tcp_bio_client::pointer_t client)
    : f_client(client)
{
}


/** \brief Make sure the socket gets released.
 *
 * This destructor makes sure that the socket gets closed.
 */
tcp_server_client_connection::~tcp_server_client_connection()
{
    close();
}


/** \brief Read data from the TCP server client socket.
 *
 * This function reads as much data up to the specified amount
 * in \p count. The read data is saved in \p buf.
 *
 * \param[in,out] buf  The buffer where the data gets read.
 * \param[in] count  The maximum number of bytes to read in buf.
 *
 * \return The number of bytes read or -1 if an error occurred.
 */
ssize_t tcp_server_client_connection::read(void * buf, size_t count)
{
    if(!f_client)
    {
        errno = EBADF;
        return -1;
    }
    return f_client->read(reinterpret_cast<char *>(buf), count);
}


/** \brief Write data to this connection's socket.
 *
 * This function writes up to \p count bytes of data from \p buf
 * to this connection's socket.
 *
 * \warning
 * This write function may not always write all the data you are
 * trying to send to the remote connection. If you want to make
 * sure that all your data is written to the other connection,
 * you want to instead use the tcp_server_client_buffer_connection
 * which overloads the write() function and saves the data to be
 * written to the socket in a buffer. The communicator run
 * loop is then responsible for sending all the data.
 *
 * \param[in] buf  The buffer of data to be written to the socket.
 * \param[in] count  The number of bytes the caller wants to write to the
 *                   conneciton.
 *
 * \return The number of bytes written to the socket or -1 if an error occurred.
 */
ssize_t tcp_server_client_connection::write(void const * buf, size_t count)
{
    if(!f_client)
    {
        errno = EBADF;
        return -1;
    }
    return f_client->write(reinterpret_cast<char const *>(buf), count);
}


/** \brief Close the socket of this connection.
 *
 * This function is automatically called whenever the object gets
 * destroyed (see destructor) or detects that the client closed
 * the network connection.
 *
 * Connections cannot be reopened.
 */
void tcp_server_client_connection::close()
{
    f_client.reset();
}


/** \brief Retrieve the socket of this connection.
 *
 * This function returns the socket defined in this connection.
 */
int tcp_server_client_connection::get_socket() const
{
    if(f_client == nullptr)
    {
        // client connection was closed
        //
        return -1;
    }
    return f_client->get_socket();
}


/** \brief Tell that we are always a reader.
 *
 * This function always returns true meaning that the connection is
 * always of a reader. In most cases this is safe because if nothing
 * is being written to you then poll() never returns so you do not
 * waste much time in have a TCP connection always marked as a
 * reader.
 *
 * \return The events to listen to for this connection.
 */
bool tcp_server_client_connection::is_reader() const
{
    return true;
}


/** \brief Retrieve a copy of the client's address.
 *
 * This function makes a copy of the address of this client connection
 * to the \p address parameter and returns the length.
 *
 * If the function returns zero, then the \p address buffer is not
 * modified and no address is defined in this connection.
 *
 * \param[out] address  The reference to an address variable where the
 *                      client's address gets copied.
 *
 * \return Return the length of the address which may be smaller than
 *         sizeof(address). If zero, then no address is defined.
 *
 * \sa get_addr()
 */
size_t tcp_server_client_connection::get_client_address(sockaddr_storage & address) const
{
    // make sure the address is defined and the socket open
    //
    if(const_cast<tcp_server_client_connection *>(this)->define_address() != 0)
    {
        return 0;
    }

    address = f_address;
    return f_length;
}


/** \brief Retrieve the address in the form of a string.
 *
 * Like the get_addr() of the tcp client and server classes, this
 * function returns the address in the form of a string which can
 * easily be used to log information and other similar tasks.
 *
 * \todo
 * Look at using libaddr for the convertion.
 *
 * \return The client's address in the form of a string.
 */
std::string tcp_server_client_connection::get_client_addr() const
{
    // make sure the address is defined and the socket open
    //
    if(!const_cast<tcp_server_client_connection *>(this)->define_address())
    {
        return std::string();
    }

    size_t const max_length(std::max(INET_ADDRSTRLEN, INET6_ADDRSTRLEN) + 1);

// in release mode this should not be dynamic (although the syntax is so
// the warning would happen), but in debug it is likely an alloca()
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wvla"
    char buf[max_length];
#pragma GCC diagnostic pop

    char const * r(nullptr);

    if(f_address.ss_family == AF_INET)
    {
        r = inet_ntop(AF_INET, &reinterpret_cast<sockaddr_in const &>(f_address).sin_addr, buf, max_length);
    }
    else
    {
        r = inet_ntop(AF_INET6, &reinterpret_cast<sockaddr_in6 const &>(f_address).sin6_addr, buf, max_length);
    }

    if(r == nullptr)
    {
        int const e(errno);
        std::string err("inet_ntop() could not convert IP address (errno: ");
        err += std::to_string(e);
        err += " -- ";
        err += strerror(e);
        err += ").";
        SNAP_LOG_FATAL << err << SNAP_LOG_SEND;
        throw event_dispatcher_runtime_error(err);
    }

    return buf;
}


/** \brief Retrieve the port.
 *
 * This function returns the port of the socket on our side.
 *
 * If the port is not available (not connected?), then -1 is returned.
 *
 * \return The client's port in host order.
 */
int tcp_server_client_connection::get_client_port() const
{
    // make sure the address is defined and the socket open
    //
    if(!const_cast<tcp_server_client_connection *>(this)->define_address())
    {
        return -1;
    }

    if(f_address.ss_family == AF_INET)
    {
        return ntohs(reinterpret_cast<sockaddr_in const &>(f_address).sin_port);
    }
    else
    {
        return ntohs(reinterpret_cast<sockaddr_in6 const &>(f_address).sin6_port);
    }
}


/** \brief Retrieve the address in the form of a string.
 *
 * Like the get_addr() of the tcp client and server classes, this
 * function returns the address in the form of a string which can
 * easily be used to log information and other similar tasks.
 *
 * \todo
 * Look at using libaddr for the convertion.
 *
 * \return The client's address in the form of a string.
 */
std::string tcp_server_client_connection::get_client_addr_port() const
{
    // get the current address and port
    std::string const addr(get_client_addr());
    int const port(get_client_port());

    // make sure they are defined
    if(addr.empty()
    || port < 0)
    {
        return std::string();
    }

    // calculate the result
    std::string buf;
    buf.reserve(addr.length() + (3 + 5));
    if(f_address.ss_family == AF_INET)
    {
        buf += addr;
        buf += ':';
    }
    else
    {
        buf += '[';
        buf += addr;
        buf += "]:";
    }
    buf += std::to_string(port);

    return buf;
}


/** \brief Retrieve the socket address if we have not done so yet.
 *
 * This function make sure that the f_address and f_length parameters are
 * defined. This is done by calling the getsockname() function.
 *
 * If f_length is still zero, then it is expected that address was not
 * yet read.
 *
 * Note that the function returns -1 if the socket is now -1 (i.e. the
 * connection is closed) whether or not the function worked before.
 *
 * \return false if the address cannot be defined, true otherwise
 */
bool tcp_server_client_connection::define_address()
{
    int const s(get_socket());
    if(s == -1)
    {
        return false;
    }

    if(f_length == 0)
    {
        // address not defined yet, retrieve with with getsockname()
        //
        f_length = sizeof(f_address);
        if(getsockname(s, reinterpret_cast<struct sockaddr *>(&f_address), &f_length) != 0)
        {
            int const e(errno);
            SNAP_LOG_ERROR
                << "getsockname() failed retrieving IP address (errno: "
                << e
                << " -- "
                << strerror(e)
                << ")."
                << SNAP_LOG_SEND;
            f_length = 0;
            return false;
        }
        if(f_address.ss_family != AF_INET
        && f_address.ss_family != AF_INET6)
        {
            SNAP_LOG_ERROR
                << "address family ("
                << f_address.ss_family
                << ") returned by getsockname() is not understood, it is neither an IPv4 nor IPv6."
                << SNAP_LOG_SEND;
            f_length = 0;
            return false;
        }
        if(f_length < sizeof(f_address))
        {
            // reset the rest of the structure, just in case
            //
            memset(reinterpret_cast<char *>(&f_address) + f_length, 0, sizeof(f_address) - f_length);
        }
    }

    return true;
}



} // namespace ed
// vim: ts=4 sw=4 et
