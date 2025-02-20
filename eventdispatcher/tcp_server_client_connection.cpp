// Copyright (c) 2012-2024  Made to Order Software Corp.  All Rights Reserved
//
// https://snapwebsites.org/project/eventdispatcher
// contact@m2osw.com
//
// This program is free software: you can redistribute it and/or modify
// it under the terms of the GNU General Public License as published by
// the Free Software Foundation, either version 3 of the License, or
// (at your option) any later version.
//
// This program is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU General Public License for more details.
//
// You should have received a copy of the GNU General Public License
// along with this program.  If not, see <https://www.gnu.org/licenses/>.

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


// snaplogger
//
#include    <snaplogger/message.h>


// C++
//
#include    <cstring>


// C
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
 * \param[in] client  The client that accept() returned.
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
    if(f_client == nullptr)
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
 * sure that all your data is written to the other side,
 * you want to instead use the tcp_server_client_buffer_connection,
 * which overloads this write() function and saves the data to be
 * written to the socket in a buffer. The communicator run()-loop is
 * then responsible for sending all the data. However, that buffering
 * has no limit, so if you are sending large files, it is also not
 * a very good idea.
 *
 * \param[in] buf  The buffer of data to be written to the socket.
 * \param[in] count  The number of bytes the caller wants to write to the
 *                   connection.
 *
 * \return The number of bytes written to the socket or -1 if an error occurred.
 */
ssize_t tcp_server_client_connection::write(void const * buf, size_t count)
{
    if(f_client == nullptr)
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
 *
 * \return The socket file descriptor or -1 if the connection is closed.
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
 * This function retrieves a copy of the client's address and returns it.
 *
 * \return A reference to the client's address.
 */
addr::addr tcp_server_client_connection::get_client_address() 
{
    if(f_address.is_default())
    {
        int const s(get_socket());
        if(s >= 0)
        {
            f_address.set_from_socket(s, false);
        }
    }

    return f_address;
}



} // namespace ed
// vim: ts=4 sw=4 et
