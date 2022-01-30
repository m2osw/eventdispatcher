// Copyright (c) 2013-2022  Made to Order Software Corp.  All Rights Reserved
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
#include    <cppprocess/io_data_pipe.h>


// last include
//
#include    <snapdev/poison.h>



namespace cppprocess
{



/** \brief Initialize the IO data pipe.
 *
 * This constructor initialize the pipe as an input pipe.
 *
 * Once created, call the add_input() to add data to the pipe.
 */
io_data_pipe::io_data_pipe()
    : io_pipe(IO_FLAG_INPUT)
{
}


/** \brief The input to be sent to stdin.
 *
 * Add the input data to be written to the stdin pipe. Note that the input
 * cannot be modified once the run() command was called unless the mode
 * is PROCESS_MODE_INOUT_INTERACTIVE.
 *
 * Note that in all case, calling this function multiple times adds more
 * data to the input. It does not erase what was added before. The thread
 * may eat some of the input in which case it gets removed from the internal
 * variable.
 *
 * \note
 * The function is safe and adding new input from the output thread
 * (which happens in interactive mode) is protected.
 *
 * \param[in] input  The input of the process (stdin).
 *
 * \sa add_input(buffer_t const & input)
 */
void io_data_pipe::add_input(std::string const & input)
{
    // this is additive!
    //
    add_input(buffer_t(input.begin(), input.end()));
}


/** \brief Binary data to be sent to stdin.
 *
 * When the input data is binary, use the QByteArray instead of a std::string
 * so you are sure it gets properly added.
 *
 * Calling this function multiple times appends the new data to the
 * existing data.
 *
 * Please, see the other set_input() function for additional information.
 *
 * \note
 * When sending a std::string, remember that these are converted to UTF-8
 * which is not compatible with purely binary data (i.e. UTF-8, for example,
 * does not allow for 0xFE and 0xFF.)
 *
 * \param[in] input  The input of the process (stdin).
 *
 * \sa add_input(std::string const & input)
 */
void io_data_pipe::add_input(buffer_t const & input)
{
    // this is additive!
    //
    f_input.insert(f_input.end(), input.begin(), input.end());
}


/** \brief Retrieve a copy of the input buffer as a string.
 *
 * This function returns the contents of the input buffer as a string.
 *
 * \return A copy of the input.
 *
 * \sa get_binary_input()
 */
std::string io_data_pipe::get_input(bool reset) const
{
    std::string input(reinterpret_cast<char const *>(f_input.data()), f_input.size());
    if(reset)
    {
        const_cast<io_data_pipe *>(this)->f_input.clear();
    }
    return input;
}


/** \brief Retrieve a copy of the input buffer.
 *
 * This function returns the contents of the input buffer. In most cases,
 * you'll never need this function (you should know what you add to your
 * command input).
 *
 * \return A copy of the input.
 *
 * \sa get_input()
 */
buffer_t io_data_pipe::get_binary_input(bool reset) const
{
    buffer_t const input(f_input);
    if(reset)
    {
        const_cast<io_data_pipe *>(this)->f_input.clear();
    }
    return input;
}


/** \brief Check whether the pipe is a writer or not.
 *
 * The pipe is a writer as long as it includes input data. Once the pipe
 * is empty, the done function will be called (after that happens, you
 * can't add data anymore).
 *
 * \return true until all the input data was sent to the process.
 */
bool io_data_pipe::is_writer() const
{
    return f_pos < f_input.size();
}


/** \brief Send some data to the process.
 *
 * This function writes the input data to the input pipe of the process.
 * The function writes as much data as posssible (i.e. as large as the
 * FIFO buffer supports) then returns.
 *
 * The function will be called as long as data can be pushed to the FIFO.
 * The communicator will know once there is more space available so this
 * function can write to the FIFO.
 */
void io_data_pipe::process_write()
{
    std::size_t const size(f_input.size() - f_pos);
    if(size > 0)
    {
        ssize_t const r(write(f_input.data() + f_pos, size));
        if(r > 0)
        {
            f_pos += r;

            if(f_pos >= f_input.size())
            {
                process_done(done_reason_t::DONE_REASON_EOF);
            }
        }
    }
}



} // namespace cppprocess
// vim: ts=4 sw=4 et
