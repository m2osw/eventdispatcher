// Copyright (c) 2013-2025  Made to Order Software Corp.  All Rights Reserved
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
#pragma once

// self
//
#include    <cppprocess/io.h>


// eventdispatcher
//
#include    <eventdispatcher/communicator.h>
#include    <eventdispatcher/pipe_connection.h>
#include    <eventdispatcher/signal_child.h>


// snapdev
//
#include    <snapdev/glob_to_list.h>


// C++
//
#include    <list>
#include    <map>
#include    <memory>



namespace cppprocess
{



class process
{
public:
    typedef std::shared_ptr<process>            pointer_t;
    typedef std::list<pointer_t>                list_t;
    typedef std::map<std::string, std::string>  environment_map_t;
    typedef std::list<std::string>              string_list_t;
    typedef snapdev::glob_to_list<string_list_t>
                                                argument_list_t;
    typedef std::function<void(ed::child_status status)>
                                                process_done_t;

                                    process(std::string const & name);
                                    process(process const & rhs) = delete;
    process &                       operator = (process const & rhs) = delete;

    std::string const &             get_name() const;

    // setup the process
    //
    void                            set_forced_environment(bool forced = true);
    bool                            get_forced_environment() const;

    void                            set_working_directory(std::string const & name);
    std::string const &             get_working_directory() const;
    void                            set_command(std::string const & name);
    std::string const &             get_command() const;
    std::string                     get_command_line() const;
    bool                            add_argument(std::string const & arg, bool expand = false);
    argument_list_t &               get_arguments();
    argument_list_t const &         get_arguments() const;
    void                            add_environ(std::string const & name, std::string const & value);
    environment_map_t const &       get_environ() const;

    void                            set_input_io(io::pointer_t input);
    io::pointer_t                   get_input_io() const;
    void                            set_output_io(io::pointer_t output);
    io::pointer_t                   get_output_io() const;
    void                            set_error_io(io::pointer_t output);
    io::pointer_t                   get_error_io() const;

    // what is received from the command stdout
    //
    void                            add_next_process(pointer_t next);
    void                            clear_next_process();
    list_t                          get_next_processes() const;

    // start/stop the process(es)
    //
    pid_t                           process_pid() const;
    int                             start();
    int                             wait();
    int                             exit_code() const;
    int                             kill(int sig);
    void                            set_process_done(process_done_t callback);

private:
    int                             start_process(
                                              ed::pipe_connection::pointer_t output_fifo
                                            , int output_index
                                            , io::pointer_t input_fifo);
    void                            child_done(ed::child_status status);
    void                            execute_command(
                                              ed::pipe_connection::pointer_t output_fifo
                                            , int output_index
                                            , io::pointer_t input_fifo);
    void                            prepare_input(ed::pipe_connection::pointer_t output_fifo);
    void                            prepare_output();
    void                            prepare_error();
    void                            input_pipe_done();
    void                            output_pipe_done(ed::pipe_connection * p);

    ed::communicator::pointer_t     f_communicator = ed::communicator::pointer_t();
    std::string const               f_name = std::string();
    std::string                     f_working_directory = std::string();
    std::string                     f_command = std::string();
    argument_list_t                 f_arguments = argument_list_t();
    environment_map_t               f_environment = environment_map_t();
    process_done_t                  f_process_done = process_done_t();
    bool                            f_forced_environment = false;
    bool                            f_running = false;
    bool                            f_capture_output = false;
    bool                            f_capture_error = false;
    io::pointer_t                   f_input = io::pointer_t();
    io::pointer_t                   f_output = io::pointer_t();
    io::pointer_t                   f_error = io::pointer_t();
    int                             f_prepared_input = -1;
    ed::pipe_connection::pointer_t  f_intermediate_output_pipe = ed::pipe_connection::pointer_t();
    std::vector<int>                f_prepared_output = {};
    int                             f_prepared_error = -1;
    list_t                          f_next = list_t();
    pid_t                           f_child = -1;
    int                             f_exit_code = -1;
};



} // namespace cppprocess
// vim: ts=4 sw=4 et
