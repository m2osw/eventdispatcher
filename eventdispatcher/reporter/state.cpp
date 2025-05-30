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

// self
//
#include    "state.h"

#include    "direct_tcp_server.h"
#include    "messenger_tcp_server.h"


// eventdispatcher
//
#include    <eventdispatcher/exception.h>
#include    <eventdispatcher/tcp_server_connection.h>


// last include
//
#include    <snapdev/poison.h>



namespace SNAP_CATCH2_NAMESPACE
{
namespace reporter
{


namespace
{



std::string const       g_no_location = std::string();



} // no name namespace



ip_t state::get_ip() const
{
    return f_ip;
}


int state::get_server_pid() const
{
    return f_server_pid;
}


pthread_t state::get_server_thread_id() const
{
    return f_server_thread_id;
}


void state::set_ip(ip_t ip)
{
    if(ip > f_program.size())
    {
        throw ed::out_of_range("ip out of program not allowed.");
    }

    f_ip = ip;
}


void state::push_ip()
{
    f_stack.push_back(f_ip);
}


void state::pop_ip()
{
    f_ip = f_stack.back();
    f_stack.pop_back();
}


statement::pointer_t state::get_statement(ip_t ip) const
{
    if(ip >= f_program.size())
    {
        throw ed::out_of_range("ip out of program not allowed.");
    }

    return f_program[ip];
}


std::size_t state::get_statement_size() const
{
    return f_program.size();
}


void state::add_statement(statement::pointer_t stmt)
{
    // save labels as they get added so we don't need an extra pass
    //
    if(stmt->get_instruction()->get_name() == "label")
    {
        expression::pointer_t name(stmt->get_parameter("name"));
        if(name == nullptr)
        {
            throw ed::runtime_error("the \"name\" parameter of the \"label\" statement is mandatory.");
        }
        if(name->get_operator() != operator_t::OPERATOR_PRIMARY)
        {
            throw ed::runtime_error("the value of the \"name\" parameter of the \"label\" statement cannot be dynamically computed.");
        }
        token const & t(name->get_token());
        if(t.get_token() != token_t::TOKEN_IDENTIFIER)
        {
            throw ed::runtime_error("the value of the \"name\" parameter of the \"label\" statement must be an identifier.");
        }
        auto const it(f_labels.find(t.get_string()));
        if(it != f_labels.end())
        {
            throw ed::runtime_error("label \""
                + t.get_string()
                + "\" already defined at position "
                + std::to_string(it->second)
                + ".");
        }
        f_labels[t.get_string()] = f_program.size();
    }

    f_program.push_back(stmt);
}


statement::pointer_t state::get_running_statement() const
{
    return f_running_statement;
}


void state::set_running_statement(statement::pointer_t stmt)
{
    f_running_statement = stmt;
}


void state::clear_parameters()
{
    f_parameters.clear();
}


void state::add_parameter(variable::pointer_t param)
{
    f_parameters[param->get_name()] = param;
}


variable::pointer_t state::get_parameter(std::string const & name, bool required) const
{
    auto const it(f_parameters.find(name));
    if(it == f_parameters.end())
    {
        if(required)
        {
            // TODO: add name of current instruction
            //
            throw ed::runtime_error("parameter \"" + name + "\" is required.");
        }
        return variable::pointer_t();
    }

    return it->second;
}


variable::pointer_t state::get_variable(std::string const & name) const
{
    auto it(f_variables.find(name));
    if(it == f_variables.end())
    {
        return variable::pointer_t();
    }
    return it->second;
}


void state::set_variable(variable::pointer_t var)
{
    f_variables[var->get_name()] = var;
}


void state::unset_variable(std::string const & name)
{
    auto it(f_variables.find(name));
    if(it != f_variables.end())
    {
        f_variables.erase(it);
    }
}


std::uint32_t state::get_label_position(std::string const & name) const
{
    auto const it(f_labels.find(name));
    if(it == f_labels.end())
    {
        throw ed::runtime_error(
                  get_location()
                + "label \"" + name + "\" not found.");
    }
    return it->second;
}


std::string const & state::get_location() const
{
    // if there is no running statement (yet) then we return an empty string
    //
    if(f_running_statement == nullptr)
    {
        return g_no_location;
    }

    return f_running_statement->get_location();
}


compare_t state::get_compare() const
{
    if(f_compare == compare_t::COMPARE_UNDEFINED)
    {
        throw ed::runtime_error("trying to use a 'compare' result when none are currently defined.");
    }
    return f_compare;
}


void state::set_compare(compare_t c)
{
    if(c == compare_t::COMPARE_UNDEFINED)
    {
        throw ed::runtime_error("'compare' cannot be set to \"undefined\".");
    }
    f_compare = c;
}


ed::message state::get_message() const
{
    if(f_message.empty())
    {
        return {};
    }
    return f_message.front();
}


void state::add_message(ed::message const & msg)
{
    f_message.push_back(msg);
}


void state::clear_message()
{
    if(!f_message.empty())
    {
        f_message.pop_front();
    }
}


ssize_t state::data_size() const
{
    ssize_t size(0);
    for(auto const & d : f_connection_data)
    {
        size += d->size();
    }
    return size;
}


ssize_t state::peek_data(connection_data_t & buf, std::size_t size)
{
    if(f_connection_data.empty())
    {
        buf.clear();
        return 0;
    }

    buf.resize(size);

    std::size_t offset(0);
    std::size_t data_position(f_data_position);
    auto it(f_connection_data.begin());
    while(it != f_connection_data.end() && offset < size)
    {
        std::size_t const copy_max_size(size - offset);
        std::size_t const copy_size(std::min((*it)->size() - data_position, copy_max_size));
        memcpy(buf.data() + offset, (*it)->data() + data_position, copy_size);
        data_position += copy_size;
        ++it;
        data_position = 0;
        offset += copy_size;
    }

    buf.resize(offset);
    return offset;
}


ssize_t state::read_data(connection_data_t & buf, std::size_t size)
{
    if(f_connection_data.empty())
    {
        errno = ENODATA;
        return -1;
    }
    errno = 0;

    buf.resize(size);
    std::size_t offset(0);
    while(!f_connection_data.empty() && offset < size)
    {
        std::size_t const copy_max_size(size - offset);
        std::size_t const copy_size(std::min(f_connection_data.front()->size() - f_data_position, copy_max_size));
        memcpy(buf.data() + offset, f_connection_data.front()->data() + f_data_position, copy_size);
        f_data_position += copy_size;
        if(f_data_position >= f_connection_data.front()->size())
        {
            f_connection_data.pop_front();
            f_data_position = 0;
        }
        offset += copy_size;
    }

    buf.resize(offset);
    return offset;
}


void state::add_data(connection_data_pointer_t data)
{
    f_connection_data.push_back(data);
}


void state::clear_data()
{
    f_connection_data.clear();
    f_data_position = 0;
}


bool state::get_in_thread() const
{
    return f_in_thread;
}


void state::set_in_thread(bool in_thread)
{
    f_in_thread = in_thread;
}


int state::get_exit_code() const
{
    return f_exit_code;
}


void state::set_exit_code(int code)
{
    f_exit_code = code;
}


state::trace_callback_t state::get_trace_callback() const
{
    return f_trace_callback;
}


void state::set_trace_callback(trace_callback_t callback)
{
    f_trace_callback = callback;
}


void state::set_connection_type(connection_type_t type)
{
    f_connection_type = type;
}


ed::connection::pointer_t state::get_listen_connection() const
{
    return f_listen;
}


void state::listen(addr::addr const & a)
{
    if(f_listen != nullptr)
    {
        throw ed::runtime_error("the listen() instruction cannot be reused without an intermediate disconnect() instruction.");
    }

    // WARNING: create an ed::connection to listen for client's connection
    //          requests; however, do NOT add that ed::connection to the
    //          communicator (i.e. these are our server connections, not
    //          client ones)
    //
    switch(f_connection_type)
    {
    case connection_type_t::CONNECTION_TYPE_TCP:
        f_listen = std::make_shared<direct_tcp_server>(this, a);
        break;

    case connection_type_t::CONNECTION_TYPE_MESSENGER:
        f_listen = std::make_shared<messenger_tcp_server>(this, a);
        break;

    // LCOV_EXCL_START
    default:
        throw ed::implementation_error("unsupported connection type in connect().");
    // LCOV_EXCL_STOP

    }
}


void state::disconnect()
{
    f_listen.reset();
}


void state::add_connection(ed::connection::pointer_t c)
{
    f_connections.push_back(c);
}


ed::connection::vector_t state::get_connections() const
{
    return f_connections;
}



} // namespace reporter
} // namespace SNAP_CATCH2_NAMESPACE
// vim: ts=4 sw=4 et
