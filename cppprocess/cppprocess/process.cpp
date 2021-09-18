// Copyright (c) 2013-2021  Made to Order Software Corp.  All Rights Reserved
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
#include    "cppprocess/process.h"

#include    "cppprocess/exception.h"


// eventdispatcher lib
//


// snaplogger lib
//
#include    <snaplogger/message.h>


// snapdev lib
//
#include    <snapdev/glob_to_list.h>
#include    <snapdev/join_strings.h>
#include    <snapdev/trim_string.h>
#include    <snapdev/not_reached.h>


// C lib
//
#include    <string.h>
#include    <signal.h>


// last include
//
#include    <snapdev/poison.h>





extern char ** environ;

namespace cppprocess
{



namespace
{




class direct_input_data
    : public ed::pipe_connection
{
public:
                            direct_input_data(process * p, buffer_t const & input);
                            direct_input_data(direct_input_data const &) = delete;
    direct_input_data &     operator = (direct_input_data const &) = delete;

    // pipe_connection implementation
    //
    virtual bool            is_writer() const override;
    virtual void            process_write() override;

    virtual void            process_error() override;
    virtual void            process_invalid() override;
    virtual void            process_hup() override;

private:
    process *               f_process = nullptr;
    buffer_t const &        f_input;
    std::size_t             f_pos = 0;
};


direct_input_data::direct_input_data(process * p, buffer_t const & input)
    : pipe_connection(ed::pipe_t::PIPE_CHILD_INPUT)
    , f_process(p)
    , f_input(input)
{
    set_name("direct_input_data");
}


bool direct_input_data::is_writer() const
{
    return f_pos < f_input.size();
}


void direct_input_data::process_write()
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
                f_process->input_pipe_done();
            }
        }
    }
}


void direct_input_data::process_error()
{
    f_process->input_pipe_done();
}


void direct_input_data::process_invalid()
{
    f_process->input_pipe_done();
}


void direct_input_data::process_hup()
{
    f_process->input_pipe_done();
}









/** \brief A direct output to input pipe.
 *
 * When piping one command to another, then this pipe object gets used.
 * This pipe directly sends the output of the previous command to the
 * input of the next command.
 *
 * Internally, we may also use the tee_pipe which sends the output of
 * the previous command to the input of all the next commands.
 */
class direct_output_to_input_pipe
    : public ed::pipe_connection
{
public:
                    direct_output_to_input_pipe();

    // pipe_connection implementation
    //
    virtual void                forked() override;

private:
};


direct_output_to_input_pipe::direct_output_to_input_pipe()
    : pipe_connection(ed::pipe_t::PIPE_CHILD_OUTPUT)
{
    set_name("direct_output_to_input_pipe");
}


void direct_output_to_input_pipe::forked()
{
    // force a full close in this case
    //
    close();
}







class capture_output_pipe
    : public ed::pipe_connection
{
public:
                                capture_output_pipe(process * p, buffer_t & output);
                                capture_output_pipe(capture_output_pipe const &) = delete;
    capture_output_pipe &       operator = (capture_output_pipe const &) = delete;

    // pipe_connection implementation
    //
    virtual void                process_read() override;

    virtual void                process_error() override;
    virtual void                process_invalid() override;
    virtual void                process_hup() override;

private:
    process *                   f_process = nullptr;
    buffer_t &                  f_output;
};


capture_output_pipe::capture_output_pipe(process * p, buffer_t & output)
    : pipe_connection(ed::pipe_t::PIPE_CHILD_OUTPUT)
    , f_process(p)
    , f_output(output)
{
    set_name("capture_output_pipe");
}


void capture_output_pipe::process_read()
{
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
        }
    }

    // process the next level
    //
    pipe_connection::process_read();
}


void capture_output_pipe::process_error()
{
    f_process->output_pipe_done(this);
}


void capture_output_pipe::process_invalid()
{
    f_process->output_pipe_done(this);
}


void capture_output_pipe::process_hup()
{
    f_process->output_pipe_done(this);
}







class buffered_pipe
    : public ed::pipe_connection
{
public:
                                buffered_pipe();

    // connection
    virtual bool                is_writer() const override;

    // pipe implementation
    virtual ssize_t             write(void const * data, size_t length) override;
    virtual void                process_write() override;
    virtual void                process_hup() override;

private:
    std::vector<char>           f_output = std::vector<char>();
    size_t                      f_position = 0;
};


buffered_pipe::buffered_pipe()
    : pipe_connection(ed::pipe_t::PIPE_CHILD_OUTPUT)
{
    set_name("buffered_pipe");
}


bool buffered_pipe::is_writer() const
{
    return get_socket() != -1 && !f_output.empty();
}


ssize_t buffered_pipe::write(void const * data, size_t length)
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


