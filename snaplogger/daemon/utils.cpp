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
 * \brief Appenders are used to append data to somewhere.
 *
 * This file declares the base appender class.
 */

// self
//
#include    "snaplogger/daemon/utils.h"

#include    "snaplogger/daemon/network_component.h"



// advgetopt
//
#include    <advgetopt/utils.h>


// last include
//
#include    <snapdev/poison.h>



namespace snaplogger_daemon
{


snaplogger::message::pointer_t ed_message_to_log_message(ed::message const & message)
{
    // TODO: let admin select the default severity in case it's not
    //       defined in the incoming message
    //
    snaplogger::severity_t severity(snaplogger::severity_t::SEVERITY_ERROR);
    if(message.has_parameter("severity"))
    {
        std::string const & name(message.get_parameter("severity"));
        snaplogger::severity::pointer_t s(snaplogger::get_severity(name));
        if(s == nullptr)
        {
            SNAP_LOG_WARNING
                << "unknown severity \""
                << name
                << SNAP_LOG_SEND;
        }
        else
        {
            severity = s->get_severity();
        }
    }

    std::string filename;
    if(message.has_parameter("filename"))
    {
        filename = message.get_parameter("filename");
    }

    std::string function;
    if(message.has_parameter("function"))
    {
        function = message.get_parameter("function");
    }

    int line(0);
    if(message.has_parameter("line"))
    {
        line = message.get_integer_parameter("line");
    }

    snaplogger::message::pointer_t msg(std::make_shared<snaplogger::message>(severity));
    if(message.has_parameter("timestamp"))
    {
// TODO: once we have stdc++20, remove all defaults & pragma
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wpedantic"
        timespec const timestamp =
        {
            .tv_sec = message.get_integer_parameter("timestamp"),
            .tv_nsec = 0,
        };
#pragma GCC diagnostic pop
        msg->set_timestamp(timestamp);
    }

    if(message.has_parameter("message"))
    {
        *msg << message.get_parameter("message");
    }

    if(message.has_parameter("recursive"))
    {
        msg->set_recursive_message(advgetopt::is_true(message.get_parameter("recursive")));
    }

    bool is_local(false);
    if(message.has_parameter("components"))
    {
        std::string const all_components(message.get_parameter("components"));
        advgetopt::string_list_t components;
        advgetopt::split_string(all_components, components, {","});
        for(auto c : components)
        {
            if(c == "local")
            {
                is_local = true;
            }
            msg->add_component(snaplogger::get_component(*msg, c));
        }
    }
    else
    {
        // the "normal" component is the default, but if we add other
        // components the "normal" component will not be there, which may
        // fail some appender tests, so we have to add it unless "secure" or
        // some other exclusive component is present
        //
        if(msg->can_add_component(snaplogger::g_normal_component))
        {
            msg->add_component(snaplogger::g_normal_component);
        }
    }
    if(!is_local)
    {
        msg->add_component(g_remote_component);
    }
    msg->add_component(g_network_component);

    if(message.has_parameter("fields"))
    {
        std::string const fields(message.get_parameter("fields"));

        // the fields could have commas and semi-colons so we need to
        // have the backslashes analyzed as we go
        //
        char state('n');
        std::string name;
        std::string value;
        for(auto f : fields)
        {
            if(f == '\\')
            {
                state &= 0x5F;  // 'n' -> 'N' and 'v' -> 'V'
            }
            else switch(state)
            {
            case 'N':
                name += f;
                state = 'n';
                break;

            case 'V':
                value += f;
                state = 'v';
                break;

            case 'n':
                if(f == ':')
                {
                    state = 'v';
                }
                else if(f == ',')
                {
                    // field with an empty value
                    //
                    msg->add_field(name, std::string());
                    name.clear();
                }
                else
                {
                    name += f;
                }
                break;

            case 'v':
                if(f == ',')
                {
                    msg->add_field(name, value);
                    name.clear();
                    value.clear();

                    state = 'n';
                }
                else
                {
                    value += f;
                }
                break;

            default:
                throw std::logic_error("unexpected state");

            }
        }
    }

    return msg;
}


} // snaplogger_daemon namespace
// vim: ts=4 sw=4 et
