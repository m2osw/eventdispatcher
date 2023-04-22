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
#include    "eventdispatcher/pipe_buffer_connection.h"

#include    "eventdispatcher/utils.h"


// snaplogger lib
//
#include    <snaplogger/message.h>


// C++ lib
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
 */
pipe_buffer_connection::pipe_buffer_connection()
    : pipe_connection(pipe_t::PIPE_BIDIRECTIONAL)
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
    return get_socket() != -1 && !f_output.empty();
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


/** \brief Read data that was received on this pipe.
 *
 * This function is used to read data whenever the process on
 * the other side sent us a message.
 */
void pipe_buffer_connection::process_read()
{
    if(get_socket() != -1)
    {
        // we could read one character at a time until we get a '\n'
        // but since we have a non-blocking socket we can read as
        // much as possible and then check for a '\n' and keep
        // any extra data in a cache.
        //
        int count_lines(0);
        int64_t const date_limit(get_current_date() + get_processing_time_limit());
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
                //
                break;
            }
            else //if(r < 0)
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
    }
    //else -- TBD: should we at least log an error when process_read() is called without a valid socket?

    // process the next level
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
    if(get_socket() != -1)
    {
        errno = 0;
        ssize_t const r(pipe_connection::write(&f_output[f_position], f_output.size() - f_position));
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
