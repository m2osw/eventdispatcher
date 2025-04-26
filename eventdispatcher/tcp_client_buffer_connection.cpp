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
#include    "eventdispatcher/tcp_client_buffer_connection.h"

#include    "eventdispatcher/utils.h"


// snaplogger
//
#include    <snaplogger/message.h>


// C++
//
#include    <algorithm>
#include    <cstring>


// last include
//
#include    <snapdev/poison.h>



namespace ed
{



/** \brief Initialize a client socket.
 *
 * The client socket gets initialized with the specified 'socket'
 * parameter.
 *
 * This constructor creates a writer connection too. This gives you
 * a read/write connection. You can get the writer with the writer()
 * function. So you may write data with:
 *
 * \code
 *      my_reader.writer().write(buf, buf_size);
 * \endcode
 *
 * \param[in] address  The address to connect to.
 * \param[in] mode  The mode to connect as (PLAIN or SECURE).
 * \param[in] blocking  If true, keep a blocking socket, other non-blocking.
 */
tcp_client_buffer_connection::tcp_client_buffer_connection(
              addr::addr const & address
            , mode_t const mode
            , bool const blocking)
    : tcp_client_connection(address, mode)
{
    if(!blocking)
    {
        non_blocking();
    }
}


/** \brief Check whether this connection still has some input in its buffer.
 *
 * This function returns true if there is partial incoming data in this
 * object's buffer.
 *
 * \return true if some buffered input is waiting for completion.
 */
bool tcp_client_buffer_connection::has_input() const
{
    return !f_line.empty();
}



/** \brief Check whether this connection still has some output in its buffer.
 *
 * This function returns true if there is still some output in the client
 * buffer. Output is added by the write() function, which is called by
 * the send_message() function.
 *
 * \return true if some buffered output is waiting to be sent out.
 */
bool tcp_client_buffer_connection::has_output() const
{
    return !f_output.empty();
}



/** \brief Write data to the connection.
 *
 * This function can be used to send data to this TCP/IP connection.
 * The data is bufferized and as soon as the connection can WRITE
 * to the socket, it will wake up and send the data. In other words,
 * we cannot just sleep and wait for an answer. The transfer will
 * be asynchronous.
 *
 * \note
 * On success, the function returns \p length whether part or all
 * of the data was sent through the socket or saved to the cached.
 *
 * \note
 * Note that the cache is always used if the socket was not marked
 * non-blocking. You can ensure so by calling the connection::non_blocking()
 * function once.
 *
 * \todo
 * Determine whether we may end up with really large buffers that
 * grow for a long time. This function only inserts and the
 * process_write() function only reads some of the bytes but it
 * does not reduce the size of the buffer until all the data was
 * sent.
 *
 * \param[in] data  The pointer to the buffer of data to be sent.
 * \param[out] length  The number of bytes to send.
 *
 * \return The number of bytes that were written or -1 on an error.
 */
ssize_t tcp_client_buffer_connection::write(void const * data, std::size_t length)
{
    if(!valid_socket())
    {
        errno = EBADF;
        return -1;
    }

    if(data != nullptr && length > 0)
    {
        char const * d(reinterpret_cast<char const *>(data));
        std::size_t l(length);

        if(f_output.empty()
        && is_non_blocking())
        {
            // it is non-blocking so we can attempt an immediate write()
            // to the socket, this way we may be able to avoid caching
            // anything
            //
            errno = 0;
            ssize_t const r(tcp_client_connection::write(d, l));
            if(r > 0)
            {
                l -= r;
                if(l == 0)
                {
                    // no buffer needed!
                    //
                    return length;
                }

                // could not write the entire buffer, cache the rest
                //
                d += r;
            }
            // TODO: handle error cases -- the process_write() will do that
            //       but we're going to cache the data, etc. which is a waste
        }

        f_output.insert(f_output.end(), d, d + l);
        return length;
    }

    return 0;
}


/** \brief The buffer is a writer when the output buffer is not empty.
 *
 * This function returns true as long as the output buffer of this
 * client connection is not empty.
 *
 * \return true if the output buffer is not empty, false otherwise.
 */
bool tcp_client_buffer_connection::is_writer() const
{
    return valid_socket() && has_output();
}


/** \brief Instantiation of process_read().
 *
 * This function reads incoming data from a socket.
 *
 * The function is what manages our low level TCP/IP connection protocol
 * which is to read one line of data (i.e. bytes up to the next '\\n'
 * character; note that '\\r' are not understood.)
 *
 * Once a complete line of data was read, it is converted to UTF-8 and
 * sent to the next layer using the process_line() function passing
 * the line it just read (without the '\\n') to that callback.
 *
 * \sa process_write()
 * \sa process_line()
 */
void tcp_client_buffer_connection::process_read()
{
    // we read one character at a time until we get a '\n'
    // since we have a non-blocking socket we can read as
    // much as possible and then check for a '\n' and keep
    // any extra data in a cache.
    //
    if(valid_socket())
    {
        int count_lines(0);
        std::int64_t const date_limit(get_current_date() + get_processing_time_limit());
        std::vector<char> buffer;
        buffer.resize(1024);
        for(;;)
        {
            errno = 0;
            ssize_t const r(read(&buffer[0], buffer.size()));
            if(r > 0)
            {
                for(ssize_t position(0); position < r; )
                {
                    std::vector<char>::const_iterator it(std::find(buffer.begin() + position, buffer.begin() + r, '\n'));
                    if(it == buffer.begin() + r)
                    {
                        // no newline, just add the whole thing
                        f_line += std::string(&buffer[position], r - position);
                        break; // do not waste time, we know we are done
                    }

                    // retrieve the characters up to the newline
                    // character and process the line
                    //
                    f_line += std::string(&buffer[position], it - buffer.begin() - position);
                    process_line(f_line);
                    ++count_lines;

                    // done with that line
                    //
                    f_line.clear();

                    // we had a newline, we may still have some data
                    // in that buffer; (+1 to skip the '\n' itself)
                    //
                    position = it - buffer.begin() + 1;
                }

                // when we reach here all the data read in `buffer` is
                // now either fully processed or in f_line
                //
                // TODO: change the way this works so we can test the
                //       limit after each process_line() call
                //
                if(count_lines >= get_event_limit()
                || get_current_date() >= date_limit)
                {
                    // we reach one or both limits, stop processing so
                    // the other events have a chance to run
                    //
                    break;
                }
            }
            else if(r == 0 || errno == 0 || errno == EAGAIN || errno == EWOULDBLOCK)
            {
                // no more data available at this time
                break;
            }
            else //if(r < 0)
            {
                // TODO: do something about the error
                int const e(errno);
                SNAP_LOG_ERROR
                    << "an error occurred while reading from socket (errno: "
                    << e
                    << " -- "
                    << strerror(e)
                    << ")."
                    << SNAP_LOG_SEND;
                process_error();
                return;
            }
        }
    }

    // process next level too
    tcp_client_connection::process_read();
}


/** \brief Instantiation of process_write().
 *
 * This function writes outgoing data to a socket.
 *
 * This function manages our own internal cache, which we use to allow
 * for out of synchronization (non-blocking) output.
 *
 * When the output buffer goes empty, this function calls the
 * process_empty_buffer() callback.
 *
 * \sa write()
 * \sa process_read()
 * \sa process_empty_buffer()
 */
void tcp_client_buffer_connection::process_write()
{
    if(valid_socket())
    {
        errno = 0;
        ssize_t const r(tcp_client_connection::write(&f_output[f_position], f_output.size() - f_position));
        if(r > 0)
        {
            // some data was written
            f_position += r;
            if(f_position >= f_output.size())
            {
                f_output.clear();
                f_position = 0;
                process_empty_buffer();
            }
        }
        else if(r < 0 && errno != 0 && errno != EAGAIN && errno != EWOULDBLOCK)
        {
            // connection is considered bad, generate an error
            //
            int const e(errno);
            SNAP_LOG_ERROR
                << "an error occurred while writing to socket of \""
                << get_name()
                << "\" (errno: "
                << e
                << " -- "
                << strerror(e)
                << ")."
                << SNAP_LOG_SEND;
            process_error();
            return;
        }
    }

    // process next level too
    //
    tcp_client_connection::process_write();
}


/** \brief The hang up event occurred.
 *
 * This function closes the socket and then calls the previous level
 * hang up code which removes this connection from the communicator
 * object it was last added in.
 */
void tcp_client_buffer_connection::process_hup()
{
    // this connection is dead...
    //
    close();

    // process next level too
    tcp_client_connection::process_hup();
}


/** \fn tcp_client_buffer_connection::process_line(std::string const & line);
 * \brief Process a line of data.
 *
 * This is the default virtual class that can be overridden to implement
 * your own processing. By default this function does nothing.
 *
 * \note
 * At this point I implemented this function so one can instantiate
 * a tcp_server_client_buffer_connection without having to
 * derive it, although I do not think that is 100% proper.
 *
 * \param[in] line  The line of data that was just read from the input
 *                  socket.
 */



} // namespace ed
// vim: ts=4 sw=4 et
