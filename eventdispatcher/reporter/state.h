// Copyright (c) 2012-2024  Made to Order Software Corp.  All Rights Reserved
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
#include    <eventdispatcher/reporter/instruction.h>
#include    <eventdispatcher/reporter/statement.h>
#include    <eventdispatcher/reporter/variable.h>


// eventdispatcher
//
#include    <eventdispatcher/connection.h>
#include    <eventdispatcher/message.h>


// C++
//
#include    <functional>


// C
//
#include    <pthread.h>



// view these as an extension of the snapcatch2 library
namespace SNAP_CATCH2_NAMESPACE
{
namespace reporter
{



typedef std::uint32_t       ip_t;

enum class compare_t : int8_t
{
    COMPARE_UNDEFINED = -3,
    COMPARE_UNORDERED = -2,
    COMPARE_LESS      = -1,
    COMPARE_EQUAL     = 0,
    COMPARE_GREATER   = 1,

    COMPARE_TRUE      = COMPARE_GREATER,
    COMPARE_FALSE     = COMPARE_EQUAL,
};


enum class callback_reason_t
{
    CALLBACK_REASON_BEFORE_CALL,
    CALLBACK_REASON_AFTER_CALL,
};


enum class connection_type_t
{
    CONNECTION_TYPE_MESSENGER,          // msg:... (messenger_tcp_server)
    CONNECTION_TYPE_TCP,                // tcp:... (direct_tcp_server)
};


typedef std::vector<std::uint8_t>                   connection_data_t;
typedef std::shared_ptr<connection_data_t>          connection_data_pointer_t;
typedef std::list<connection_data_pointer_t>        connection_data_list_t;


class state
{
public:
    typedef std::shared_ptr<state>  pointer_t;
    typedef std::function<void(state & s, callback_reason_t reason)> trace_callback_t;

    pid_t                   get_server_pid() const;
    pthread_t               get_server_thread_id() const;

    ip_t                    get_ip() const;
    void                    set_ip(ip_t ip);
    void                    push_ip();
    void                    pop_ip();

    statement::pointer_t    get_statement(ip_t ip) const;
    std::size_t             get_statement_size() const;
    void                    add_statement(statement::pointer_t stmt);

    statement::pointer_t    get_running_statement() const;
    void                    set_running_statement(statement::pointer_t stmt);
    void                    clear_parameters();
    void                    add_parameter(variable::pointer_t param);
    variable::pointer_t     get_parameter(std::string const & name, bool required = false) const;

    variable::pointer_t     get_variable(std::string const & name) const;
    void                    set_variable(variable::pointer_t variable);
    void                    unset_variable(std::string const & name);

    std::uint32_t           get_label_position(std::string const & name) const;
    std::string const &     get_location() const;

    compare_t               get_compare() const;
    void                    set_compare(compare_t c);

    ed::message             get_message() const;
    void                    add_message(ed::message const & msg);
    void                    clear_message();

    ssize_t                 data_size() const;
    ssize_t                 read_data(connection_data_t & buf, std::size_t size);
    ssize_t                 peek_data(connection_data_t & buf, std::size_t size);
    void                    add_data(connection_data_pointer_t data);
    void                    clear_data();

    bool                    get_in_thread() const;
    void                    set_in_thread(bool in_thread);

    int                     get_exit_code() const;
    void                    set_exit_code(int code);

    trace_callback_t        get_trace_callback() const;
    void                    set_trace_callback(trace_callback_t callback);

    void                    set_connection_type(connection_type_t type);
    ed::connection::pointer_t
                            get_listen_connection() const;
    void                    listen(addr::addr const & a);
    void                    disconnect();
    void                    add_connection(ed::connection::pointer_t c);
    ed::connection::vector_t
                            get_connections() const;

private:
    typedef std::map<std::string, ip_t>     label_map_t;
    typedef std::vector<ip_t>               call_stack_t;

    pid_t                   f_server_pid = getpid();
    pthread_t               f_server_thread_id = pthread_self();
    ip_t                    f_ip = 0;                               // instruction pointer in f_program
    call_stack_t            f_stack = call_stack_t();
    statement::vector_t     f_program = statement::vector_t();      // instruction being executed with original parameters
    statement::pointer_t    f_running_statement = statement::pointer_t();
    variable::map_t         f_parameters = variable::map_t();       // parameters at time of call (replaced variables in strings, etc.)
    variable::map_t         f_variables = variable::map_t();        // program variables ("globals")
    label_map_t             f_labels = label_map_t();
    compare_t               f_compare = compare_t::COMPARE_UNDEFINED;
    bool                    f_in_thread = false;
    int                     f_exit_code = -1;
    ed::message::list_t     f_message = ed::message::list_t();
    connection_data_list_t  f_connection_data = connection_data_list_t();
    std::size_t             f_data_position = 0;
    trace_callback_t        f_trace_callback = nullptr;
    connection_type_t       f_connection_type = connection_type_t::CONNECTION_TYPE_MESSENGER;
    ed::connection::pointer_t
                            f_listen = ed::connection::pointer_t();
    ed::connection::vector_t
                            f_connections = ed::connection::vector_t();
};



} // namespace reporter
} // namespace SNAP_CATCH2_NAMESPACE
// vim: ts=4 sw=4 et
