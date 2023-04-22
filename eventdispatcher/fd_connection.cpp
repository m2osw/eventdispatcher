// Copyright (c) 2012-2023  Made to Order Software Corp.  All Rights Reserved
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
#include    "eventdispatcher/fd_connection.h"


// C
//
#include    <unistd.h>


// last include
//
#include    <snapdev/poison.h>



namespace ed
{



/** \brief Initializes the file descriptor connection.
 *
 * This function creates a connection based on an existing file descriptor.
 * This is a class used to handle existing pipes or sockets (opposed to other
 * implementations which create a pipe, open a socket, etc.) It is especially
 * useful if you want to listen to stdin and stdout. Use the `fileno()`
 * function to get the file descriptor and create an `fd_connection`
 * object with that descriptor.
 *
 * The mode parameter defines how you are to use the file descriptor. In
 * other words, a socket that is read/write could be added to two different
 * `fd_connection` objects: one to read and one to write, instead of
 * a single read/write object.
 *
 * Note that although you can say it's only going to be used in READ or
 * in WRITE mode, in all cases, make sure that the file is readable before
 * specifying READ or RW, and make sure that the file is writable before
 * specifying WRITE or RW.
 *
 * \note
 * It is important to note that the lifetime of the file descriptor is
 * not managed by this object. You are responsible for the descriptor to
 * stay valid as long as the connection is added to the communicator
 * list of connections. If you want to close the connection, please first
 * remove the connection from the communicator, destroy the connection,
 * then close the file descriptor.
 *
 * \note
 * It is possible to pass -1 (or any negative number) as the file
 * descriptor. In that case it is interpreted as "not a valid
 * file descriptor."
 *
 * \warning
 * If you are to use a read() or a write() that may block, make sure to
 * first set your file descriptor in non-blocking mode. If that's not
 * possible, then make sure to read or write only one byte at a time.
 * The loop will be slower, but you will avoid blocks which would
 * prevent the rest of the software from getting their own events.
 *
 * \param[in] fd  The file descriptor to handle.
 * \param[in] mode  The mode this descriptor is to be used by the connection.
 */
fd_connection::fd_connection(int fd, mode_t mode)
    : f_fd(fd)
    , f_mode(mode)
{
}


/** \brief Used to close the file descriptor.
 *
 * This function closes the file descriptor of this connection.
 *
 * The function is not called automatically as we mention in the
 * constructor, the function cannot just close the file descriptor
 * on its own so it is up to you to call this function or not.
 * It is not mandatory.
 *
 * The function does not verify to know whether the file descriptor
 * was already closed outside of this function. Although it is safe
 * to call this function multiple times, if you close the file
 * descriptor with other means, then calling this function may
 * end up closing another file...
 */
void fd_connection::close()
{
    if(f_fd != -1)
    {
        ::close(f_fd);
        f_fd = -1;
    }
}


/** \brief Used to mark the file descriptor as closed.
 *
 * This function marks the file descriptor as closed. Whether it is,
 * is your own concern. This is used to avoid a double close in
 * case some other function ends up calling close() and yet you
 * somehow closed the file descriptor (i.e. fclose(f) will do that
 * on you...)
 *
 * \code
 *      ...
 *      fclose(f);
 *      // tell connection that we've close the connection fd
 *      c->mark_closed();
 *      ...
 * \endcode
 *
 * Note that if you do not have any other copy of the file descriptor
 * and you call mark_closed() instead of close(), you will leak that
 * file descriptor.
 */
void fd_connection::mark_closed()
{
    f_fd = -1;
}


/** \brief Check whether this connection is a reader.
 *
 * If you created this file descriptor connection as a reader, then this
 * function returns true.
 *
 * A reader has a mode of FD_MODE_READ or FD_MODE_RW.
 *
 * \return true if the connection is considered to be a reader.
 */
bool fd_connection::is_reader() const
{
    return f_mode != mode_t::FD_MODE_WRITE && get_socket() != -1;
}


/** \brief Check whether this connection is a writer.
 *
 * If you created this file descriptor connection as a writer, then this
 * function returns true.
 *
 * A writer has a mode of FD_MODE_WRITE or FD_MODE_RW.
 *
 * \return true if the connection is considered to be a writer.
 */
bool fd_connection::is_writer() const
{
    return f_mode != mode_t::FD_MODE_READ && get_socket() != -1;
}


/** \brief Return the file descriptor ("socket").
 *
 * This function returns the file descriptor specified in the constructor.
 *
 * The current naming convention comes from the fact that the library
 * was first created for sockets.
 *
 * \return The connection file descriptor.
 */
int fd_connection::get_socket() const
{
    return f_fd;
}


/** \brief Read up data from the file descriptor.
 *
 * This function attempts to read up to \p count bytes of data from
 * this file descriptor. If that works to some extend, then the
 * data read will be saved in \p buf and the function returns
 * the number of bytes read.
 *
 * If no data can be read, the function may return -1 or 0.
 *
 * The file descriptor must be a reader or the function will always fail.
 *
 * \param[in] buf  The buffer where the data read is saved.
 * \param[in] count  The maximum number of bytes to read at once.
 *
 * \return The number of bytes read from the file connection.
 */
ssize_t fd_connection::read(void * buf, size_t count)
{
    // WARNING: We MUST call the fd_connection version of the is_writer(),
    //          because the fd_buffer_connection::is_writer() also checks
    //          the f_output buffer which has unwanted side effects
    //
    if(!fd_connection::is_reader())
    {
        errno = EBADF;
        return -1;
    }

    return ::read(f_fd, buf, count);
}


/** \brief Write buffer data to the file descriptor.
 *
 * This function writes \p count bytes of data from \p buf to
 * the file descriptor attached to this connection.
 *
 * If the file descriptor is closed, then an error occurs and
 * the function returns -1.
 *
 * \note
 * If you setup the file descriptor in non-blocking mode, then the
 * function will return early if it cannot cache or immediately
 * send the specified data.
 *
 * \param[in] buf  A pointer to a buffer of data to write to the file.
 * \param[in] count  The number of bytes to write to the file.
 *
 * \return The number of bytes written to the file.
 */
ssize_t fd_connection::write(void const * buf, size_t count)
{
    // WARNING: We MUST call the fd_connection version of the is_writer(),
    //          because the fd_buffer_connection::is_writer() also checks
    //          the f_output buffer which has unwanted side effects
    //
    if(!fd_connection::is_writer())
    {
        errno = EBADF;
        return -1;
    }

    return ::write(f_fd, buf, count);
}



} // namespace ed
// vim: ts=4 sw=4 et
