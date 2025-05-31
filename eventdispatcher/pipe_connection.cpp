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


// C++
//
#include    <iostream>


// C
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
 * This function creates pipes to quickly communicate between two
 * processes created with a fork() or fork() + execve().
 *
 * The function supports three types of pipes:
 *
 * * PIPE_BIDIRECTIONAL
 *
 *     Create Unix sockets that can be used to send and receive messages
 *     between two processes.
 *
 * * PIPE_CHILD_INPUT
 *
 *     A pipe for the input stream of a child. This type of pipe is to
 *     be used as a replacement to the stdin of a child process. This is
 *     used in the cppprocess library when creating a fork() + execve()
 *     process.
 *
 * * PIPE_CHILD_OUTPUT
 *
 *     A pipe for the output stream and error stream of a child. This
 *     type of pipe is to be used as a replacement to the stdout and
 *     stderr of a child process. This is used in the cppprocess when
 *     creating a fork() + execve() process.
 *
 * To save file descriptors (1 or 2), it is suggested that you
 * call the forked() function once you called fork(). It will
 * take care of closing the descriptors used by the other side
 * (those you do not need on your side).
 *
 * \warning
 * The file descriptors are opened in a non-blocking state. However, they
 * are not closed when you call fork() since they are to be used across
 * processes. If you want to do a fork() + execve(), duplicating the
 * necessary file descriptors and then call close().
 *
 * \warning
 * You need to create a new pipe_connection each time you want to create
 * a new child with fork(). If you want to create a a new process with
 * fork() + execve(), you want to create three pipe_connection, one per
 * stream (stdin, stdout, stderr). You can always create more if necessary
 * in your situation.
 *
 * \exception initialization_error
 * This exception is raised if the pipes (socketpair, pipe) cannot be
 * created.
 *
 * \param[in] type  The type of pipe to create.
 *
 * \sa forked()
 */
pipe_connection::pipe_connection(pipe_t type)
    : f_type(type)
    , f_parent(getpid())
{
    switch(type)
    {
    case pipe_t::PIPE_BIDIRECTIONAL:
        if(socketpair(AF_LOCAL, SOCK_STREAM | SOCK_NONBLOCK, 0, f_socket) != 0)
        {
            // pipe could not be created
            throw initialization_error("somehow the AF_LOCAL pipes used for a two way pipe connection could not be created.");
        }
        break;

    case pipe_t::PIPE_CHILD_INPUT:
        if(pipe(f_socket) != 0)
        {
            // pipe could not be created
            throw initialization_error("somehow the FIFO pipes used for a one way pipe (child input) connection could not be created.");
        }
        std::swap(f_socket[0], f_socket[1]);
        break;

    case pipe_t::PIPE_CHILD_OUTPUT:
        if(pipe(f_socket) != 0)
        {
            // pipe could not be created
            throw initialization_error("somehow the FIFO pipes used for a one way pipe (child output) connection could not be created.");
        }
        break;

    }
}


/** \brief Make sure to close the pipes.
 *
 * The destructor ensures that the pipes get closed.
 *
 * They may already have been closed if a broken pipe was detected.
 *
 * \sa close()
 */
pipe_connection::~pipe_connection()
{
    close();
}


/** \brief Get the type of pipe connections.
 *
 * This function returns the type of this connection, as specified
 * on the contructor.
 *
 * \return The type of the pipe.
 */
