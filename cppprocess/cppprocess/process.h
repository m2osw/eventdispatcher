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
#pragma once

// eventdispatcher lib
//
#include    <eventdispatcher/communicator.h>
#include    <eventdispatcher/pipe_connection.h>
#include    <eventdispatcher/signal_child.h>


// snapdev lib
//
#include    <snapdev/glob_to_list.h>


// C++ lib
//
#include    <list>
#include    <map>
#include    <memory>



namespace cppprocess
{



typedef std::vector<std::uint8_t>           buffer_t;



class process
{
public:
    typedef std::shared_ptr<process>            pointer_t;
    typedef std::list<pointer_t>                list_t;
    typedef std::map<std::string, std::string>  environment_map_t;
    typedef std::list<std::string>              string_list_t;
    typedef snap::glob_to_list<string_list_t>   argument_list_t;
    typedef std::function<void(ed::child_status status)>
                                                process_done_t;
    typedef std::function<void(std::string const & capture)>
                                                capture_done_t;

                                    process(std::string const & name);
                                    process(process const & rhs) = delete;
    process &                       operator = (process const & rhs) = delete;

    std::string const &             get_name() const;

    // setup the process
    //
    void                            set_forced_environment(bool forced = true);
    bool                            get_forced_environment() const;

    void                            set_command(std::string const & name);
    std::string const &             get_command() const;
    bool                            add_argument(std::string const & arg, bool expand = false);
    argument_list_t &               get_arguments();
    argument_list_t const &         get_arguments() const;
    void                            add_environ(std::string const & name, std::string const & value);
    environment_map_t const &       get_environ() const;

    // what is sent to the command stdin
    //
    void                            set_input_filename(std::string const & filename);
    std::string const &             get_input_filename() const;
    void                            add_input(std::string const & input);
    void                            add_input(buffer_t const & input);
    std::string                     get_input(bool reset = false) const;
    buffer_t                        get_binary_input(bool reset = false) const;
    void                            set_input_pipe(ed::pipe_connection::pointer_t pipe);
    ed::pipe_connection::pointer_t  get_input_pipe() const;

    // what is received from the command stdout
    //
    void                            set_output_filename(std::string const & filename);
    std::string const &             get_output_filename() const;
    void                            set_capture_output(bool capture = true);
    bool                            get_capture_output() const;
    void                            set_output_capture_done(capture_done_t callback);
    std::string                     get_output(bool reset = false) const;
    std::string                     get_trimmed_output(bool inside = false, bool reset = false) const;
    buffer_t                        get_binary_output(bool reset = false) const;
    void                            set_output_pipe(ed::pipe_connection::pointer_t pipe);
    ed::pipe_connection::pointer_t  get_output_pipe() const;
    void                            add_next_process(pointer_t next);
    void                            clear_next_process();
    list_t                          get_next_processes() const;

    // what is received from the command stderr
    //
    void                            set_error_filename(std::string const & filename);
    std::string const &             get_error_filename() const;
    void                            set_capture_error(bool capture = true);
    bool                            get_capture_error() const;
    void                            set_error_capture_done(capture_done_t callback);
    std::string                     get_error(bool reset = false) const;
    buffer_t                        get_binary_error(bool reset = false) const;
    void                            set_error_pipe(ed::pipe_connection::pointer_t pipe);
    ed::pipe_connection::pointer_t  get_error_pipe() const;

    // start/stop the process(es)
    //
    int                             start();
    int                             wait();
    int                             kill(int sig);
    void                            set_process_done(process_done_t callback);

    // these are internal functions called by an internal pipe
    // once the capture is over
    //
    void                            input_pipe_done();
    void                            output_pipe_done(ed::pipe_connection * p);

private:
    int                             start_process(
                                              ed::pipe_connection::pointer_t output_fifo
                                            , int output_index
                                            , ed::pipe_connection::pointer_t input_fifo);
    void                            child_done(ed::child_status status);
    void                            execute_command(
                                              ed::pipe_connection::pointer_t output_fifo
                                            , int output_index
                                            , ed::pipe_connection::pointer_t input_fifo);
    void                            prepare_input(ed::pipe_connection::pointer_t output_fifo);
    void                            prepare_output();
    void                            prepare_error();

    ed::communicator::pointer_t     f_communicator = ed::communicator::pointer_t();
    std::string const               f_name = std::string();
    std::string                     f_command = std::string();
    argument_list_t                 f_arguments = argument_list_t();
    environment_map_t               f_environment = environment_map_t();
    process_done_t                  f_process_done = process_done_t();
    bool                            f_forced_environment = false;
    bool                            f_running = false;
    bool                            f_capture_output = false;
    bool                            f_capture_error = false;
    buffer_t                        f_input = buffer_t();
    buffer_t                        f_output = buffer_t();
    buffer_t                        f_error = buffer_t();
    std::string                     f_input_filename = std::string();
    snap::raii_fd_t                 f_input_file = snap::raii_fd_t();
    ed::pipe_connection::pointer_t  f_input_pipe = ed::pipe_connection::pointer_t();
    ed::pipe_connection::pointer_t  f_internal_input_pipe = ed::pipe_connection::pointer_t();
    int                             f_prepared_input = -1;
    std::string                     f_output_filename = std::string();
    snap::raii_fd_t                 f_output_file = snap::raii_fd_t();
    ed::pipe_connection::pointer_t  f_output_pipe = ed::pipe_connection::pointer_t();
    ed::pipe_connection::pointer_t  f_intermediate_output_pipe = ed::pipe_connection::pointer_t();
    std::vector<int>                f_prepared_output = {};
    capture_done_t                  f_output_done_callback = capture_done_t();
    std::string                     f_error_filename = std::string();
    snap::raii_fd_t                 f_error_file = snap::raii_fd_t();
    ed::pipe_connection::pointer_t  f_error_pipe = ed::pipe_connection::pointer_t();
    ed::pipe_connection::pointer_t  f_internal_error_pipe = ed::pipe_connection::pointer_t();
    int                             f_prepared_error = -1;
    capture_done_t                  f_error_done_callback = capture_done_t();
    list_t                          f_next = list_t();
    pid_t                           f_child = -1;
    int                             f_exit_code = -1;
};



} // namespace cppprocess
// vim: ts=4 sw=4 et