void buffered_pipe::process_write()
{
    if(get_socket() != -1)
    {
        errno = 0;
        ssize_t const r(pipe_connection::write(&f_output[f_position], f_output.size() - f_position));
        if(r > 0)
        {
            // some data was written
            //
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
    //
    pipe_connection::process_write();
}


void buffered_pipe::process_hup()
{
    close();

    pipe_connection::process_hup();
}







class tee_pipe
    : public ed::pipe_connection
{
public:
                                    tee_pipe(std::size_t const size);

    ed::pipe_connection::pointer_t  get_output_pipe(std::size_t idx);

    // pipe_connection
    //
    virtual void                    process_read() override;
    virtual void                    connection_added() override;
    virtual void                    connection_removed() override;

private:
    ed::communicator::pointer_t     f_communicator = ed::communicator::pointer_t();
    ed::pipe_connection::vector_t   f_output = ed::pipe_connection::vector_t();
};


tee_pipe::tee_pipe(std::size_t const size)
    : pipe_connection(ed::pipe_t::PIPE_CHILD_INPUT)
    , f_communicator(ed::communicator::instance())
    , f_output(size)
{
    set_name("tee_pipe");

    if(size == 0)
    {
        throw cppprocess_logic_error("tee_pipe constructor called with a size of zero is not supported");
    }

    for(std::size_t idx(0); idx < size; ++idx)
    {
        f_output[idx] = std::make_shared<buffered_pipe>();
    }
}


ed::pipe_connection::pointer_t tee_pipe::get_output_pipe(std::size_t idx)
{
    if(idx >= f_output.size())
    {
        throw cppprocess_out_of_range(
              "get_output_pipe() called with index "
            + std::to_string(idx)
            + ", which is out of allowed range: [0.."
            + std::to_string(f_output.size())
            + ").");
    }

    return f_output[idx];
}


void tee_pipe::process_read()
{
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

        if(r > 0)
        {
            // this is the T functionality, where we duplicate the data in
            // the input of each of the succeeded processes
            //
            for(auto & out : f_output)
            {
                out->write(&buffer[0], r);
            }
        }
    }

    // process the next level
    //
    pipe_connection::process_read();
}


void tee_pipe::connection_added()
{
    for(auto & out : f_output)
    {
        f_communicator->add_connection(out);
    }
}


void tee_pipe::connection_removed()
{
    for(auto & out : f_output)
    {
        f_communicator->remove_connection(out);
    }
}



} // no name namespace




/** \class process
 * \brief A process class to run a process and get information about the results.
 *
 * This class is used to run processes. Especially, it can run with in and
 * out capabilities (i.e. piping) although this is generally not recommanded
 * because piping can block (if you do not send enough data, or do not read
 * enough data, then the pipes can get stuck.) We use a thread to read the
 * results. We do not currently expect that the use of this class will require
 * the input read to be necessary to know what needs to be written (i.e. in
 * most cases all we want is to convert a file [input] from one format to
 * another [output] avoiding reading/writing on disk.)
 *
 * The whole process, when using the interactive mode, is quite complicated
 * so I wrote the following diagram. As you can see, the loop of sending
 * and receiving data from the child process is fairly simple. Note that the
 * callback is called from the Output Thread, not the main process. This does
 * not make much of a difference because no other function can be running on
 * the main process when that happens. The output is blocked and thus the
 * output variable is safe. The input is not blocked but adding input was
 * made safe internally.
 *
 * \msc
 * hscale = "2";
 * a [label="Function"],b [label="Process"],c [label="Input Thread"],d [label="Output Thread"],e [label="Child Process"];
 *
 * a=>b [label="run()"];
 * b=>e [label="fork()"];
 * e->e [label="execvpe()"];
 * b=>c [label="pthread_create()"];
 * b=>d [label="pthread_create()"];
 * b=>e [label="wait() child's death"];
 *
 * --- [label="start repeat"];
 * c->e [label="write() (Input Data)"];
 * d<-e [label="read() (Output Data)"];
 * b<:d [label="output shared"];
 * a<<=d [label="output callback"];
 * a=>b [label="set_input()"];
 * b=>c [label="add input"];
 * --- [label="end repeat"];
 *
 * b<-e [label="child died"];
 * b->c [label="stop()"];
 * b<-c [label="stopped"];
 * b->d [label="stop()"];
 * b<-d [label="stopped"];
 * a<\<b [label="run()"];
 * \endmsc
 *
 * Using the add_next_process function, it is possible to pipe the output
 * of one process as the input of the next process. In other words, this
 * class allows you to pipe any number of processes between each other.
 *
 * \code
 *     cppprocess:process a;
 *     cppprocess:process b;
 *     cppprocess:process c;
 *
 *     a.set_next_process(b);
 *     b.set_next_process(c);
 *
 *     a.start();       // runs `a | b | c`
 * \endcode
 *
 * When piping processes in this way, pipes are used and the more powerful
 * scheme is used (fork() + execvpe() instead of a simpler popen() or even
 * a system() call).
 *
 * Also, when piping processes, you can only add one input pipe to the
 * very first process and one output pipe to the very last process. You
 * can, however, add one error pipe to each process (i.e. so you can know
 * which process output such and such errors).
 *
 * When piping processes, you only call the start() function of
 * the first process. That will have the side effect of starting all the
 * following processes as expected.
 *
 * The feature includes a "tee" feature, which allows you to pipe the
 * output of one process as the input of any number of processes. This
 * is done by adding multiple processes as the next process of one
 * proces.
 *
 * \code
 *     cppprocess:process a;
 *     cppprocess:process b;
 *     cppprocess:process c;
 *     cppprocess:process c;
 *
 *     a.set_next_process(b);
 *     a.set_next_process(c);
 *     a.set_next_process(d);
 *
 *     a.start();
 *
 *     // equivalent to:
 *     //    a > data.tmp
 *     //    b < data.tmp
 *     //    c < data.tmp
 *     //    d < data.tmp
 *     // only we avoid the temporary file and b, c, d are run in parallel
 *     // note that b, c, d could be the same process with different args
 * \endcode
 */


/** \brief Initialize the process object.
 *
 * This function saves the name of the process. The name is generally a
 * static string and it is used to distinguish between processes when
 * managing several at once. The function makes a copy of the name.
 *
 * \note
 * The name of the process is not the command. See set_command().
 *
 * \param[in] name  The name of the process.
 *
 * \sa set_command()
 * \sa get_name()
 */
process::process(std::string const & name)
    : f_communicator(ed::communicator::instance())
    , f_name(name)
{
}


/** \brief Retrieve the name of this process object.
 *
 * This process object is given a name on creation. In most cases this is
 * a static name that is used to determine which process is which.
 *
 * \return The name of the process.
 */
std::string const & process::get_name() const
{
    return f_name;
}