pipe_t pipe_connection::type() const
{
    return f_type;
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
 * The function returns an error (-1, errno = EBAD) if you try to
 * read from a read-only stream.
 *
 * \param[in] buf  A pointer to a buffer of data.
 * \param[in] count  The number of bytes to read from the pipe connection.
 *
 * \return The number of bytes read from this pipe socket, or -1 on errors.
 *
 * \sa write()
 */
ssize_t pipe_connection::read(void * buf, size_t count)
{
    if(f_parent == getpid())
    {
        if(f_type == pipe_t::PIPE_CHILD_INPUT)
        {
            errno = EBADF;
            return -1;
        }
    }
    else
    {
        if(f_type == pipe_t::PIPE_CHILD_OUTPUT)
        {
            errno = EBADF;
            return -1;
        }
    }

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
 * The function returns an error (-1, errno = EBAD) if you try to
 * write to a read-only stream.
 *
 * \param[in] buf  A pointer to a buffer of data.
 * \param[in] count  The number of bytes to write to the pipe connection.
 *
 * \return The number of bytes written to this pipe socket, or -1 on errors.
 *
 * \sa read()
 */
ssize_t pipe_connection::write(void const * buf, size_t count)
{
    if(f_parent == getpid())
    {
        if(f_type == pipe_t::PIPE_CHILD_OUTPUT)
        {
            errno = EBADF;
            return -1;
        }
    }
    else
    {
        if(f_type == pipe_t::PIPE_CHILD_INPUT)
        {
            errno = EBADF;
            return -1;
        }
    }

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


/** \brief Close the other side sockets.
 *
 * This function closes the sockets not used by this side of the pipe.
 * This is useful to clean up file descriptors that we otherwise endup
 * to never use until this object is destroyed.
 *
 * Make sure to call this function only after you called the fork()
 * function. Calling this too early (before the fork() call) would
 * mean that you'd end up closing the file descriptors of the other
 * side before the other side received the socket information.
 *
 * The function can safely be called multiple times.
 *
 * \note
 * To close both sides, call the close() function.
 *
 * \sa close()
 */
void pipe_connection::forked()
{
    int const idx(f_parent == getpid() ? 1 : 0);
    if(f_socket[idx] != -1)
    {
        ::close(f_socket[idx]);
        f_socket[idx] = -1;
    }
}


/** \brief Close the sockets.
 *
 * This function closes the pair of sockets managed by this
 * pipe connection object.
 *
 * After this call, the pipe connection is closed and cannot be
 * used anymore. The read and write functions will return immediately
 * if called.
 *
 * The function can safely be called multiple times. It is also safe
 * to call the forked() function and then the close() function.
 *
 * \sa forked()
 */
void pipe_connection::close()
{
    if(f_socket[0] != -1)
    {
        ::close(f_socket[0]);
        f_socket[0] = -1;
    }

    if(f_socket[1] != -1)
    {
        ::close(f_socket[1]);
        f_socket[1] = -1;
    }
}


/** \brief This function returns the pipe of the other side.
 *
 * In some cases, it can be practical to find out the socket from the other
 * side of the pipe. This function returns that other socket.
 *
 * \note
 * The main reason for this function is to be able to extract the other
 * socket and create the necessary input/output/error stream in a child
 * process and close it.
 *
 * \return A pipe descriptor to listen on with poll().
 */
int pipe_connection::get_other_socket() const
{
    if(f_parent == getpid())
    {
        return f_socket[1];
    }

    return f_socket[0];
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
    if(f_parent == getpid())
    {
        if(f_type == pipe_t::PIPE_CHILD_INPUT)
        {
            return false;
        }
    }
    else
    {
        if(f_type == pipe_t::PIPE_CHILD_OUTPUT)
        {
            return false;
        }
    }

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


/** \var pipe_t::PIPE_BIDIRECTIONAL
 * \brief Used to create a bidirectional pipe.
 *
 * The pipe is bidirectional. You can read and write to it.
 */


/** \var pipe_t::PIPE_CHILD_INPUT
 * \brief Used to create an input stream for a child.
 *
 * This type defines a pipe where the parent writes to the pipe and the
 * child reads from the pipe.
 */


/** \var pipe_t::PIPE_CHILD_OUTPUT
 * \brief Used to create an output stream for a child.
 *
 * This type defines a pipe where the parent reads from the pipe and the
 * child writes to the pipe.
 */



} // namespace ed
// vim: ts=4 sw=4 et
