// Copyright (c) 2021-2024  Made to Order Software Corp.  All Rights Reserved
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

/** \file
 * \brief Implementation of the base network appender.
 *
 * This file implements the common functions between the TCP and the UDP
 * network appenders.
 */

// self
//
#include    "snaplogger/network/base_network_appender.h"



// snaplogger
//
#include    <snaplogger/guard.h>


// libaddr
//
#include    <libaddr/addr_parser.h>


// snapdev
//
#include    <snapdev/string_replace_many.h>


// C++
//
#include    <iostream>


// last include
//
#include    <snapdev/poison.h>



namespace snaplogger_network
{


base_network_appender::base_network_appender(std::string const & name, std::string const & type)
    : appender(name, type)
{
}


base_network_appender::~base_network_appender()
{
}


void base_network_appender::set_config(advgetopt::getopt const & opts)
{
    appender::set_config(opts);

    // SERVER ADDRESS
    //
    std::string const server_address_field(get_name() + "::server_address");
    if(opts.is_defined(server_address_field))
    {
        f_server_address = addr::string_to_addr(
                              opts.get_string(server_address_field)
                            , "127.0.0.1"
                            , 4043
                            , get_type()
                            , false);
    }
    else
    {
        f_server_address = addr::string_to_addr(
                              "127.0.0.1:4043"
                            , "127.0.0.1"
                            , 4043
                            , get_type()
                            , false);
    }

    // ACKNOWLEDGE
    //
    std::string const acknowledge_field(get_name() + "::acknowledge");
    if(opts.is_defined(acknowledge_field))
    {
        std::string const acknowledge(opts.get_string(acknowledge_field));
        if(acknowledge == "none")
        {
            f_acknowledge = acknowledge_t::ACKNOWLEDGE_NONE;
        }
        else if(acknowledge == "severity")
        {
            f_acknowledge = acknowledge_t::ACKNOWLEDGE_SEVERITY;
        }
        else
        {
            f_acknowledge = acknowledge_t::ACKNOWLEDGE_ALL;
        }
    }

    // ACKNOWLEDGE SEVERITY
    //
    std::string const acknowledge_severity_field(get_name() + "::acknowledge_severity");
    if(opts.is_defined(acknowledge_severity_field))
    {
        std::string const acknowledge_severity(opts.get_string(acknowledge_severity_field));
        snaplogger::severity::pointer_t severity(snaplogger::get_severity(acknowledge_severity));
        if(severity != nullptr)
        {
            f_acknowledge_severity = severity->get_severity();
        }
    }

    // FALLBACK TO CONSOLE
    //
    std::string const fallback_to_console_field(get_name() + "::fallback_to_console");
    if(opts.is_defined(fallback_to_console_field))
    {
        f_fallback_to_console = advgetopt::is_true(opts.get_string(fallback_to_console_field));
    }
}


void base_network_appender::set_server_address(addr::addr const & server_address)
{
    snaplogger::guard g;

    if(f_server_address != server_address)
    {
        f_server_address = server_address;

        server_address_changed();
    }
}


void base_network_appender::server_address_changed()
{
    // do nothing by default
}


void base_network_appender::log_message_to_ed_message(
          snaplogger::message const & msg
        , ed::message & log_message
        , snaplogger::component::pointer_t extra_component)
{
    // WARNING: the Snap! environment already uses the "LOG" message for
    //          resetting the snaplogger so here we want to use something
    //          else to clearly distinguish between both
    //
    log_message.set_command("LOGGER");

    // severity
    //
    snaplogger::severity::pointer_t severity(snaplogger::get_severity(msg.get_severity()));
    if(severity != nullptr)
    {
        log_message.add_parameter("severity", severity->get_name());
    }

    // for now, only sends seconds
    timespec const & timestamp(msg.get_timestamp());
    log_message.add_parameter("timestamp", timestamp.tv_sec);

    if(!msg.get_filename().empty())
    {
        log_message.add_parameter("filename", msg.get_filename());
    }
    if(!msg.get_function().empty())
    {
        log_message.add_parameter("function", msg.get_function());
    }
    if(msg.get_line() != 0)
    {
        log_message.add_parameter("line", msg.get_line());
    }
    if(msg.get_recursive_message())
    {
        log_message.add_parameter("recursive", "true");
    }

    std::string comps;
    if(extra_component != nullptr)
    {
        comps += extra_component->get_name();
    }
    snaplogger::component::set_t const & components(msg.get_components());
    for(auto c : components)
    {
        if(!comps.empty())
        {
            comps += ',';
        }
        comps += c->get_name();
    }
    if(!comps.empty())
    {
        log_message.add_parameter("components", comps);
    }

    // this needs to be sent early and just once, but I don't have a way
    // to "simulate" the environment on the other side at the moment
    //
    //environment::pointer_t env(msg.get_environment());

    log_message.add_parameter("message", msg.get_message());

    std::string flds;
    snaplogger::field_map_t const fields(msg.get_fields());
    for(auto const & f : fields)
    {
        if(!flds.empty())
        {
            flds += ',';
        }
        std::string const safe_name(snapdev::string_replace_many(
                    f.first,
                    {
                        { ",", "\\," },
                        { ":", "\\:" },
                    }));
        std::string const safe_value(snapdev::string_replace_many(
                    f.second,
                    {
                        { ",", "\\," },
                        { ":", "\\:" },
                    }));
        flds += safe_name;
        if(!safe_value.empty())
        {
            flds += ':';
            flds += safe_value;
        }
    }
    if(!flds.empty())
    {
        log_message.add_parameter("fields", flds);
    }
}





} // snaplogger_network namespace
// vim: ts=4 sw=4 et