/** \brief Set how the environment variables are defined in the process.
 *
 * By default all of the current process environment variables are
 * passed to the child process. If the child process is not 100% trustworthy,
 * it may be preferable to only pass a specific set of environment variables
 * (as added by the add_environ() function) to the child process.
 *
 * This function sets a flag to determine whether we want to force the
 * environment (true) to the list of variables added with the
 * add_environ() function or let our current process variables
 * flow through (false, the default).
 *
 * \param[in] forced  Whether the environment will be forced.
 *
 * \sa add_environ()
 * \sa get_forced_environment()
 */
void process::set_forced_environment(bool forced)
{
    f_forced_environment = forced;
}


/** \brief Check the current status of the forced environment flag.
 *
 * This function returns a copy of the forced environment flag. If true,
 * then the run() function will force the environment variables as
 * defined by add_environ() function instead of all the environment
 * variables of the calling process.
 *
 * \return The status of the forced environment flag.
 *
 * \sa add_environ()
 * \sa set_forced_environment()
 */
bool process::get_forced_environment() const
{
    return f_forced_environment;
}


/** \brief Define the command to run.
 *
 * The command name may be a full path or just the command filename.
 * (i.e. the `execvp()` function makes use of the PATH variable to find
 * the command on disk unless the \p command parameter includes a
 * slash character.)
 *
 * \warning
 * Do not add any arguments here. Instead, make sure to use the
 * add_argument() to add any number of arguments.
 *
 * If the process cannot be found, an error is generated at the time you
 * call the run() function.
 *
 * \param[in] command  The command to start the new process.
 *
 * \sa add_argument()
 * \sa get_command()
 */
void process::set_command(std::string const & command)
{
    f_command = command;
}


/** \brief Retrieve the command name & path.
 *
 * This function returns the command as set with the set_command() function.
 *
 * If the set_command() was never called, then the command is the
 * empty string at this point.
 *
 * \return The command name and path (the path is optional).
 *
 * \sa set_command()
 */
std::string const & process::get_command() const
{
    return f_command;
}


/** \brief Add an argument to the command line.
 *
 * This function adds one individual arguement to the command line.
 *
 * You have to add all the arguments in the right order.
 *
 * If you set the \p expand flag to true, then the function transforms the
 * argument into a list of file names and add those instead. If the
 * argument does not match any filename, then it is added as is.
 *
 * If the expansion fails, then the function prints out an error message
 * in the log and it returns false.
 *
 * \param[in] arg  The argument to be added.
 * \param[in] expand  Whether this argument includes glob characters to expand.
 *
 * \return true if the argument was added without issue.
 *
 * \sa get_arguments()
 */
bool process::add_argument(std::string const & arg, bool expand)
{
    if(!expand)
    {
        f_arguments.push_back(arg);
        return true;
    }

    if(!f_arguments.read_path<
              snap::glob_to_list_flag_t::GLOB_FLAG_BRACE
            , snap::glob_to_list_flag_t::GLOB_FLAG_PERIOD
            , snap::glob_to_list_flag_t::GLOB_FLAG_TILDE>(arg))
    {
        SNAP_LOG_ERROR
            << "an error occurred reading argument filenames from pattern \""
            << arg
            << "\": "
            << f_arguments.get_last_error_message()
            << " (errno: "
            << f_arguments.get_last_error_errno()
            << ", "
            << strerror(f_arguments.get_last_error_errno())
            << ")."
            << SNAP_LOG_SEND;
        return false;
    }

    return true;
}


/** \brief Return the current list of updatable arguments.
 *
 * This function returns a non-constant reference to the list of arguments
 * currently available in this process.
 *
 * This gives you the ability to go through the list and make changes.
 * This is often used when you want to run the same command with different
 * parameters (i.e. maybe a filename that changes between runs).
 *
 * It is important to keep in mind that if you expanded an argument, the
 * list may now include from 0 to any number of arguments as a replacement
 * to that one expanded argument.
 *
 * \return The list of arguments attached to this process.
 *
 * \sa add_argument()
 */
process::argument_list_t & process::get_arguments()
{
    return f_arguments;
}


/** \brief Return the current list of arguments.
 *
 * This function returns a constant reference to the list of arguments
 * currently available in this process.
 *
 * The list is grown by calling the add_argument() function. It can be
 * edited by calling the non-constant get_argument() and keeping a
 * reference to the list.
 *
 * If you asked for some arguments to be expanded, then they will be
 * in the expanded state (you will have lost the original pattern).
 *
 * \return The list of arguments attached to this process.
 *
 * \sa add_argument()
 */
process::argument_list_t const & process::get_arguments() const
{
    return f_arguments;
}


/** \brief Add an environment to the command line.
 *
 * This function adds a new environment variable for the child process to
 * use. In most cases this function doesn't get used.
 *
 * By default all the parent process (this current process) environment
 * variables are passed down to the child process. To avoid this behavior,
 * call the set_forced_environment() function before the run() function.
 *
 * An environment variable is defined as a name and a value as in:
 *
 * \code
 * add_environ("HOME", "/home/cppthread");
 * \endcode
 *
 * If the value is set to the empty string, then the environment variable
 * is removed from the list.
 *
 * \param[in] name  The name of the environment variable to add.
 * \param[in] value  The new value of that environment variable.
 *
 * \sa get_environ()
 */
void process::add_environ(std::string const & name, std::string const & value)
{
    if(value.empty())
    {
        auto it(f_environment.find(name));
        if(it != f_environment.end())
        {
            f_environment.erase(it);
        }
    }
    else
    {
        f_environment[name] = value;
    }
}


/** \brief Get a reference to the current environment.
 *
 * This parameter is generally empty since the run() function will use the
 * calling process environment variables.
 *
 * It is possible, however, to hide the calling process environment and
 * use these variables instead. This is very good for all sorts of safety
 * reasons (i.e. not leak a secret key saved in a variable, for example).
 *
 * \return A constant reference to the map of environment parameters.
 *
 * \sa add_environ()
 */
