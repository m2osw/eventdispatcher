// Copyright (c) 2012-2024  Made to Order Software Corp.  All Rights Reserved
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
 * \brief Handle an AF_UNIX socket.
 *
 * Class used to handle a Unix socket.
 */


// self
//
#include    "eventdispatcher/local_stream_client_connection.h"

#include    "eventdispatcher/exception.h"


// snaplogger
//
#include    <snaplogger/message.h>


// last include
//
#include    <snapdev/poison.h>



namespace ed
{



/** \class local_stream_client_connection
 * \brief Create a client socket and connect to a server.
 *
 * This class is a client socket implementation used to connect to a server.
 * The server is expected to be running at the time the client is created
 * otherwise it fails connecting.
 *
 * This class is not appropriate to connect to a server that may come and go
 * over time.
 */




/** \brief Construct a local_stream_client_connection object.
 *
 * The local_stream_client_connection constructor initializes a streaming
 * Unix socket object by connecting to the specified server. The server
 * is defined with the \p u parameters.
 *
 * \exception runtime_error
 * This exception is raised if the client cannot create the socket or it
 * cannot connect to the server.
 *
 * \param[in] address  The Unix address of the server to connect to.
 * \param[in] blocking  Whether the socket has to be opened in blocking mode.
 * \param[in] close_on_exec  Whether to close this socket on an execve() call.
 */
local_stream_client_connection::local_stream_client_connection(
              addr::addr_unix const & address
            , bool const blocking
            , bool const close_on_exec)
    : f_address(address)
{
    sockaddr_un un;
    f_address.get_un(un);
    f_socket.reset(socket(
                  un.sun_family
                , SOCK_STREAM
                    | (blocking ? 0 : SOCK_NONBLOCK)
                    | (close_on_exec ? SOCK_CLOEXEC : 0)
                , 0));
    if(f_socket == nullptr)
    {
        int const e(errno);
        SNAP_LOG_FATAL
            << "socket() failed to create a Unix socket descriptor (errno: "
            << e
            << " -- "
            << strerror(e)
            << ")"
            << SNAP_LOG_SEND;
        throw runtime_error("could not create socket for client");
    }

    if(f_address.is_unnamed())
    {
        // for an unnamed socket, we do not bind at all the user is
        // responsible for knowing where to read and where to write
        //
        return;
    }

    int r(-1);
    if(f_address.is_abstract())
    {
        // to create a correct abstract socket, we need to adjust the
        // size to the exact length of the address (i.e. we do not want
        // to send the '\0' after the name)
        //
        std::size_t const size(strlen(un.sun_path + 1));
        r = connect(
                  f_socket.get()
                , reinterpret_cast<sockaddr const *>(&un)
                , size);
    }
    else
    {
        // in this case we can send the entire address
        //
        r = connect(
                  f_socket.get()
                , reinterpret_cast<sockaddr const *>(&un)
                , sizeof(un));
    }

    if(r < 0)
    {
        int const e(errno);
        SNAP_LOG_FATAL
            << "connect() failed to connect a socket with address \""
            << f_address.to_uri()
            << "\" (errno: "
            << e
            << " -- "
            << strerror(e)
            << ")"
            << SNAP_LOG_SEND;
        throw runtime_error(
                  "could not connect client socket to \""
                + f_address.to_uri()
                + "\"");
    }

    if(!blocking)
    {
        non_blocking();
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
local_stream_client_connection::~local_stream_client_connection()
{
}


/** \brief Close this connection.
 *
 * A connction automatically gets closed when it gets destroyed. There are
 * times, though, when it is useful to close the connection early
 * (specifically, when you receive an error).
 *
 * This function can be used to close the connection at any time. Many
 * of the other functions will stop working once the connection is
 * officially closed.
 */
void local_stream_client_connection::close()
{
    f_socket.reset();
}


/** \brief Get the Unix server address.
 *
 * This function returns the address used when creating the Unix server.
 *
 * \return The Unix address.
 */
addr::addr_unix local_stream_client_connection::get_address() const
{
    return f_address;
}


/** \brief Check whether this connection is a reader.
 *
 * We change the default to true since Unix sockets are generally
 * always readers. You can still override this function and
 * return false if necessary.
 *
 * However, we do not override the is_writer() because that is
 * much more dynamic (i.e. you do not want to advertise as
 * being a writer unless you have data to write to the
 * socket.)
 *
 * \return Always true since this is a reader connection.
 */
bool local_stream_client_connection::is_reader() const
{
    return true;
}


/** \brief Get the socket descriptor.
 *
 * This function returns the TCP client socket descriptor. This can be
 * used to change the descriptor behavior (i.e. make it non-blocking for
 * example.)
 *
 * \return The socket descriptor.
 */
int local_stream_client_connection::get_socket() const
{
    return f_socket.get();
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
ssize_t local_stream_client_connection::read(char * buf, size_t size)
{
    return ::read(f_socket.get(), buf, size);
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
ssize_t local_stream_client_connection::write(void const * buf, size_t size)
{
    return ::write(f_socket.get(), buf, size);
}




} // namespace ed
// vim: ts=4 sw=4 et
