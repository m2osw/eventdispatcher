// Copyright (c) 2012-2026  Made to Order Software Corp.  All Rights Reserved
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
#include    "eventdispatcher/tcp_server_client_buffer_connection.h"

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
 * \param[in] input_buffer_size  Size of the memory input buffer in bytes.
 */
tcp_server_client_buffer_connection::tcp_server_client_buffer_connection(
          tcp_bio_client::pointer_t client
        , std::size_t input_buffer_size)
    : tcp_server_client_connection(client)
    , f_input(input_buffer_size)
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
 * This function checks to know whether there is data to be written to
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
    return valid_socket() && !f_output.empty();
}


/** \brief Write data to the connection.
 *
 * This function can be used to send data to this TCP/IP connection.
 * The data is bufferized and as soon as the connection can WRITE
 * to the socket, it will wake up and send the data. In other words,
 * we cannot just sleep and wait for an answer. The transfer will
 * be asynchronous.
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
 *
 * \return The number of bytes written or -1 if the socket is already closed.
 */
ssize_t tcp_server_client_buffer_connection::write(void const * data, size_t const length)
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
            ssize_t const r(tcp_server_client_connection::write(d, l));
            if(r > 0)
            {
                l -= r;
                if(l == 0)
                {
                    // no buffer needed!
                    //
                    process_empty_buffer();
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

    if(f_output.empty())
    {
        process_empty_buffer();
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
 * the pipe_buffer_connection classes. (See TODO.txt file
 * about templates.)
 *
 * \sa process_write()
 * \sa process_line()
 */
void tcp_server_client_buffer_connection::process_read()
{
    if(valid_socket())
    {
        int count_lines(0);
        std::int64_t const date_limit(get_current_date() + get_processing_time_limit());
        bool more(true);
        do
        {
            errno = 0;
            if(f_input_position >= f_input_size)
            {
                f_input_size = read(f_input.data(), f_input.size());
                f_input_position = 0;
            }
            if(f_input_size > 0)
            {
                while(f_input_position < f_input_size)
                {
                    std::vector<char>::const_iterator it(std::find(f_input.begin() + f_input_position, f_input.begin() + f_input_size, '\n'));
                    if(it == f_input.begin() + f_input_size)
                    {
                        // no newline, just add the whole thing and wait
                        // for more data
                        //
                        f_line += std::string(f_input.data() + f_input_position, f_input_size - f_input_position);
                        f_input_size = 0;
                        f_input_position = 0;
                        break;
                    }

                    // retrieve the characters up to the newline
                    // character and process the line
                    //
                    f_line += std::string(f_input.data() + f_input_position, it - f_input.begin() - f_input_position);
                    process_line(f_line);
                    ++count_lines;

                    // done with that line
                    //
                    f_line.clear();

                    // we had a newline, we may still have some data
                    // in that buffer; (+1 to skip the '\n' itself)
                    //
                    f_input_position = it - f_input.begin() + 1;

                    if(count_lines >= get_event_limit()
                    || get_current_date() >= date_limit)
                    {
                        // we reach one or both limits, stop processing so
                        // the other events have a chance to run
                        //
                        if(f_input_position < f_input_size)
                        {
                            add_yield(YIELD_READ);
                        }
                        more = false;
                        break;
                    }
                }
            }
            else if(f_input_size == 0 || errno == 0 || errno == EAGAIN || errno == EWOULDBLOCK)
            {
                // no more data available at this time
                //
                break;
            }
            else //if(f_input_size < 0)
            {
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
        while(more);
    }

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
    if(valid_socket())
    {
        errno = 0;
        ssize_t const r(tcp_server_client_connection::write(&f_output[f_output_position], f_output.size() - f_output_position));
        if(r > 0)
        {
            // some data was written
            f_output_position += r;
            if(f_output_position >= f_output.size())
            {
                f_output.clear();
                f_output_position = 0;
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

    tcp_server_client_connection::process_write();
}


/** \brief The remote hanged up.
 *
 * This function makes sure that the local connection gets closed properly.
 */
void tcp_server_client_buffer_connection::process_hup()
{
    close();
    tcp_server_client_connection::process_hup();
}


/** \brief Reset buffers once they are not necessary anymore.
 *
 * This function clears the buffers so memory is saved immediately instead
 * of when the connection gets deleted.
 */
void tcp_server_client_buffer_connection::close()
{
    tcp_server_client_connection::close();

    f_input.clear();
    f_input.shrink_to_fit();
    f_output.clear();
    f_output.shrink_to_fit();
    f_line.clear();
    f_line.shrink_to_fit();
}



} // namespace ed
// vim: ts=4 sw=4 et
