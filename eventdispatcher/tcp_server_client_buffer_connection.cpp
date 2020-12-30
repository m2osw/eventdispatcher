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
#include    "eventdispatcher/tcp_server_client_buffer_connection.h"

#include    "eventdispatcher/utils.h"


// snaplogger lib
//
#include    <snaplogger/message.h>


// C++ lib
//
#include    <algorithm>


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
 * If you are a pure client (opposed to a client that was just accepted)
 * you may want to consider using the tcp_client_buffer_connection
 * instead. That gives you a way to open the socket from a set of address
 * and port definitions among other things.
 *
 * This initialization, so things work as expected in our environment,
 * the function marks the socket as non-blocking. This is important for
 * the reader and writer capabilities.
 *
 * \param[in] client  The client to be used for reading and writing.
 */
tcp_server_client_buffer_connection::tcp_server_client_buffer_connection(tcp_bio_client::pointer_t client)
    : tcp_server_client_connection(client)
{
    non_blocking();
}


/** \brief Check whether this connection still has some input in its buffer.
 *
 * This function returns true if there is partial incoming data in this
 * object's buffer.
 *
 * \return true if some buffered input is waiting for completion.
 */
bool tcp_server_client_buffer_connection::has_input() const
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
bool tcp_server_client_buffer_connection::has_output() const
{
    return !f_output.empty();
}



/** \brief Tells that this connection is a writer when we have data to write.
 *
 * This function checks to know whether there is data to be writen to
 * this connection socket. If so then the function returns true. Otherwise
 * it just returns false.
 *
 * This happens whenever you called the write() function and our cache
 * is not empty yet.
 *
 * \return true if there is data to write to the socket, false otherwise.
 */
bool tcp_server_client_buffer_connection::is_writer() const
{
    return get_socket() != -1 && !f_output.empty();
}


/** \brief Write data to the connection.
 *
 * This function can be used to send data to this TCP/IP connection.
 * The data is bufferized and as soon as the connection can WRITE
 * to the socket, it will wake up and send the data. In other words,
 * we cannot just sleep and wait for an answer. The transfer will
 * be asynchroneous.
 *
 * \todo
 * Determine whether we may end up with really large buffers that
 * grow for a long time. This function only inserts and the
 * process_signal() function only reads some of the bytes but it
 * does not reduce the size of the buffer until all the data was
 * sent.
 *
 * \param[in] data  The pointer to the buffer of data to be sent.
 * \param[out] length  The number of bytes to send.
 */
ssize_t tcp_server_client_buffer_connection::write(void const * data, size_t const length)
{
    if(get_socket() == -1)
    {
        errno = EBADF;
        return -1;
    }

    if(data != nullptr && length > 0)
    {
        char const * d(reinterpret_cast<char const *>(data));
        f_output.insert(f_output.end(), d, d + length);
        return length;
    }

    return 0;
}


/** \brief Read and process as much data as possible.
 *
 * This function reads as much incoming data as possible and processes
 * it.
 *
 * If the input includes a newline character ('\n') then this function
 * calls the process_line() callback which can further process that
 * line of data.
 *
 * \todo
 * Look into a way, if possible, to have a single instantiation since
 * as far as I know this code matches the one written in the
 * process_read() of the tcp_client_buffer_connection and
 * the pipe_buffer_connection classes.
 */
void tcp_server_client_buffer_connection::process_read()
{
    // we read one character at a time until we get a '\n'
    // since we have a non-blocking socket we can read as
    // much as possible and then check for a '\n' and keep
    // any extra data in a cache.
    //
    if(get_socket() != -1)
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
                int const e(errno);
                SNAP_LOG_WARNING
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
    tcp_server_client_connection::process_read();
}


/** \brief Write to the connection's socket.
 *
 * This function implementation writes as much data as possible to the
 * connection's socket.
 *
 * This function calls the process_empty_buffer() callback whenever the
 * output buffer goes empty.
 */
void tcp_server_client_buffer_connection::process_write()
{
    if(get_socket() != -1)
    {
        errno = 0;
        ssize_t const r(tcp_server_client_connection::write(&f_output[f_position], f_output.size() - f_position));
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
        else if(r != 0 && errno != 0 && errno != EAGAIN && errno != EWOULDBLOCK)
        {
            // connection is considered bad, get rid of it
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
    tcp_server_client_connection::process_write();
}


/** \brief The remote hanged up.
 *
 * This function makes sure that the local connection gets closed properly.
 */
void tcp_server_client_buffer_connection::process_hup()
{
    // this connection is dead...
    //
    close();

    tcp_server_client_connection::process_hup();
}



} // namespace ed
// vim: ts=4 sw=4 et
