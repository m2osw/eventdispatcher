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
#include    "eventdispatcher/pipe_connection.h"

#include    "eventdispatcher/exception.h"


// C lib
//
#include    <unistd.h>
#include    <sys/socket.h>


// last include
//
#include    <snapdev/poison.h>




namespace ed
{



/** \brief Initializes the pipe connection.
 *
 * This function creates the pipes that are to be used to connect
 * two processes (these are actually Unix sockets). These are
 * used whenever you fork() so the parent process can very quickly
 * communicate with the child process using complex messages just
 * like you do between services and Snap Communicator.
 *
 * \warning
 * The sockets are opened in a non-blocking state. However, they are
 * not closed when you call fork() since they are to be used across
 * processes.
 *
 * \warning
 * You need to create a new pipe_connection each time you want
 * to create a new child.
 *
 * \exception event_dispatcher_initialization_error
 * This exception is raised if the pipes (socketpair) cannot be created.
 */
pipe_connection::pipe_connection()
{
    if(socketpair(AF_LOCAL, SOCK_STREAM | SOCK_NONBLOCK, 0, f_socket) != 0)
    {
        // pipe could not be created
        throw event_dispatcher_initialization_error("somehow the pipes used for a two way pipe connection could not be created.");
    }

    f_parent = getpid();
}


/** \brief Make sure to close the pipes.
 *
 * The destructor ensures that the pipes get closed.
 *
 * They may already have been closed if a broken pipe was detected.
 */
pipe_connection::~pipe_connection()
{
    close();
}


/** \brief Read data from this pipe connection.
 *
 * This function reads up to count bytes from this pipe connection.
 *
 * The function makes sure to use the correct socket for the calling
 * process (i.e. depending on whether this is the parent or child.)
 *
 * Just like the system read(2) function, errno is set to the error
 * that happened when the function returns -1.
 *
 * \param[in] buf  A pointer to a buffer of data.
 * \param[in] count  The number of bytes to read from the pipe connection.
 *
 * \return The number of bytes read from this pipe socket, or -1 on errors.
 */
ssize_t pipe_connection::read(void * buf, size_t count)
{
    int const s(get_socket());
    if(s == -1)
    {
        errno = EBADF;
        return -1;
    }
    return ::read(s, buf, count);
}


/** \brief Write data to this pipe connection.
 *
 * This function writes count bytes to this pipe connection.
 *
 * The function makes sure to use the correct socket for the calling
 * process (i.e. depending on whether this is the parent or child.)
 *
 * Just like the system write(2) function, errno is set to the error
 * that happened when the function returns -1.
 *
 * \param[in] buf  A pointer to a buffer of data.
 * \param[in] count  The number of bytes to write to the pipe connection.
 *
 * \return The number of bytes written to this pipe socket, or -1 on errors.
 */
ssize_t pipe_connection::write(void const * buf, size_t count)
{
    int const s(get_socket());
    if(s == -1)
    {
        errno = EBADF;
        return -1;
    }
    if(buf != nullptr && count > 0)
    {
        return ::write(s, buf, count);
    }
    return 0;
}


/** \brief Close the sockets.
 *
 * This function closes the pair of sockets managed by this
 * pipe connection object.
 *
 * After this call, the pipe connection is closed and cannot be
 * used anymore. The read and write functions will return immediately
 * if called.
 */
void pipe_connection::close()
{
    if(f_socket[0] != -1)
    {
        ::close(f_socket[0]);
        ::close(f_socket[1]);
        f_socket[0] = -1;
        f_socket[1] = -1;
    }
}


/** \brief Pipe connections accept reads.
 *
 * This function returns true meaning that the pipe connection can be
 * used to read data.
 *
 * \return true since a pipe connection is a reader.
 */
bool pipe_connection::is_reader() const
{
    return true;
}


/** \brief This function returns the pipe we want to listen on.
 *
 * This function returns the file descriptor of one of the two
 * sockets. The parent process returns the descriptor of socket
 * number 0. The child process returns the descriptor of socket
 * number 1.
 *
 * \note
 * If the close() function was called, this function returns -1.
 *
 * \return A pipe descriptor to listen on with poll().
 */
int pipe_connection::get_socket() const
{
    if(f_parent == getpid())
    {
        return f_socket[0];
    }

    return f_socket[1];
}







} // namespace ed
// vim: ts=4 sw=4 et
