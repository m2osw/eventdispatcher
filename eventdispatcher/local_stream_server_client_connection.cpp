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
 * \brief Implementation of the Unix stream server-client class.
 *
 * Whenever a Unix stream server accepts a new connection, it expects the
 * accept() function to return a socket descriptor which is used to create
 * a local_stream_server_client_connection object. If you want to receive
 * data by line, use a buffer. If you want messages, use the message
 * version.
 */

// self
//
#include    "eventdispatcher/local_stream_server_client_connection.h"

#include    "eventdispatcher/exception.h"


// snaplogger lib
//
#include    <snaplogger/message.h>


// C++ lib
//
#include    <cstring>


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
 * \param[in] client  The client that accept() returned.
 */
local_stream_server_client_connection::local_stream_server_client_connection(snap::raii_fd_t client)
    : f_client(std::move(client))
{
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
ssize_t local_stream_server_client_connection::read(void * buf, size_t count)
{
    if(f_client == nullptr)
    {
        errno = EBADF;
        return -1;
    }
    return ::read(f_client.get(), reinterpret_cast<char *>(buf), count);
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
 * you want to instead use the local_stream_server_client_buffer_connection
 * which overloads the write() function and saves the data to be
 * written to the socket in a buffer. The communicator run
 * loop is then responsible for sending all the data.
 *
 * \param[in] buf  The buffer of data to be written to the socket.
 * \param[in] count  The number of bytes the caller wants to write to the
 *                   connection.
 *
 * \return The number of bytes written to the socket or -1 if an error occurred.
 */
ssize_t local_stream_server_client_connection::write(void const * buf, size_t count)
{
    if(f_client == nullptr)
    {
        errno = EBADF;
        return -1;
    }
    return ::write(f_client.get(), reinterpret_cast<char const *>(buf), count);
}


/** \brief Retrieve the socket of this connection.
 *
 * This function returns the socket defined in this connection. It is
 * the socket that was received through an accept() call.
 *
 * \return The socket descriptor of this connection.
 */
int local_stream_server_client_connection::get_socket() const
{
    if(f_client == nullptr)
    {
        // client connection was closed
        //
        errno = EBADF;
        return -1;
    }
    return f_client.get();
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
bool local_stream_server_client_connection::is_reader() const
{
    return true;
}


/** \brief Close this connection.
 *
 * This function is most often called on an error to clearly mark it
 * as closed. That way other function that attempt to use the socket
 * fill automatically fail instead of trying to access another file.
 */
void local_stream_server_client_connection::close()
{
    f_client.reset();
}


/** \brief Retrieve a copy of the client's address.
 *
 * This function returns a copy of the address of this client connection.
 *
 * The function may throw an error if the address is (somehow) not
 * supported, which is possible in clients that connect to servers
 * other than eventdispatcher servers.
 *
 * \return The Unix address this client is connected to.
 */
addr::unix local_stream_server_client_connection::get_client_address() const
{
    // make sure the address is defined and the socket open
    //
    const_cast<local_stream_server_client_connection *>(this)->define_address();
    return f_address;
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
 */
void local_stream_server_client_connection::define_address()
{
    int const s(get_socket());
    if(s == -1)
    {
        errno = EBADF;
        return;
    }

    if(!f_address_defined)
    {
        // address not defined yet, retrieve it with getsockname()
        //
        f_address_defined = true;
        f_address.set_from_socket(s);
    }
}



} // namespace ed
// vim: ts=4 sw=4 et
