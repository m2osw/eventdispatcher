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
#include    "eventdispatcher/pipe_buffer_connection.h"

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



/** \brief Create a pipe with a buffer.
 *
 * This function creates a pipe with a buffer which can then be used to
 * manage lines of data. This version of the pipe forcibly uses the
 * bidirectional pipe. This is so because the buffered and message
 * classes are expected to be able to send and receive. The input
 * or ouptut only pipes are expected to be used as replacements to the
 * stdin, stdout, and stderr streams which are unidirectional.
 *
 * \param[in] input_buffer_size  Size of the memory input buffer in bytes.
 */
pipe_buffer_connection::pipe_buffer_connection(std::size_t input_buffer_size)
    : pipe_connection(pipe_t::PIPE_BIDIRECTIONAL)
    , f_input(input_buffer_size)
{
}


/** \brief Pipe connections accept writes.
 *
 * This function returns true when there is some data in the pipe
 * connection buffer meaning that the pipe connection needs to
 * send data to the other side of the pipe.
 *
 * \return true if some data has to be written to the pipe.
 */
bool pipe_buffer_connection::is_writer() const
{
    return valid_socket() && !f_output.empty();
}


void pipe_buffer_connection::close()
{
    pipe_connection::close();

    f_input.clear();
    f_input.shrink_to_fit();
    f_output.clear();
    f_output.shrink_to_fit();
    f_line.clear();
    f_line.shrink_to_fit();
}


/** \brief Write the specified data to the pipe buffer.
 *
 * This function writes the data specified by \p data to the pipe buffer.
 * Note that the data is not sent immediately. This will only happen
 * when the Snap Communicator loop is re-entered.
 *
 * \param[in] data  The pointer to the data to write to the pipe.
 * \param[in] length  The size of the data buffer.
 *
 * \return The number of bytes written. The function returns 0 when no
 *         data can be written to that connection (i.e. it was already
 *         closed or data is a null pointer.)
 */
ssize_t pipe_buffer_connection::write(void const * data, size_t length)
{
    if(!valid_socket())
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

    if(f_output.empty())
    {
        process_empty_buffer();
    }

    return 0;
}


/** \brief Read data that was received on this pipe.
 *
 * This function is used to read data whenever the process on
 * the other side sent us a message.
 */
void pipe_buffer_connection::process_read()
{
    if(valid_socket())
    {
        // we could read one character at a time until we get a '\n'
        // but since we have a non-blocking socket we can read as
        // much as possible and then check for a '\n' and keep
        // any extra data in a cache.
        //
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
                // this happens all the time (i.e. another process quits)
                // so we make it a debug and not a warning or an error...
                //
                int const e(errno);
                SNAP_LOG_DEBUG
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

    pipe_connection::process_read();
}


/** \brief Write as much data as we can to the pipe.
 *
 * This function writes the data that was cached in our f_output
 * buffer to the pipe, as much as possible, then it returns.
 *
 * The is_writer() function takes care of returning true if more
 * data is present in the f_output buffer and thus the process_write()
 * needs to be called again.
 *
 * Once the write buffer goes empty, this function calls the
 * process_empty_buffer() callback.
 */
void pipe_buffer_connection::process_write()
{
    if(valid_socket())
    {
        errno = 0;
        ssize_t const r(pipe_connection::write(&f_output[f_output_position], f_output.size() - f_output_position));
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
    //else -- TBD: should we generate an error when the socket is not valid?

    // process next level too
    pipe_connection::process_write();
}


/** \brief The process received a hanged up pipe.
 *
 * The pipe on the other end was closed, somehow.
 */
void pipe_buffer_connection::process_hup()
{
    close();
    pipe_connection::process_hup();
}



} // namespace ed
// vim: ts=4 sw=4 et
