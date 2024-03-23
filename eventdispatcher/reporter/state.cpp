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


// last include
//
#include    <snapdev/poison.h>


namespace SNAP_CATCH2_NAMESPACE
{
namespace reporter
{



ip_t state::get_ip() const
{
    return f_ip;
}


void state::set_ip(ip_t ip)
{
    if(ip > f_program.size())
    {
        throw std::out_of_range("ip out of program not allowed.");
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
        throw std::out_of_range("ip out of program not allowed.");
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
            throw std::runtime_error("the \"name\" parameter of the \"label\" statement is mandatory.");
        }
        if(name->get_operator() != operator_t::OPERATOR_PRIMARY)
        {
            throw std::runtime_error("the value of the \"name\" parameter of the \"label\" statement cannot be dynamically computed.");
        }
        token const & t(name->get_token());
        if(t.get_token() != token_t::TOKEN_IDENTIFIER)
        {
            throw std::runtime_error("the value of the \"name\" parameter of the \"label\" statement must be an identifier.");
        }
        auto const it(f_labels.find(t.get_string()));
        if(it != f_labels.end())
        {
            throw std::runtime_error("label \""
                + t.get_string()
                + "\" already defined at position "
                + std::to_string(it->second)
                + ".");
        }
        f_labels[t.get_string()] = f_program.size();
    }

    f_program.push_back(stmt);
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
            throw std::runtime_error("parameter \"" + name + "\" is required.");
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


ip_t state::get_label_position(std::string const & name) const
{
    auto const it(f_labels.find(name));
    if(it == f_labels.end())
    {
        throw std::runtime_error("label \"" + name + "\" not found.");
    }
    return it->second;
}


compare_t state::get_compare() const
{
    if(f_compare == compare_t::COMPARE_UNDEFINED)
    {
        throw std::runtime_error("trying to use a 'compare' result when none are currently defined.");
    }
    return f_compare;
}


void state::set_compare(compare_t c)
{
    if(c == compare_t::COMPARE_UNDEFINED)
    {
        throw std::runtime_error("'compare' cannot be set to \"undefined\".");
    }
    f_compare = c;
}


ed::message const & state::get_message() const
{
    return f_message;
}


void state::set_message(ed::message const & msg)
{
    f_message = msg;
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



} // namespace reporter
} // namespace SNAP_CATCH2_NAMESPACE
// vim: ts=4 sw=4 et
