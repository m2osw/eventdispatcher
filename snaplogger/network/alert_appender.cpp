// Copyright (c) 2021-2025  Made to Order Software Corp.  All Rights Reserved
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
 * \brief The implementation of the alert appender.
 *
 * This file implements the sending of log messages via TCP whenever too
 * many of a certain set of messages were received, in effect generating
 * alerts from things that should never happen.
 *
 * The alert system is expected to send its messages to a daemon which
 * can then convert those log messages in an email or other type of
 * message that quickly reaches the administrators.
 */

// self
//
#include    "snaplogger/network/alert_appender.h"



// advgetopt
//
#include    <advgetopt/validator_integer.h>


// snaplogger
//
#include    "snaplogger/guard.h"


// C++
//
#include    <iostream>


// last include
//
#include    <snapdev/poison.h>



namespace snaplogger_network
{


namespace
{


APPENDER_FACTORY(alert);








}
// no name namespace



snaplogger::component::pointer_t        g_alert_component(snaplogger::get_component(COMPONENT_ALERT));



alert_appender::alert_appender(std::string const & name)
    : tcp_appender(name)
{
}


alert_appender::~alert_appender()
{
}


void alert_appender::set_config(advgetopt::getopt const & opts)
{
    tcp_appender::set_config(opts);

    // COUNTER
    //
    std::string const standard_field(get_name() + "::standard");
    if(opts.is_defined(standard_field))
    {
        // TODO: determine how we want to count what
        //
        std::string const standard(opts.get_string(standard_field));
        if(standard == "off")
        {
            f_limit = -1;
        }
        else if(standard == "instant")
        {
            f_limit = 0;
        }
        else
        {
            advgetopt::validator_integer::convert_string(standard, f_limit);
        }
    }

    // ALERT
    //
    std::string const alert_field(get_name() + "::alert");
    if(opts.is_defined(alert_field))
    {
        std::string const alert(opts.get_string(alert_field));
        if(alert == "instant")
        {
            f_alert_limit = 0;
        }
        else
        {
            advgetopt::validator_integer::convert_string(alert, f_alert_limit);
        }
    }
}


void alert_appender::process_message(
          snaplogger::message const & msg
        , std::string const & formatted_message)
{
    bool forward(false);

    {
        snaplogger::guard g;

        // we make a distinction between messages clearly marked as alerts
        // and others
        //
        // TODO: add support for other component names from settings
        //
        if(msg.has_component(g_alert_component))
        {
            ++f_alert_counter;
            forward = f_alert_counter >= f_alert_limit;
            if(forward)
            {
                f_alert_counter = 0;
            }
        }
        else if(f_limit >= 0)
        {
            ++f_counter;
            forward = f_counter >= f_limit;
            if(forward)
            {
                f_counter = 0;
            }
        }
        // else -- ignore that message
    }

    if(forward)
    {
        // the alert component should be added, I should be able to add
        // that component, but the msg is const and we do not have a copy
        // option at the moment
        //
        if(!msg.has_component(g_alert_component)
        && msg.can_add_component(g_alert_component))
        {
            // we can't copy a message (it includes a stream) and it is const
            //
            // so instead we have a special process_message() which accepts
            // an extra component
            //
            tcp_appender::process_message(
                      msg
                    , formatted_message
                    , g_alert_component);
        }
        else
        {
            tcp_appender::process_message(msg, formatted_message);
        }
    }
}



} // snaplogger_network namespace
// vim: ts=4 sw=4 et