process::environment_map_t const & process::get_environ() const
{
    return f_environment;
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
void process::add_input(std::string const & input)
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
void process::add_input(buffer_t const & input)
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
std::string process::get_input(bool reset) const
{
    std::string input(reinterpret_cast<char const *>(f_input.data()), f_input.size());
    if(reset)
    {
        const_cast<process *>(this)->f_input.clear();
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
buffer_t process::get_binary_input(bool reset) const
{
    buffer_t const input(f_input);
    if(reset)
    {
        const_cast<process *>(this)->f_input.clear();
    }
    return input;
}


/** \brief Setup a pipe to send the child input to.
 *
 * This function is used to setup a pipe to replace stdin of the process.
 * You are expected to send data to that pipe and the child is expected
 * to read from this pipe and process the data as expected.
 *
 * \warning
 * Only the very first process in a list of processes can be assigned
 * an input pipe. If other processes are given an input pipe, then
 * the start() function will throw an error.
 *
 * \param[in] pipe  The pipe used to send data to the child process.
 *
 * \sa get_input_pipe()
 */
void process::set_input_pipe(ed::pipe_connection::pointer_t pipe)
{
    if(pipe->type() != ed::pipe_t::PIPE_CHILD_INPUT)
    {
        throw cppprocess_incorrect_pipe_type("incorrect pipe type, expected a PIPE_CHILD_INPUT type of pipe for the input.");
    }

    f_input_pipe = pipe;
}


/** \brief Retrieve the child input pipe.
 *
 * This function returns a pointer to the input pipe. This pipe can be used
 * to send data to the child process while it runs. It will send the data
 * to the child's stdin stream.
 *
 * This pipe works with the eventdispatcher communicator so it is non-blocking
 * and you can add data as space becomes available in the pipe without getting
 * stuck doing so. This means you can also handle the output and error pipes
 * as they get filled up.
 *
 * \return A pointer to the pipe connection used for the process input.
 *
 * \sa set_input_pipe()
 */
ed::pipe_connection::pointer_t process::get_input_pipe() const
{
    return f_input_pipe;
}


/** \brief Setup capture of output.
 *
 * By default, the output is set to stdout or an output pipe. You can also
 * request that the process class capture the output to a buffer by calling
 * this function with true.
 *
 * If you setup an output pipe, this flag is ignored.
 *
 * \param[in] capture  Whether to capture the output (true) or not (false).
 */
void process::set_capture_output(bool capture)
{
    f_capture_output = capture;
}


/** \brief Check whether the output will be captured or not.
 *
 * This function returns the capture flag.
 *
 * When the capture flag is true, the process output will be captured in an
 * internal buffer.
 *
 * \return true if the output is to be captured by the process object.
 */
bool process::get_capture_output() const
{
    return f_capture_output;
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
std::string process::get_output(bool reset) const
{
    std::string const output(reinterpret_cast<char const *>(f_output.data()), f_output.size());
    if(reset)
    {
        const_cast<process *>(this)->f_output.clear();
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
 * \sa snap::trim_string() (in snapdev)
 */
std::string process::get_trimmed_output(bool inside, bool reset) const
{
    return snap::trim_string(get_output(reset), true, true, inside);
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
buffer_t process::get_binary_output(bool reset) const
{
    buffer_t const output(f_output);
    if(reset)
    {
        const_cast<process *>(this)->f_output.clear();
    }
    return output;
}


/** \brief Setup a pipe where the process output is to be written.
 *
 * This function is used to setup a pipe which will receive data from
 * the stdout of the command being started by the process object.
 *
 * The pipe is going to be setup as non-blocking, which means you can
 * attempt a very large read (pipes on Linux are generally supporting
 * about 64Kb of data) and your process won't be blocked if the amount
 * of data is smaller that what you requested.
 *
 * \warning
 * Only the very last process or a list of piped processes can be
 * assigned an output pipe.
 *
 * \param[in] pipe  The pipe to attach to this process output.
 *
 * \sa get_output_pipe()
 */
void process::set_output_pipe(ed::pipe_connection::pointer_t pipe)
{
    if(pipe->type() != ed::pipe_t::PIPE_CHILD_OUTPUT)
    {
        throw cppprocess_incorrect_pipe_type("incorrect pipe type, expected a PIPE_CHILD_OUTPUT type of pipe for the output.");
    }

    f_output_pipe = pipe;
}


/** \brief Retrieve the pointer to the output pipe.
 *
 * This function retrieves the pointer of the current output pipe.
 * By default, this is a null pointer.
 *
 * \return The callback interface pointer.
 *
 * \sa set_output_pipe()
 */
ed::pipe_connection::pointer_t process::get_output_pipe() const
{
    return f_output_pipe;
}


/** \brief Pipe the output of this process to the next process.
 *
 * This function is used to pipe processes one after the other.
 *
 * The next process receives as input the output of this process,
 * in effect creating a pipeline of Unix processes.
 *
 * The function is called "add" (next process) because you can
 * pipe the output of one process to any number of processes'
 * input pipe. This is done by one of our internal pipe object
 * which is capable of such a feat.
 *
 * \note
 * The pipes created in this case are created internally and you
 * have no direct or indirect access to them except from within
 * the processes added here.
 *
 * \param[in] next  A process that will receive the output of this
 * process as input.
 */
void process::add_next_process(pointer_t next)
{
    f_next.push_back(next);
}


/** \brief Clear the list of next processes.
 *
 * This function clears the list of all the next processes defined
 * in this process, cutting off the pipeline.
 */
void process::clear_next_process()
{
    f_next.clear();
}


/** \brief Retrieve the list of next processes.
 *
 * This function returns the list of next processes as created by the
 * add_next_process() function.
 *
 * A next process receives as input the output of this process--i.e.
 * if creates a pair of piped processes. Any number of processes
 * can be piped in this manner.
 *
 * When the list is empty, the output is instead sent to the output
 * pipe connection or, if not such pipe is defined, to the output
 * buffer which you can retrieve later.
 *
 * If there is more than one next process, then the process creates
 * a process_tee class which is used to send the output of this
 * process to all the following processes.
 *
 * \return The list of processes to run after this one.
 */
process::list_t process::get_next_processes() const
{
    return f_next;
}


/** \brief Setup capture of the error stream.
 *
 * By default, the error stream is set to stderr. You can also request that
 * the process object captures the error stream to a buffer by calling
 * this function with true.
 *
 * If you setup an error pipe, this flag is ignored.
 *
 * \param[in] capture  Whether to capture the error stream (true) or not (false).
 */
void process::set_capture_error(bool capture)
{
    f_capture_error = capture;
}


/** \brief Check whether the error stream will be captured or not.
 *
 * This function returns the capture flag for the error stream.
 *
 * When the capture flag is true, the process error stream will be captured
 * in an internal buffer.
 *
 * \return true if the error stream is to be captured by the process object.
 */
bool process::get_capture_error() const
{
    return f_capture_error;
}


/** \brief Read the error output of the command.
 *
 * This function reads the error output stream of the process. This
 * function converts the output to UTF-8. Note that if some bytes are
 * missing this function is likely to fail. If you are reading the
 * data little by little as it comes in, you may want to use the
 * get_binary_output() function instead. That way you can detect
 * characters such as the "\n" and at that point convert the data
 * from the previous "\n" you found in the buffer to that new "\n".
 * This will generate valid UTF-8 strings.
 *
 * This function is most often used when stderr is to be saved
 * in a different file than the default.
 *
 * \todo
 * Look at validating the UTF-8 data.
 *
 * \param[in] reset  Whether the error output so far should be cleared.
 *
 * \return The current error output buffer.
 *
 * \sa get_binary_error()
 */
std::string process::get_error(bool reset) const
{
    std::string const error(reinterpret_cast<char const *>(f_error.data()), f_error.size());
    if(reset)
    {
        const_cast<process *>(this)->f_error.clear();
    }
    return error;
}


/** \brief Read the error output of the command as a binary buffer.
 *
 * This function reads the error output of the process in binary (untouched).
 *
 * This function does not fail like get_error() which attempts to
 * convert the output of the function to UTF-8. Also the error output
 * of the command may not be UTF-8 in which case you would have to use
 * the binary version and use a different conversion.
 *
 * \param[in] reset  Whether the error output so far should be cleared.
 *
 * \return The current error output buffer.
 *
 * \sa get_error()
 */
buffer_t process::get_binary_error(bool reset) const
{
    buffer_t const error(f_error);
    if(reset)
    {
        const_cast<process *>(this)->f_error.clear();
    }
    return error;
}


/** \brief Setup a pipe where the process errors are to be written.
 *
 * This function is used to setup a pipe which will receive data from
 * the stderr of the command being started by the process object.
 *
 * The pipe is going to be setup as non-blocking, which means you can
 * attempt a very large read (pipes on Linux are generally supporting
 * about 64Kb of data) and your process won't be blocked if the amount
 * of data is smaller than what you requested.
 *
 * \param[in] callback  The callback class that is called on output arrival.
 *
 * \sa get_error_pipe()
 */
void process::set_error_pipe(ed::pipe_connection::pointer_t pipe)
{
    if(pipe->type() != ed::pipe_t::PIPE_CHILD_OUTPUT)
    {
        throw cppprocess_incorrect_pipe_type("incorrect pipe type, expected a PIPE_CHILD_OUTPUT type of pipe for the error.");
    }

    f_error_pipe = pipe;
}


/** \brief Retrieve the pointer to the error pipe.
 *
 * This function retrieves the pointer of the current error pipe.
 * By default, this is a null pointer.
 *
 * \return The callback interface pointer.
 *
 * \sa set_error_pipe()
 */
ed::pipe_connection::pointer_t process::get_error_pipe() const
{
    return f_error_pipe;
}


/** \brief Start the process.
 *
 * This function creates all the necessary things that the process requires
 * and start the command.
 *
 * If the function encounters problems before it can run the child process,
 * it returns -1 instead.
 *
 * The function always uses `fork()` and `execvpe()`. That way we handle
 * one single case, which is much easier to maintain. The process is started
 * in the background. This function doesn't wait for the process to be done.
 *
 * The input can be a buffer where you add data using add_input(). It can
 * also be setup with your own pipe_connection object. If neither is
 * specified, the process is given your stdin as a default.
 *
 * The output can automatically be captured or you can setup your own
 * pipe_connection object to receive the output. If neither is specified,
 * the process is given your stdout as a default.
 *
 * Like the output, the error stream can be captured or sent to your own
 * pipe. It otherwise defaults to stderr.
 *
 * If you are not using ed::communicator (or at least you did not call the
 * ed::communicator::run() function), you can wait on the process with
 * the wait() function. This function will call the ed::communicator::run()
 * loop. If you supplied your own pipes, you'll want to make sure to close
 * them and remove them from the ed::communicator once done with them
 * otherwise the wait() function will be stuck forever. The SIGCHLD signal
 * is also automatically handled by the wait() function.
 *
 * \return 0 on success or -1 if an error occurs.
 *
 * \sa wait()
 * \sa add_input()
 * \sa set_input_pipe()
 * \sa set_capture_output()
 * \sa get_output()
 * \sa set_capture_error()
 * \sa get_error()
 */
int process::start()
{
    if(start_process(
              ed::pipe_connection::pointer_t()
            , 0
            , ed::pipe_connection::pointer_t()) != 0)
    {
        return -1;
    }

    if(!f_next.empty())
    {
        if(f_next.size() != f_prepared_output.size())
        {
            // the prepare_output() should have generated an error already
            // so we should never get here, hence the logic error
            //
            throw cppprocess_logic_error(
                      "incorrect number of output pipes (expected "
                    + std::to_string(f_next.size())
                    + ", found "
                    + std::to_string(f_prepared_output.size())
                    + ")");
        }

        int idx(0);
        for(auto & n : f_next)
        {
            if(n->start_process(
                      f_intermediate_output_pipe
                    , idx
                    , f_internal_input_pipe) != 0)
            {
                return -1;
            }
            ++idx;
        }
    }

    if(f_intermediate_output_pipe != nullptr)
    {
        f_intermediate_output_pipe->forked();
    }

    if(f_next.size() == 1)
    {
        pointer_t n(*f_next.begin());
        while(n->f_next.size() == 1)
        {
            n = *n->f_next.begin();
        }

        if(n->f_next.empty())
        {
            // we found the last item and no tee_pipe
            //
            if(n->f_intermediate_output_pipe != nullptr)
            {
                n->f_intermediate_output_pipe->forked();
            }
        }
    }

    return 0;
}


/** \brief Wait for the command to be done.
 *
 * If you are using the ed::communicator and start a command, you will
 * receive an event whenever the command is done. However, if you are
 * not using ed::comminocator, this function helps you in hiding the
 * grueling details on how to handle the event loop just to wait for
 * a command to run.
 *
 * \exception cppprocess_recursive_call
 * However, if you use ed::communicator and are already in the run()
 * function, you can't call this function. It will raise this exception
 * if you tried to do so. Of course, it is assumed that you also called
 * the start() function to actually start the process.
 *
 * \exception cppprocess_not_started
 * If one of the processes was not started (this process or one of its
 * next processes) then this exception is raised.
 *
 * \return The exit code from the command.
 */
int process::wait()
{
    if(f_communicator->is_running())
    {
        throw cppprocess_recursive_call("you already are in the communicator::run() function, you cannot call process::wait().");
    }

    if(f_child == -1
    || !f_running)
    {
        throw cppprocess_not_started("the process was not started or already died.");
    }

    list_t n(f_next);
    for(auto & it : n)
    {
        if(it->f_child == -1
        || !it->f_running)
        {
            throw cppprocess_not_started("one of the next processes was not started or already died.");
        }

        n.insert(n.end(), it->f_next.begin(), it->f_next.end());
    }

    ed::signal_child::pointer_t child_signal(ed::signal_child::get_instance());

    // this object may not have a shared pointer so we can't add it to the
    // `n` list; for this reason we have to have a special case, unfortunately
    //
    child_signal->add_listener(
              f_child
            , std::bind(&process::child_done, this, std::placeholders::_1));

    for(auto & it : n)
    {
        child_signal->add_listener(
                  it->f_child
                , std::bind(&process::child_done, this, std::placeholders::_1));
    }

    f_communicator->add_connection(child_signal);

    f_communicator->run();

    return f_exit_code;
}


void process::child_done(ed::child_status status)
{
    // remove this signal from the communicator, we're done with it
    //
    f_communicator->remove_connection(ed::signal_child::get_instance());
    f_exit_code = status.exit_code();
}


int process::start_process(
          ed::pipe_connection::pointer_t output_fifo
        , int output_index
        , ed::pipe_connection::pointer_t input_fifo)
{
    if(f_running)
    {
        // already running
        return -1;
    }

    // prepare the pipes
    //
    prepare_input(output_fifo);
    prepare_output();
    prepare_error();

    f_child = fork();
    switch(f_child)
    {
    case -1:
        // an error occurred
        //
        return -1;

    case 0:
        // child
        //
        // we want to run the execvpe() command
        //
        execute_command(output_fifo, output_index, input_fifo);

        // the child can't safely return so just exit now
        //
        exit(1);
        snap::NOT_REACHED();
        return -1;

    default:
        // parent
        //
        if(f_input_pipe != nullptr)
        {
            f_input_pipe->forked();
        }
        if(f_internal_input_pipe != nullptr)
        {
            f_internal_input_pipe->forked();
        }
        if(f_output_pipe != nullptr)
        {
            f_output_pipe->forked();
        }

// this one is done outside, here it's too soon in the case of a pipeline
// since we need both sides to create the next process
//
//        if(f_intermediate_output_pipe != nullptr)
//        {
//std::cerr << "--- intermediate output pipe forked()\n";
//            f_intermediate_output_pipe->forked();
//        }
//
        if(f_error_pipe != nullptr)
        {
            f_error_pipe->forked();
        }
        if(f_internal_error_pipe != nullptr)
        {
            f_internal_error_pipe->forked();
        }

        f_running = true;

        return 0;

    }
    snap::NOT_REACHED();
}


void process::execute_command(
          ed::pipe_connection::pointer_t output_fifo
        , int output_index
        , ed::pipe_connection::pointer_t input_fifo)
{
    // child
    //
    try
    {
        // convert arguments so we can use them with execvpe()
        //
        std::vector<char const *> args_strings;
        args_strings.reserve(1 + f_arguments.size() + 1);
        args_strings.push_back(f_command.c_str());
        for(auto const & a : f_arguments)
        {
            args_strings.push_back(a.c_str());
        }
        args_strings.push_back(nullptr); // NULL terminated

        // convert the environment variables so we can use them with execvpe()
        //
        environment_map_t src_envs(f_environment);
        if(!f_forced_environment)
        {
            // since we do not limit the child to only the specified
            // environment, add ours but do not overwrite anything
            //
            for(char ** env(environ); *env != nullptr; ++env)
            {
                char const * s(*env);
                char const * n(s);
                while(*s != '\0')
                {
                    if(*s == '=')
                    {
                        std::string const name(n, s - n);

                        // do not overwrite user overridden values
                        //
                        if(src_envs.find(name) == src_envs.end())
                        {
                            // in Linux all is UTF-8 so we are already good here
                            //
                            src_envs[name] = s + 1;
                        }
                        break;
                    }
                    ++s;
                }
            }
        }
        std::vector<char const *> envs_strings;
        for(auto const & it : src_envs)
        {
            envs_strings.push_back(strdup((it.first + "=" + it.second).c_str()));
        }
        envs_strings.push_back(nullptr); // NULL terminated

        // replace the stdin and stdout (and optionally stderr)
        // with their respective pipes
        //
        if(f_prepared_input != -1)
        {
            if(dup2(f_prepared_input, STDIN_FILENO) < 0)  // stdin
            {
                throw cppprocess_initialization_failed("dup2() of the stdin pipe failed");
            }
        }
        if(f_prepared_output[output_index] != -1)
        {
            if(dup2(f_prepared_output[output_index], STDOUT_FILENO) < 0)  // stdout
            {
                if(f_prepared_input != -1)
                {
                    close(f_prepared_input);
                }
                throw cppprocess_initialization_failed("dup2() of the stdout pipe failed");
            }
        }
        if(f_prepared_error != -1)
        {
            if(dup2(f_prepared_error, STDERR_FILENO) < 0)  // stderr
            {
                if(f_prepared_input != -1)
                {
                    close(f_prepared_input);
                }
                if(f_prepared_output[output_index] != -1)
                {
                    close(f_prepared_output[output_index]);
                }
                throw cppprocess_initialization_failed("dup2() of the stderr pipe failed");
            }
        }

        // we duplicated the files we were interested in as required,
        // now close all the other pipes
        //
        if(input_fifo != nullptr)
        {
            input_fifo->close();
        }
        if(f_input_pipe != nullptr)
        {
            f_input_pipe->close();
        }
        if(f_internal_input_pipe != nullptr)
        {
            f_internal_input_pipe->close();
        }
        if(f_output_pipe != nullptr)
        {
            f_output_pipe->close();
        }
        if(output_fifo != nullptr)
        {
            output_fifo->close();
        }
        if(f_intermediate_output_pipe != nullptr)
        {
            f_intermediate_output_pipe->close();
        }
        if(f_error_pipe != nullptr)
        {
            f_error_pipe->close();
        }
        if(f_internal_error_pipe != nullptr)
        {
            f_internal_error_pipe->close();
        }

        execvpe(
            f_command.c_str(),
            const_cast<char * const *>(&args_strings[0]),
            const_cast<char * const *>(&envs_strings[0])
        );

        // the child returns only if execvp() fails, which is possible
        //
        int const e(errno);
        SNAP_LOG_FATAL
            << "Starting child process \""
            << f_command
            << " "
            << snap::join_strings(f_arguments, " ")
            << "\" failed. (errno: "
            << e
            << " -- "
            << strerror(e)
            << ")"
            << SNAP_LOG_SEND;
    }
    catch(libexcept::exception_t const & e)
    {
        SNAP_LOG_FATAL
            << "process::run(): snap_exception caught: "
            << e.what()
            << SNAP_LOG_SEND;
    }
    catch(std::exception const & e)
    {
        // the snap_logic_exception is not a snap_exception
        // and other libraries may generate other exceptions
        // (i.e. libtld, libQtCassandra...)
        //
        SNAP_LOG_FATAL
            << "process::run(): std::exception caught: "
            << e.what()
            << SNAP_LOG_SEND;
    }
    catch(...)
    {
        SNAP_LOG_FATAL
            << "process::run(): unknown exception caught!"
            << SNAP_LOG_SEND;
    }
}


/** \brief Setup the input pipe.
 *
 * This function prepare the input pipe. If this process is a \em next
 * process, then the output_fifo will be set to the output of the previous
 * process. In effect, this allows us to pipe any number of commands
 * automatically and with just one pipe object.
 *
 * The output_fifo is that pipe file descriptor.
 *
 * The last process should instead setup an output fifo with the
 * set_output_pipe() function.
 *
 * \note
 * If an output fifo gets set in a process other than a last process
 * (i.e. a process with at least one next() process), then the function
 * throws an error (i.t. invalid setup).
 */
void process::prepare_input(ed::pipe_connection::pointer_t output_fifo)
{
    // piping between process objects
    //
    if(output_fifo != nullptr)
    {
        // we are being piped from a previous command, we must be
        // using this output_fifo as our input
        //
        if(f_input_pipe != nullptr)
        {
            throw cppprocess_invalid_parameters("you cannot pipe a command (add_next()) and define your own input pipe.");
        }
        if(!f_input.empty())
        {
            throw cppprocess_invalid_parameters("you cannot pipe a command (add_next()) and define your own input data.");
        }

        //f_prepared_input = output_fifo->get_other_socket();
        f_prepared_input = output_fifo->get_socket();
        return;
    }

    // user specified pipe
    //
    if(f_input_pipe != nullptr)
    {
        if(!f_input.empty())
        {
            throw cppprocess_invalid_parameters("you cannot define pipe and input data in the same process.");
        }

        f_prepared_input = f_input_pipe->get_other_socket();
        return;
    }

    if(!f_input.empty())
    {
        // the user supplied a buffer instead of a pipe, we create a pipe
        // to send that buffer which will be pretty much automatic
        //
        f_internal_input_pipe = std::make_shared<direct_input_data>(this, f_input);
        f_prepared_input = f_internal_input_pipe->get_other_socket();
        f_communicator->add_connection(f_internal_input_pipe);
        return;
    }

    // as a fallback, pass our stdin
    //
    f_prepared_input = STDIN_FILENO;
}


/** \brief Prepare the output pipes.
 *
 * The output can be unique (99% of the time) or it can be duplicated
 * with a tee pipe.
 *
 * The tee pipe is an internal feature used when a process is piped to
 * two or more other processes. Each one of the following processes
 * receives a copy of the previous process output.
 *
 * If there are no output processes, then the output is expected to
 * use a pipe that you define with set_output_pipe(). If you don't
 * define an output pipe, then stdout is used.
 */
void process::prepare_output()
{
    // the output has four cases
    //
    // 1. there is exactly one process following this one
    //
    //     we need a simple FIFO between the two processes; this is handled
    //     internally
    //
    //     in this case it is illegal for the user to define an output FIFO
    //
    // 2. there is more than one process following this one (tee-feature)
    //
    //     we create one output FIFO and each of the following processes
    //     will receive a specific input FIFO; our output FIFO will
    //     duplicate all the data it receives in all the input FIFOs
    //
    //     in this case it is illegal for the user to define an output FIFO
    //
    // 3. no following processes & a user defined output FIFO
    //
    //     the user output FIFO is used for our stdout
    //
    // 4. no following processes, no user defined output FIFO, capture is true
    //
    //     create an internal FIFO and save output in process
    //
    // 5. no following processes & no user defined output FIFO
    //
    //     we use the default stdout
    //

    switch(f_next.size())
    {
    case 0:
        // no piping to another process:
        //   1. use the user output if defined (f_output_pipe)
        //   2. use internal output if f_capture_output is true
        //   3. use stdout
        //
        if(f_output_pipe != nullptr)
        {
            f_prepared_output.push_back(f_output_pipe->get_socket());
        }
        else if(f_capture_output)
        {
            f_intermediate_output_pipe = std::make_shared<capture_output_pipe>(this, f_output);
            f_prepared_output.push_back(f_intermediate_output_pipe->get_other_socket());
            f_communicator->add_connection(f_intermediate_output_pipe);
        }
        else
        {
            // as a fallback, pass our stdout
            //
            f_prepared_output.push_back(STDOUT_FILENO);
        }
        break;

    case 1:
        // normal case where there is a one to one match
        // (i.e. not tee-feature required)
        //
        if(f_output_pipe != nullptr)
        {
            throw cppprocess_invalid_parameters("you cannot pipe a command (add_next()) and define your own output pipe in the sender.");
        }
        f_intermediate_output_pipe = std::make_shared<direct_output_to_input_pipe>();
        f_prepared_output.push_back(f_intermediate_output_pipe->get_other_socket());

        // in this one case, the FIFO works automatically, our communicator
        // does not have to intervene -- so no add required
        //
        //f_communicator->add_connection(f_intermediate_output_pipe);
        break;

    default:
        // special case where we create one output pipe that
        // distribute the output to N input pipes for the next
        // N processes
        //
        if(f_output_pipe != nullptr)
        {
            throw cppprocess_invalid_parameters("you cannot pipe the output of a command (add_next()) to many other commands and define your own output pipe in the sender.");
        }
        f_intermediate_output_pipe = std::make_shared<tee_pipe>(f_next.size());
        for(std::size_t idx(0); idx < f_next.size(); ++idx)
        {
            f_prepared_output.push_back(std::dynamic_pointer_cast<tee_pipe>(f_intermediate_output_pipe)->get_output_pipe(idx)->get_other_socket());
        }
        f_communicator->add_connection(f_intermediate_output_pipe);
        break;

    }
}


/** \brief Prepare the error file descriptor.
 *
 * This function prepares the error file descriptor.
 *
 * The error pipe is set to stderr if you did not setup an error pipe with
 * the set_error_pipe() function.
 */
void process::prepare_error()
{
    // user specified pipe
    //
    if(f_error_pipe != nullptr)
    {
        f_prepared_error = f_error_pipe->get_other_socket();
        return;
    }

    if(f_capture_error)
    {
        f_internal_error_pipe = std::make_shared<capture_output_pipe>(this, f_error);
        f_prepared_error = f_internal_error_pipe->get_other_socket();
        f_communicator->add_connection(f_internal_error_pipe);
        return;
    }

    // as a fallback, pass our stderr
    //
    f_prepared_error = STDERR_FILENO;
}



/** \brief Send the specified signal to this process.
 *
 * When the process is running, it is possible to send it a signal using
 * this function. The signal is sent immediately.
 *
 * \param[in] sig  The signal to send to the child process.
 *
 * \return 0 if the signal was sent, -1 on error and errno is set.
 */
int process::kill(int sig)
{
    if(f_child != -1
    && f_running)
    {
        return ::kill(f_child, sig);
    }

    errno = ESRCH;
    return -1;
}


/** \brief Call this function once down with the output pipe.
 *
 * This function will remove the intermediate output pipe from the
 * communicator. This pipe is an internal pipe created when you don't
 * specify your own.
 *
 * We call this function as soon as we get an error or HUP in the
 * pipe managed internally.
 */
void process::input_pipe_done()
{
    if(f_internal_input_pipe != nullptr)
    {
        f_communicator->remove_connection(f_internal_input_pipe);
        f_internal_input_pipe->close();
        f_internal_input_pipe.reset();
    }
}


/** \brief Call this function once down with the output or error pipe.
 *
 * This function will remove the intermediate output pipe from the
 * communicator. This pipe is an internal pipe created when you don't
 * specify your own.
 *
 * We call this function as soon as we get an error or HUP in the
 * pipe managed internally.
 *
 * \param[in] p  The pipe being closed.
 */
void process::output_pipe_done(ed::pipe_connection * p)
{
    if(f_intermediate_output_pipe.get() == p)
    {
        f_communicator->remove_connection(f_intermediate_output_pipe);
        f_intermediate_output_pipe->close();
        f_intermediate_output_pipe.reset();
    }
    else if(f_internal_error_pipe.get() == p)
    {
        f_communicator->remove_connection(f_internal_error_pipe);
        f_internal_error_pipe->close();
        f_internal_error_pipe.reset();
    }
    else if(f_next.size() == 1)
    {
        // for the tee, we're not internally responsible for final output
        // pipes, but when only one we want to take care of it here
        //
        pointer_t n(*f_next.begin());
        while(n->f_next.size() != 0)
        {
            if(n->f_next.size() != 1)
            {
                return;
            }
            n = *n->f_next.begin();
        }
        if(n->f_intermediate_output_pipe.get() == p)
        {
            f_communicator->remove_connection(n->f_intermediate_output_pipe);
            n->f_intermediate_output_pipe->close();
            n->f_intermediate_output_pipe.reset();
        }
    }
}



} // namespace cppthread
// vim: ts=4 sw=4 et
