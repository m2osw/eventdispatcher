// Copyright (c) 2013-2023  Made to Order Software Corp.  All Rights Reserved
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
// 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA.

// self
//
#include    <cppprocess/io_capture_pipe.h>


// snaplogger
//
#include    <snaplogger/message.h>


// snapdev
//
#include    <snapdev/trim_string.h>


// C
//
#include    <string.h>


// last include
//
#include    <snapdev/poison.h>



namespace cppprocess
{



io_capture_pipe::io_capture_pipe()
    : io_pipe(IO_FLAG_OUTPUT)
{
}


void io_capture_pipe::process_read()
{
//std::cerr << "PIPE got read signal -- " << get_socket() << "\n";
    if(get_socket() != -1)
    {
        // handle up to 64Kb at once
        //
        char buffer[1'024 * 64];
        errno = 0;
        ssize_t const r(read(&buffer[0], sizeof(buffer)));
        if(r < 0
        && errno != 0
        && errno != EAGAIN
        && errno != EWOULDBLOCK)
        {
            int const e(errno);
            SNAP_LOG_ERROR
                << "an error occurred while reading from pipe (errno: "
                << e
                << " -- "
                << strerror(e)
                << ")."
                << SNAP_LOG_SEND;
            process_error();
            return;
        }

        if(r > 0)
        {
            // append to output buffer
            //
            f_output.insert(f_output.end(), buffer, buffer + r);

//std::cerr << "PIPE read " << r << " bytes ["
//<< std::string(reinterpret_cast<char *>(f_output.data()), f_output.size())
//<< "]\n";
        }
    }

    // process the next level
    //
    pipe_connection::process_read();
}


/** \brief Read the output of the command.
 *
 * This function reads the output of the process. This function converts
 * the output to UTF-8. Note that if some bytes are missing this function
 * is likely to fail. If you are reading the data little by little as it
 * comes in, you may want to use the get_binary_output() function
 * instead. That way you can detect characters such as the "\n" and at
 * that point convert the data from the previous "\n" you found in the
 * buffer to that new "\n". This will generate valid UTF-8 strings.
 *
 * This function is most often used by users of commands that process
 * one given input and generate one given output all at once.
 *
 * \param[in] reset  Whether the output so far should be cleared.
 *
 * \return The current output buffer.
 *
 * \sa get_binary_output()
 */
std::string io_capture_pipe::get_output(bool reset) const
{
    std::string const output(reinterpret_cast<char const *>(f_output.data()), f_output.size());
    if(reset)
    {
        const_cast<io_capture_pipe *>(this)->f_output.clear();
    }
    return output;
}


/** \brief Get the trimmed output.
 *
 * The get_output() function returns the output as is, with all the
 * characters. Quite often, though, you do not want the ending `'\n'`,
 * introductory spaces or tabs, or even multiple spaces when aligned
 * column output is used by the process.
 *
 * This function can be used to trimmed all of that junk for you. It
 * will always remove the starting and ending spaces and new line,
 * carriage return characters.
 *
 * When \p inside is true, it also replaces multiple spaces within
 * the string in a single space. This feature also replaces all spaces
 * with 0x20 (`' '`).
 *
 * \param[in] inside  Whether to remove double, triple, etc. spaces inside
 * the string.
 * \param[in] reset  Whether to reset the value after this call.
 *
 * \return The trimmed output string.
 *
 * \sa snapdev::trim_string() (in snapdev)
 */
std::string io_capture_pipe::get_trimmed_output(bool inside, bool reset) const
{
    return snapdev::trim_string(get_output(reset), true, true, inside);
}


/** \brief Read the output of the command as a binary buffer.
 *
 * This function reads the output of the process in binary (untouched).
 *
 * This function does not fail like the get_output() which attempts to
 * convert the output of the function to UTF-8. Also the output of the
 * command may not be UTF-8 in which case you would have to use the
 * binary version and use a different conversion.
 *
 * \param[in] reset  Whether the output so far should be cleared.
 *
 * \return The current output buffer.
 *
 * \sa get_output()
 */
buffer_t io_capture_pipe::get_binary_output(bool reset) const
{
    buffer_t const output(f_output);
    if(reset)
    {
        const_cast<io_capture_pipe *>(this)->f_output.clear();
    }
    return output;
}



} // namespace cppprocess
// vim: ts=4 sw=4 et
